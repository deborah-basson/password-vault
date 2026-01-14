#include "../include/database.h"
#include "../include/crypto.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sodium.h>

static void hash_security_answer(const char *answer, const char *salt, char *output, size_t output_len) {
    if (!answer || !salt || !output) return;
    
    unsigned char hash_raw[crypto_generichash_BYTES];
    
    char salted_answer[512];
    snprintf(salted_answer, sizeof(salted_answer), "%s|%s", answer, salt);
    
    crypto_generichash(hash_raw, sizeof(hash_raw),
                      (unsigned char*)salted_answer, strlen(salted_answer),
                      NULL, 0);
    
    sodium_bin2hex(output, output_len, hash_raw, sizeof(hash_raw));
}

int db_create_user_with_questions(Database *db, const char *master_hash, const char *salt,
                                 const char *question1, const char *answer1,
                                 const char *question2, const char *answer2,
                                 const char *question3, const char *answer3,
                                 const char *recovery_code) {
    if (!db_create_user(db, master_hash, salt, "")) {
        return 0;
    }
    
    char answer1_salt[65];
    char answer2_salt[65];
    char answer3_salt[65];
    
    unsigned char salt1[32], salt2[32], salt3[32];
    randombytes_buf(salt1, sizeof(salt1));
    randombytes_buf(salt2, sizeof(salt2));
    randombytes_buf(salt3, sizeof(salt3));
    
    sodium_bin2hex(answer1_salt, sizeof(answer1_salt), salt1, sizeof(salt1));
    sodium_bin2hex(answer2_salt, sizeof(answer2_salt), salt2, sizeof(salt2));
    sodium_bin2hex(answer3_salt, sizeof(answer3_salt), salt3, sizeof(salt3));
    
    char answer1_hash[crypto_generichash_BYTES * 2 + 1];
    char answer2_hash[crypto_generichash_BYTES * 2 + 1];
    char answer3_hash[crypto_generichash_BYTES * 2 + 1];
    
    hash_security_answer(answer1, answer1_salt, answer1_hash, sizeof(answer1_hash));
    hash_security_answer(answer2, answer2_salt, answer2_hash, sizeof(answer2_hash));
    hash_security_answer(answer3, answer3_salt, answer3_hash, sizeof(answer3_hash));
    
    const char *sql = "INSERT INTO security_questions "
                     "(user_id, question1, answer1_hash, answer1_salt, "
                     "question2, answer2_hash, answer2_salt, "
                     "question3, answer3_hash, answer3_salt, recovery_code) "
                     "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation questions sécurité: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, question1, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, answer1_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, answer1_salt, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, question2, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, answer2_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, answer2_salt, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, question3, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, answer3_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, answer3_salt, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, recovery_code, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    secure_zero_memory(answer1_hash, sizeof(answer1_hash));
    secure_zero_memory(answer2_hash, sizeof(answer2_hash));
    secure_zero_memory(answer3_hash, sizeof(answer3_hash));
    
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
    if (!db || !answer1 || !answer2 || !answer3) {
        return 0;
    }
    
    const char *check_sql = "SELECT locked_until FROM security_questions WHERE locked_until > datetime('now')";
    sqlite3_stmt *check_stmt;
    
    if (sqlite3_prepare_v2(db->db, check_sql, -1, &check_stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            sqlite3_finalize(check_stmt);
            return -1;
        }
        sqlite3_finalize(check_stmt);
    }
    
    const char *sql = "SELECT answer1_hash, answer1_salt, answer2_hash, answer2_salt, "
                     "answer3_hash, answer3_salt, recovery_attempts "
                     "FROM security_questions ORDER BY id DESC LIMIT 1";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation verify_security_answers: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    int result = 0;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *stored_hash1 = (const char*)sqlite3_column_text(stmt, 0);
        const char *salt1 = (const char*)sqlite3_column_text(stmt, 1);
        const char *stored_hash2 = (const char*)sqlite3_column_text(stmt, 2);
        const char *salt2 = (const char*)sqlite3_column_text(stmt, 3);
        const char *stored_hash3 = (const char*)sqlite3_column_text(stmt, 4);
        const char *salt3 = (const char*)sqlite3_column_text(stmt, 5);
        int current_attempts = sqlite3_column_int(stmt, 6);
        
        if (!stored_hash1 || !salt1 || !stored_hash2 || !salt2 || !stored_hash3 || !salt3) {
            sqlite3_finalize(stmt);
            return 0;
        }
        
        char computed_hash1[crypto_generichash_BYTES * 2 + 1];
        char computed_hash2[crypto_generichash_BYTES * 2 + 1];
        char computed_hash3[crypto_generichash_BYTES * 2 + 1];
        
        hash_security_answer(answer1, salt1, computed_hash1, sizeof(computed_hash1));
        hash_security_answer(answer2, salt2, computed_hash2, sizeof(computed_hash2));
        hash_security_answer(answer3, salt3, computed_hash3, sizeof(computed_hash3));
        
        int match1 = (sodium_memcmp(stored_hash1, computed_hash1, strlen(stored_hash1)) == 0);
        int match2 = (sodium_memcmp(stored_hash2, computed_hash2, strlen(stored_hash2)) == 0);
        int match3 = (sodium_memcmp(stored_hash3, computed_hash3, strlen(stored_hash3)) == 0);
        
        secure_zero_memory(computed_hash1, sizeof(computed_hash1));
        secure_zero_memory(computed_hash2, sizeof(computed_hash2));
        secure_zero_memory(computed_hash3, sizeof(computed_hash3));
        
        if (match1 && match2 && match3) {
            db_reset_recovery_attempts(db);
            result = 1;
        } else {
            if (current_attempts >= 2) {
                db_increment_recovery_attempts(db);
                result = -1;
            } else {
                db_increment_recovery_attempts(db);
                result = 0;
            }
        }
    }
    
    sqlite3_finalize(stmt);
    return result;
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