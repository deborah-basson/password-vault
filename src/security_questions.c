#include "../include/database.h"
#include "../include/crypto.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int db_create_user_with_questions(Database *db, const char *master_hash, const char *salt,
                                 const char *question1, const char *answer1,
                                 const char *question2, const char *answer2,
                                 const char *question3, const char *answer3,
                                 const char *recovery_code) {
    if (!db_create_user(db, master_hash, salt, "")) {
        return 0;
    }
    
    char answer1_hash[HASH_LENGTH];
    char answer2_hash[HASH_LENGTH]; 
    char answer3_hash[HASH_LENGTH];
    
    const char *demo_salt = "security_salt";
    
    hash_password(answer1, demo_salt, answer1_hash, sizeof(answer1_hash));
    hash_password(answer2, demo_salt, answer2_hash, sizeof(answer2_hash));
    hash_password(answer3, demo_salt, answer3_hash, sizeof(answer3_hash));
    
    const char *sql = "INSERT INTO security_questions (user_id, question1, answer1_hash, question2, answer2_hash, question3, answer3_hash, recovery_code) "
                     "VALUES (1, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation questions sécurité: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, question1, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, answer1_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, question2, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, answer2_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, question3, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, answer3_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, recovery_code, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        printf("✓ Questions de sécurité créées\n");
    }
    
    return rc == SQLITE_DONE;
}

int db_get_security_questions(Database *db, SecurityQuestions *questions) {
    const char *sql = "SELECT question1, question2, question3, recovery_attempts, locked_until "
                     "FROM security_questions ORDER BY id DESC LIMIT 1";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *q1 = (const char*)sqlite3_column_text(stmt, 0);
        const char *q2 = (const char*)sqlite3_column_text(stmt, 1);
        const char *q3 = (const char*)sqlite3_column_text(stmt, 2);
        
        if (q1) strncpy(questions->question1, q1, sizeof(questions->question1) - 1);
        if (q2) strncpy(questions->question2, q2, sizeof(questions->question2) - 1);
        if (q3) strncpy(questions->question3, q3, sizeof(questions->question3) - 1);
        
        questions->recovery_attempts = sqlite3_column_int(stmt, 3);
        
        const char *locked = (const char*)sqlite3_column_text(stmt, 4);
        if (locked) strncpy(questions->locked_until, locked, sizeof(questions->locked_until) - 1);
        
        sqlite3_finalize(stmt);
        return 1;
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

int db_verify_security_answers(Database *db, const char *answer1, const char *answer2, const char *answer3) {
    const char *check_sql = "SELECT locked_until FROM security_questions WHERE locked_until > datetime('now')";
    sqlite3_stmt *check_stmt;
    
    if (sqlite3_prepare_v2(db->db, check_sql, -1, &check_stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            sqlite3_finalize(check_stmt);
            return -1;
        }
        sqlite3_finalize(check_stmt);
    }
    
    const char *sql = "SELECT answer1_hash, answer2_hash, answer3_hash, recovery_attempts FROM security_questions ORDER BY id DESC LIMIT 1";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *hash1 = (const char*)sqlite3_column_text(stmt, 0);
        const char *hash2 = (const char*)sqlite3_column_text(stmt, 1);
        const char *hash3 = (const char*)sqlite3_column_text(stmt, 2);
        int current_attempts = sqlite3_column_int(stmt, 3);
        
        if (!hash1 || !hash2 || !hash3) {
            sqlite3_finalize(stmt);
            return 0;
        }
        
        const char *demo_salt = "security_salt";
        int valid1 = verify_password(answer1, demo_salt, hash1);
        int valid2 = verify_password(answer2, demo_salt, hash2);
        int valid3 = verify_password(answer3, demo_salt, hash3);
        
        sqlite3_finalize(stmt);
        
        if (valid1 && valid2 && valid3) {
            db_reset_recovery_attempts(db);
            return 1;
        } else {
            if (current_attempts >= 2) {
                db_increment_recovery_attempts(db);
                return -1;
            } else {
                db_increment_recovery_attempts(db);
                return 0;
            }
        }
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

int db_verify_recovery_code(Database *db, const char *code) {
    if (!code) return 0;
    
    const char *check_sql = "SELECT locked_until FROM security_questions WHERE locked_until > datetime('now')";
    sqlite3_stmt *check_stmt;
    
    if (sqlite3_prepare_v2(db->db, check_sql, -1, &check_stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            sqlite3_finalize(check_stmt);
            return -1;
        }
        sqlite3_finalize(check_stmt);
    }
    
    const char *sql = "SELECT recovery_code, recovery_attempts FROM security_questions ORDER BY id DESC LIMIT 1";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *stored_code_ptr = (const char*)sqlite3_column_text(stmt, 0);
        int current_attempts = sqlite3_column_int(stmt, 1);
        
        char stored_code[256];
        if (stored_code_ptr) {
            strncpy(stored_code, stored_code_ptr, sizeof(stored_code) - 1);
            stored_code[sizeof(stored_code) - 1] = '\0';
        } else {
            sqlite3_finalize(stmt);
            return 0;
        }
        
        sqlite3_finalize(stmt);
        
        if (strcmp(stored_code, code) == 0) {
            db_reset_recovery_attempts(db);
            return 1;
        } else {
            if (current_attempts >= 2) {
                db_increment_recovery_attempts(db);
                return -1;
            } else {
                db_increment_recovery_attempts(db);
                return 0;
            }
        }
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

int db_increment_recovery_attempts(Database *db) {
    const char *sql = "UPDATE security_questions SET recovery_attempts = recovery_attempts + 1";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        const char *check_sql = "SELECT recovery_attempts FROM security_questions WHERE recovery_attempts >= 3";
        sqlite3_stmt *check_stmt;
        
        if (sqlite3_prepare_v2(db->db, check_sql, -1, &check_stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(check_stmt) == SQLITE_ROW) {
                db_lock_recovery_until(db, "15 minutes");
            }
            sqlite3_finalize(check_stmt);
        }
    }
    
    return rc == SQLITE_DONE;
}

int db_reset_recovery_attempts(Database *db) {
    const char *sql = "UPDATE security_questions SET recovery_attempts = 0, locked_until = NULL";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

int db_lock_recovery_until(Database *db, const char *until_time) {
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE security_questions SET locked_until = datetime('now', '+%s')", until_time);
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        printf("🔒 Récupération verrouillée jusqu'à %s\n", until_time);
    }
    
    return rc == SQLITE_DONE;
}