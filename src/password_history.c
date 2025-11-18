#include "../include/database.h"
#include "../include/crypto.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int db_get_password_history(Database *db, int password_id, PasswordHistoryEntry **entries, int *count) {
    const char *sql = "SELECT * FROM password_history WHERE password_id = ? ORDER BY changed_at DESC LIMIT 10";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation historique: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, password_id);
    
    // Compter d'abord
    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*count)++;
    }
    
    if (*count == 0) {
        sqlite3_finalize(stmt);
        *entries = NULL;
        return 1;
    }
    
    *entries = calloc(*count, sizeof(PasswordHistoryEntry));
    if (!*entries) {
        fprintf(stderr, "❌ Erreur allocation mémoire historique\n");
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_reset(stmt);
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < *count) {
        (*entries)[i].id = sqlite3_column_int(stmt, 0);
        (*entries)[i].password_id = sqlite3_column_int(stmt, 1);
        
        const char *encrypted = (const char*)sqlite3_column_text(stmt, 2);
        if (encrypted) {
            strncpy((*entries)[i].encrypted_password, encrypted, 
                   sizeof((*entries)[i].encrypted_password) - 1);
            (*entries)[i].encrypted_password[sizeof((*entries)[i].encrypted_password) - 1] = '\0';
        }
        
        const char *website = (const char*)sqlite3_column_text(stmt, 3);
        if (website) {
            strncpy((*entries)[i].website, website, 
                   sizeof((*entries)[i].website) - 1);
            (*entries)[i].website[sizeof((*entries)[i].website) - 1] = '\0';
        }
        
        const char *username = (const char*)sqlite3_column_text(stmt, 4);
        if (username) {
            strncpy((*entries)[i].username, username, 
                   sizeof((*entries)[i].username) - 1);
            (*entries)[i].username[sizeof((*entries)[i].username) - 1] = '\0';
        }
        
        const char *changed_at = (const char*)sqlite3_column_text(stmt, 5);
        if (changed_at) {
            strncpy((*entries)[i].changed_at, changed_at, 
                   sizeof((*entries)[i].changed_at) - 1);
            (*entries)[i].changed_at[sizeof((*entries)[i].changed_at) - 1] = '\0';
        }
        
        i++;
    }
    
    sqlite3_finalize(stmt);
    return 1;
}

int db_check_password_reuse(Database *db, int password_id, const char *new_encrypted_password, int user_id) {
    const char *sql = "SELECT password_history_size FROM user_security_policies WHERE user_id = ?";
    sqlite3_stmt *stmt;
    int history_size = 5; // Valeur par défaut
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            history_size = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    sql = "SELECT encrypted_password FROM password_history WHERE password_id = ? ORDER BY changed_at DESC LIMIT ?";
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur vérification réutilisation: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, password_id);
    sqlite3_bind_int(stmt, 2, history_size);
    
    int is_reused = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *old_encrypted = (const char*)sqlite3_column_text(stmt, 0);
        if (old_encrypted && strcmp(old_encrypted, new_encrypted_password) == 0) {
            is_reused = 1;
            break;
        }
    }
    
    sqlite3_finalize(stmt);
    return is_reused;
}

int db_update_password_with_history(Database *db, int password_id, const char *new_encrypted_password, const char *master_key) {
    (void)master_key;
    
    if (!db || !new_encrypted_password) {
        return 0;
    }
    // Vérifier si le mot de passe a été utilisé récemment
    PasswordEntry current_entry;
    if (db_get_password_by_id(db, password_id, &current_entry)) {
        int is_reused = db_check_password_reuse(db, password_id, new_encrypted_password, 1);
        if (is_reused) {
            fprintf(stderr, "❌ Ce mot de passe a déjà été utilisé récemment\n");
            return -1; // Code d'erreur pour réutilisation
        }
    }
    
    // Mettre à jour le mot de passe (le trigger gérera l'historique)
    const char *sql = "UPDATE passwords SET encrypted_password = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation mise à jour: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, new_encrypted_password, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, password_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        printf("✓ Mot de passe %d mis à jour avec historique\n", password_id);
    }
    
    return rc == SQLITE_DONE;
}

int db_create_user_security_policy(Database *db, int user_id) {
    const char *sql = "INSERT OR IGNORE INTO user_security_policies (user_id) VALUES (?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

int db_get_user_security_policy(Database *db, int user_id, UserSecurityPolicy *policy) {
    const char *sql = "SELECT * FROM user_security_policies WHERE user_id = ?";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        policy->id = sqlite3_column_int(stmt, 0);
        policy->user_id = sqlite3_column_int(stmt, 1);
        policy->require_2fa = sqlite3_column_int(stmt, 2);
        policy->password_min_length = sqlite3_column_int(stmt, 3);
        policy->require_mixed_case = sqlite3_column_int(stmt, 4);
        policy->require_numbers = sqlite3_column_int(stmt, 5);
        policy->require_symbols = sqlite3_column_int(stmt, 6);
        policy->password_history_size = sqlite3_column_int(stmt, 7);
        policy->max_login_attempts = sqlite3_column_int(stmt, 8);
        policy->session_timeout = sqlite3_column_int(stmt, 9);
        policy->auto_lock_timeout = sqlite3_column_int(stmt, 10);
        
        const char *created = (const char*)sqlite3_column_text(stmt, 11);
        if (created) {
            strncpy(policy->created_at, created, sizeof(policy->created_at) - 1);
            policy->created_at[sizeof(policy->created_at) - 1] = '\0';
        }
        
        sqlite3_finalize(stmt);
        return 1;
    }
    
    sqlite3_finalize(stmt);
    return 0;
}