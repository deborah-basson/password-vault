#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stdlib.h>

#define DB_FILE "vault.db"

typedef struct {
    sqlite3 *db;
} Database;

typedef struct {
    int id;
    char website[256];
    char username[256];
    char encrypted_password[512];
    char notes[512];
    char created_at[32];
    char updated_at[32];
} PasswordEntry;

typedef struct {
    int id;
    char master_hash[256];
    char salt[64];
    char totp_secret[64];
    int is_2fa_enabled;
    char created_at[32];
} UserAccount;

// ==================== STRUCTURES POUR LA SÉCURITÉ ====================

// Structure pour l'historique des mots de passe
typedef struct {
    int id;
    int password_id;
    char encrypted_password[512];
    char changed_at[32];
} PasswordHistory;

// Structure pour les questions de sécurité
typedef struct {
    int id;
    char question1[256];
    char answer1[256];
    char question2[256];
    char answer2[256];
    char question3[256];
    char answer3[256];
    char recovery_code[16];
    int recovery_attempts;
    char locked_until[32];
} SecurityQuestions;

// Structure pour les événements de sécurité
typedef struct {
    char timestamp[32];
    char action[64];
    char severity[16]; // "low", "medium", "high", "critical"
    char description[256];
    char ip_address[64];
} SecurityEvent;

// ==================== FONCTIONS PRINCIPALES ====================

int db_init(Database *db);
void db_close(Database *db);

int db_create_user(Database *db, const char *master_hash, const char *salt, const char *totp_secret);
int db_get_user(Database *db, UserAccount *user);
int db_update_2fa(Database *db, const char *totp_secret, int enabled);

// ==================== FONCTIONS QUESTIONS DE SÉCURITÉ ====================

int db_create_user_with_questions(Database *db, const char *master_hash, const char *salt,
                                 const char *question1, const char *answer1,
                                 const char *question2, const char *answer2,
                                 const char *question3, const char *answer3,
                                 const char *recovery_code);
int db_get_security_questions(Database *db, SecurityQuestions *questions);
int db_verify_security_answers(Database *db, const char *answer1, const char *answer2, const char *answer3);
int db_verify_recovery_code(Database *db, const char *code);
int db_increment_recovery_attempts(Database *db);
int db_reset_recovery_attempts(Database *db);
int db_lock_recovery_until(Database *db, const char *until_time);

// ==================== FONCTIONS MOTS DE PASSE ====================

int db_add_password(Database *db, const char *website, const char *username, 
                    const char *encrypted_pass, const char *notes);
int db_get_all_passwords(Database *db, PasswordEntry **entries, int *count);
int db_get_password_by_id(Database *db, int id, PasswordEntry *entry);
int db_update_password(Database *db, int id, const char *encrypted_pass);
int db_delete_password(Database *db, int id);

int db_add_password_encrypted(Database *db, const char *website, const char *username,
                               const char *plaintext_password, const char *notes, 
                               const char *master_key);
int db_get_password_decrypted(Database *db, int id, PasswordEntry *entry,
                              const char *master_key);

// ==================== FONCTIONS SÉCURITÉ ET AUDIT ====================

int db_log_attempt(Database *db, const char *action, int success, const char *ip);
int db_get_failed_attempts(Database *db, const char *ip, int minutes);

// ==================== FONCTIONS SECURITY_ENHANCEMENTS ====================

int db_add_password_history(Database *db, int password_id, const char *encrypted_password);
int db_get_password_history(Database *db, int password_id, PasswordHistory **history, int *count);
int is_common_password(const char *password);
int check_password_breach(const char *password);
void generate_secure_passphrase(char *output, int length);
int db_log_security_event(Database *db, const char *action, const char *severity, 
                         const char *description, const char *ip);
int validate_enhanced_password(const char *password);

// ==================== FONCTIONS UTILITAIRES ====================

int db_load_dev_data(Database *db);
int db_execute_migration(Database *db, const char *migration_file);

#endif