#include "../include/database.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int execute_sql_file(sqlite3 *db, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "❌ Impossible d'ouvrir %s\n", filename);
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *sql = malloc(size + 1);
    if (!sql) {
        fclose(file);
        fprintf(stderr, "❌ Erreur allocation mémoire\n");
        return 0;
    }
    
    size_t bytes_read = fread(sql, 1, size, file);
    if (bytes_read != (size_t)size) {
        fprintf(stderr, "⚠️  Lecture partielle du fichier %s\n", filename);
    }
    sql[bytes_read] = '\0';
    fclose(file);
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur SQL dans %s: %s\n", filename, err_msg);
        sqlite3_free(err_msg);
        free(sql);
        return 0;
    }
    
    free(sql);
    return 1;
}

int db_init(Database *db) {
    int rc = sqlite3_open(DB_FILE, &db->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur ouverture BD: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_exec(db->db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    sqlite3_exec(db->db, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);
    sqlite3_exec(db->db, "PRAGMA synchronous = FULL;", NULL, NULL, NULL);
    sqlite3_exec(db->db, "PRAGMA temp_store = MEMORY;", NULL, NULL, NULL);
    
    if (!execute_sql_file(db->db, "sql/schema.sql")) {
        fprintf(stderr, "⚠️  Impossible de charger sql/schema.sql, utilisation du schéma intégré\n");
        
        const char *minimal_schema = 
            "CREATE TABLE IF NOT EXISTS users ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  master_hash TEXT NOT NULL,"
            "  salt TEXT NOT NULL,"
            "  totp_secret TEXT,"
            "  is_2fa_enabled INTEGER DEFAULT 1,"
            "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
            ");"
            "CREATE TABLE IF NOT EXISTS passwords ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  website TEXT NOT NULL,"
            "  username TEXT NOT NULL,"
            "  encrypted_password TEXT NOT NULL,"
            "  notes TEXT,"
            "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
            ");"
            "CREATE TABLE IF NOT EXISTS login_attempts ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  action TEXT NOT NULL,"
            "  success INTEGER NOT NULL,"
            "  ip_address TEXT,"
            "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
            ");";
        
        char *err_msg = NULL;
        if (sqlite3_exec(db->db, minimal_schema, NULL, NULL, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "❌ Erreur création schéma: %s\n", err_msg);
            sqlite3_free(err_msg);
            return 0;
        }
    }
    
    printf("✓ Base de données initialisée\n");
    return 1;
}

int db_load_dev_data(Database *db) {
    printf("⚠️  Chargement des données de développement...\n");
    return execute_sql_file(db->db, "sql/init_dev.sql");
}

void db_close(Database *db) {
    if (db && db->db) {
        sqlite3_close(db->db);
        db->db = NULL;
        printf("✓ Base de données fermée\n");
    }
}

int db_create_user(Database *db, const char *master_hash, const char *salt, const char *totp_secret) {
    const char *sql = "INSERT INTO users (master_hash, salt, totp_secret, is_2fa_enabled) VALUES (?, ?, ?, 0)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation requête: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, master_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, salt, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, totp_secret, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "❌ Erreur création utilisateur: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    return 1;
}

int db_get_user(Database *db, UserAccount *user) {
    const char *sql = "SELECT * FROM users ORDER BY id DESC LIMIT 1";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation requête: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user->id = sqlite3_column_int(stmt, 0);
        
        const char *hash = (const char*)sqlite3_column_text(stmt, 1);
        if (hash) strncpy(user->master_hash, hash, sizeof(user->master_hash) - 1);
        
        const char *salt = (const char*)sqlite3_column_text(stmt, 2);
        if (salt) strncpy(user->salt, salt, sizeof(user->salt) - 1);
        
        const char *totp = (const char*)sqlite3_column_text(stmt, 3);
        if (totp) strncpy(user->totp_secret, totp, sizeof(user->totp_secret) - 1);
        
        user->is_2fa_enabled = sqlite3_column_int(stmt, 4);
        
        const char *created = (const char*)sqlite3_column_text(stmt, 5);
        if (created) strncpy(user->created_at, created, sizeof(user->created_at) - 1);
        
        sqlite3_finalize(stmt);
        return 1;
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

int db_update_2fa(Database *db, const char *totp_secret, int enabled) {
    const char *sql = "UPDATE users SET totp_secret = ?, is_2fa_enabled = ? WHERE id = (SELECT MAX(id) FROM users)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, totp_secret, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, enabled);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

int db_add_password(Database *db, const char *website, const char *username, 
                    const char *encrypted_pass, const char *notes) {
    const char *sql = "INSERT INTO passwords (website, username, encrypted_password, notes) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation requête: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, website, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, encrypted_pass, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, notes ? notes : "", -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "❌ Erreur ajout mot de passe: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    return 1;
}

int db_get_all_passwords(Database *db, PasswordEntry **entries, int *count) {
    const char *count_sql = "SELECT COUNT(*) FROM passwords";
    sqlite3_stmt *count_stmt;
    
    if (sqlite3_prepare_v2(db->db, count_sql, -1, &count_stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation requête de comptage: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    *count = 0;
    if (sqlite3_step(count_stmt) == SQLITE_ROW) {
        *count = sqlite3_column_int(count_stmt, 0);
    }
    sqlite3_finalize(count_stmt);
    
    if (*count == 0) {
        *entries = NULL;
        return 1;
    }
    
    *entries = calloc(*count, sizeof(PasswordEntry));
    if (!*entries) {
        fprintf(stderr, "❌ Erreur allocation mémoire\n");
        return 0;
    }
    
    const char *select_sql = "SELECT * FROM passwords ORDER BY website ASC";
    sqlite3_stmt *select_stmt;
    
    if (sqlite3_prepare_v2(db->db, select_sql, -1, &select_stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation requête de sélection: %s\n", sqlite3_errmsg(db->db));
        free(*entries);
        *entries = NULL;
        return 0;
    }
    
    int i = 0;
    while (sqlite3_step(select_stmt) == SQLITE_ROW && i < *count) {
        (*entries)[i].id = sqlite3_column_int(select_stmt, 0);
        
        const char *website = (const char*)sqlite3_column_text(select_stmt, 2);
        if (website) {
            strncpy((*entries)[i].website, website, sizeof((*entries)[i].website) - 1);
            (*entries)[i].website[sizeof((*entries)[i].website) - 1] = '\0';
        }
        
        const char *username = (const char*)sqlite3_column_text(select_stmt, 3);
        if (username) {
            strncpy((*entries)[i].username, username, sizeof((*entries)[i].username) - 1);
            (*entries)[i].username[sizeof((*entries)[i].username) - 1] = '\0';
        }
        
        const char *password = (const char*)sqlite3_column_text(select_stmt, 4);
        if (password) {
            strncpy((*entries)[i].encrypted_password, password, sizeof((*entries)[i].encrypted_password) - 1);
            (*entries)[i].encrypted_password[sizeof((*entries)[i].encrypted_password) - 1] = '\0';
        }
        
        const char *notes = (const char*)sqlite3_column_text(select_stmt, 5);
        if (notes) {
            strncpy((*entries)[i].notes, notes, sizeof((*entries)[i].notes) - 1);
            (*entries)[i].notes[sizeof((*entries)[i].notes) - 1] = '\0';
        }
        
        const char *created = (const char*)sqlite3_column_text(select_stmt, 6);
        if (created) {
            strncpy((*entries)[i].created_at, created, sizeof((*entries)[i].created_at) - 1);
            (*entries)[i].created_at[sizeof((*entries)[i].created_at) - 1] = '\0';
        }
        
        const char *updated = (const char*)sqlite3_column_text(select_stmt, 7);
        if (updated) {
            strncpy((*entries)[i].updated_at, updated, sizeof((*entries)[i].updated_at) - 1);
            (*entries)[i].updated_at[sizeof((*entries)[i].updated_at) - 1] = '\0';
        }
        
        i++;
    }
    
    sqlite3_finalize(select_stmt);
    return 1;
}

int db_get_password_by_id(Database *db, int id, PasswordEntry *entry) {
    const char *sql = "SELECT * FROM passwords WHERE id = ?";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        entry->id = sqlite3_column_int(stmt, 0);
        
        const char *website = (const char*)sqlite3_column_text(stmt, 2);
        if (website) {
            strncpy(entry->website, website, sizeof(entry->website) - 1);
            entry->website[sizeof(entry->website) - 1] = '\0';
        }
        
        const char *username = (const char*)sqlite3_column_text(stmt, 3);
        if (username) {
            strncpy(entry->username, username, sizeof(entry->username) - 1);
            entry->username[sizeof(entry->username) - 1] = '\0';
        }
        
        const char *password = (const char*)sqlite3_column_text(stmt, 4);
        if (password) {
            strncpy(entry->encrypted_password, password, sizeof(entry->encrypted_password) - 1);
            entry->encrypted_password[sizeof(entry->encrypted_password) - 1] = '\0';
        }
        
        const char *notes = (const char*)sqlite3_column_text(stmt, 5);
        if (notes) {
            strncpy(entry->notes, notes, sizeof(entry->notes) - 1);
            entry->notes[sizeof(entry->notes) - 1] = '\0';
        }
        
        sqlite3_finalize(stmt);
        return 1;
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

int db_update_password(Database *db, int id, const char *encrypted_pass) {
    const char *sql = "UPDATE passwords SET encrypted_password = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, encrypted_pass, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

int db_delete_password(Database *db, int id) {
    const char *sql = "DELETE FROM passwords WHERE id = ?";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Erreur préparation requête: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "❌ Erreur suppression: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    
    return 1;
}

int db_log_attempt(Database *db, const char *action, int success, const char *ip) {
    const char *sql = "INSERT INTO login_attempts (action, success, ip_address) VALUES (?, ?, ?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, action, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, success);
    sqlite3_bind_text(stmt, 3, ip, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

int db_get_failed_attempts(Database *db, const char *ip, int minutes) {
    const char *sql = "SELECT COUNT(*) FROM login_attempts "
                     "WHERE ip_address = ? AND success = 0 "
                     "AND timestamp > datetime('now', '-' || ? || ' minutes')";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, ip, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, minutes);
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

#include "../include/crypto.h"

int db_add_password_encrypted(Database *db, const char *website, const char *username,
                               const char *plaintext_password, const char *notes, 
                               const char *master_key) {
    if (!master_key || !plaintext_password) {
        fprintf(stderr, "❌ Clé ou mot de passe manquant pour chiffrement\n");
        return 0;
    }
    
    char encrypted[1024];
    size_t encrypted_len = sizeof(encrypted);
    
    if (!encrypt_data(plaintext_password, master_key, encrypted, &encrypted_len)) {
        fprintf(stderr, "❌ Erreur lors du chiffrement du mot de passe\n");
        return 0;
    }
    
    int result = db_add_password(db, website, username, encrypted, notes);
    
    secure_zero_memory(encrypted, sizeof(encrypted));
    
    if (result) {
        printf("✓ Mot de passe chiffré et stocké pour %s\n", website);
    }
    
    return result;
}

int db_get_password_decrypted(Database *db, int id, PasswordEntry *entry,
                              const char *master_key) {
    if (!master_key) {
        fprintf(stderr, "❌ Clé maître manquante pour déchiffrement\n");
        return 0;
    }
    
    if (!db_get_password_by_id(db, id, entry)) {
        return 0;
    }
    
    char decrypted[512];
    memset(decrypted, 0, sizeof(decrypted));
    
    size_t cipher_len = strlen(entry->encrypted_password);
    
    if (!decrypt_data(entry->encrypted_password, cipher_len, master_key, decrypted)) {
        fprintf(stderr, "❌ Erreur lors du déchiffrement du mot de passe\n");
        secure_zero_memory(decrypted, sizeof(decrypted));
        return 0;
    }
    
    strncpy(entry->encrypted_password, decrypted, sizeof(entry->encrypted_password) - 1);
    entry->encrypted_password[sizeof(entry->encrypted_password) - 1] = '\0';
    
    secure_zero_memory(decrypted, sizeof(decrypted));
    
    return 1;
}