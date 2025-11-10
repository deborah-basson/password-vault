#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "config.h"

typedef struct {
    sqlite3 *db;
} Database;

typedef struct {
    int id;
    char master_hash[HASH_LENGTH];
    char salt[SALT_LENGTH];
    char totp_secret[TOTP_SECRET_LENGTH];
    int is_2fa_enabled;
    char created_at[64];
} UserAccount;

typedef struct {
    int id;
    int user_id;
    char website[256];
    char username[256];
    char encrypted_password[1024];
    char notes[1024];
    char created_at[64];
    char updated_at[64];
} PasswordEntry;

typedef struct {
    int id;
    int password_id;
    char encrypted_password[1024];
    char website[256];
    char username[256];
    char changed_at[64];
} PasswordHistoryEntry;

typedef struct {
    int id;
    int user_id;
    int require_2fa;
    int password_min_length;
    int require_mixed_case;
    int require_numbers;
    int require_symbols;
    int password_history_size;
    int max_login_attempts;
    int session_timeout;
    int auto_lock_timeout;
    char created_at[64];
} UserSecurityPolicy;

typedef struct {
    int id;
    char question1[256];
    char question2[256];
    char question3[256];
    int recovery_attempts;
    char locked_until[64];
} SecurityQuestions;

// Fonctions de base de données
int db_init(Database *db);
int db_load_dev_data(Database *db);
void db_close(Database *db);

// Gestion des utilisateurs
int db_create_user(Database *db, const char *master_hash, const char *salt, const char *totp_secret);
int db_get_user(Database *db, UserAccount *user);
int db_update_2fa(Database *db, const char *totp_secret, int enabled);

// Gestion des mots de passe
int db_add_password(Database *db, const char *website, const char *username, 
                    const char *encrypted_pass, const char *notes);
int db_get_all_passwords(Database *db, PasswordEntry **entries, int *count);
int db_get_password_by_id(Database *db, int id, PasswordEntry *entry);
int db_update_password(Database *db, int id, const char *encrypted_pass);
int db_delete_password(Database *db, int id);

// Chiffrement/déchiffrement
int db_add_password_encrypted(Database *db, const char *website, const char *username,
                               const char *plaintext_password, const char *notes, 
                               const char *master_key);
int db_get_password_decrypted(Database *db, int id, PasswordEntry *entry,
                              const char *master_key);

// Historique des mots de passe
int db_get_password_history(Database *db, int password_id, PasswordHistoryEntry **entries, int *count);
int db_check_password_reuse(Database *db, int password_id, const char *new_encrypted_password, int user_id);
int db_update_password_with_history(Database *db, int password_id, const char *new_encrypted_password, const char *master_key);

// Politiques de sécurité
int db_create_user_security_policy(Database *db, int user_id);
int db_get_user_security_policy(Database *db, int user_id, UserSecurityPolicy *policy);

// Journalisation et sécurité
int db_log_attempt(Database *db, const char *action, int success, const char *ip);
int db_get_failed_attempts(Database *db, const char *ip, int minutes);
int db_get_suspicious_ips(Database *db, char ***ips, int *count);

// Questions de sécurité
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

#endif