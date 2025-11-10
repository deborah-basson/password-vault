#include "../include/config.h"
#include "../include/crypto.h"
#include "../include/database.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// ==================== HISTORIQUE DES MOTS DE PASSE ====================

typedef struct {
    int id;
    int password_id;
    char encrypted_password[512];
    char changed_at[32];
} PasswordHistory;

int db_add_password_history(Database *db, int password_id, const char *encrypted_password) {
    const char *sql = "INSERT INTO password_history (password_id, encrypted_password) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, password_id);
    sqlite3_bind_text(stmt, 2, encrypted_password, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

int db_get_password_history(Database *db, int password_id, PasswordHistory **history, int *count) {
    const char *sql = "SELECT * FROM password_history WHERE password_id = ? ORDER BY changed_at DESC LIMIT 10";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, password_id);
    
    *count = 0;
    PasswordHistory temp[10];
    
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < 10) {
        temp[*count].id = sqlite3_column_int(stmt, 0);
        temp[*count].password_id = sqlite3_column_int(stmt, 1);
        
        const char *encrypted = (const char*)sqlite3_column_text(stmt, 2);
        if (encrypted) {
            strncpy(temp[*count].encrypted_password, encrypted, 
                   sizeof(temp[*count].encrypted_password) - 1);
        }
        
        const char *changed = (const char*)sqlite3_column_text(stmt, 3);
        if (changed) {
            strncpy(temp[*count].changed_at, changed, 
                   sizeof(temp[*count].changed_at) - 1);
        }
        
        (*count)++;
    }
    
    sqlite3_finalize(stmt);
    
    if (*count > 0) {
        *history = malloc(*count * sizeof(PasswordHistory));
        memcpy(*history, temp, *count * sizeof(PasswordHistory));
    }
    
    return 1;
}

// ==================== VÉRIFICATION DE FAIBLESSE DES MOTS DE PASSE ====================

int is_common_password(const char *password) {
    // Liste des mots de passe courants à éviter
    const char *common_passwords[] = {
        "password", "123456", "12345678", "123456789", "12345",
        "qwerty", "abc123", "password1", "1234567", "1234567890",
        "admin", "welcome", "monkey", "letmein", "master",
        NULL
    };
    
    for (int i = 0; common_passwords[i] != NULL; i++) {
        if (strcmp(password, common_passwords[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

int check_password_breach(const char *password, Database *db) {
    // Implémentation simplifiée - en production, utiliser l'API Have I Been Pwned
    // Pour l'instant, on vérifie juste les patterns évidents
    
    // Vérifier les séquences
    if (strstr(password, "123") != NULL || strstr(password, "abc") != NULL) {
        return 1;
    }
    
    // Vérifier les répétitions
    int repeats = 0;
    for (size_t i = 1; i < strlen(password); i++) {
        if (password[i] == password[i-1]) repeats++;
    }
    if (repeats > strlen(password) / 2) {
        return 1;
    }
    
    return 0;
}

// ==================== GÉNÉRATEUR DE MOT DE PASSE SÉCURISÉ ====================

void generate_secure_passphrase(char *output, int length) {
    const char *words[] = {
        "correct", "battery", "horse", "staple", "memory", "system",
        "security", "vault", "encrypt", "decrypt", "access", "control",
        "random", "secure", "password", "manager", "protection", "privacy",
        NULL
    };
    
    const char *separators = "-_=+!@#$%^&*";
    unsigned char random_bytes[32];
    randombytes_buf(random_bytes, sizeof(random_bytes));
    
    int word_count = 0;
    while (words[word_count] != NULL) word_count++;
    
    output[0] = '\0';
    
    for (int i = 0; i < 4 && strlen(output) < (size_t)length - 10; i++) {
        if (i > 0) {
            char sep[2] = {separators[random_bytes[i] % strlen(separators)], '\0'};
            strcat(output, sep);
        }
        
        const char *word = words[random_bytes[i + 4] % word_count];
        strcat(output, word);
    }
    
    // Ajouter chiffres et symboles
    char suffix[16];
    snprintf(suffix, sizeof(suffix), "%c%02d%c", 
             separators[random_bytes[8] % strlen(separators)],
             random_bytes[9] % 100,
             separators[random_bytes[10] % strlen(separators)]);
    
    strcat(output, suffix);
}

// ==================== AUDIT DE SÉCURITÉ ====================

typedef struct {
    char timestamp[32];
    char action[64];
    char severity[16]; // "low", "medium", "high", "critical"
    char description[256];
    char ip_address[64];
} SecurityEvent;

int db_log_security_event(Database *db, const char *action, const char *severity, 
                         const char *description, const char *ip) {
    const char *sql = "INSERT INTO security_events (action, severity, description, ip_address) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, action, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, severity, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, ip, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}