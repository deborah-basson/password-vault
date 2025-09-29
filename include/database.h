#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

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

// Initialisation
int db_init(Database *db);
void db_close(Database *db);

// Gestion utilisateur
int db_create_user(Database *db, const char *master_hash, const char *salt, const char *totp_secret);
int db_get_user(Database *db, UserAccount *user);
int db_update_2fa(Database *db, const char *totp_secret, int enabled);

// Gestion mots de passe
int db_add_password(Database *db, const char *website, const char *username, 
                    const char *encrypted_pass, const char *notes);
int db_get_all_passwords(Database *db, PasswordEntry **entries, int *count);
int db_get_password_by_id(Database *db, int id, PasswordEntry *entry);
int db_update_password(Database *db, int id, const char *encrypted_pass);
int db_delete_password(Database *db, int id);

// Sécurité
int db_log_attempt(Database *db, const char *action, int success, const char *ip);
int db_get_failed_attempts(Database *db, const char *ip, int minutes);

// Ajouter cette fonction après db_close:
int db_load_dev_data(Database *db);
int db_execute_migration(Database *db, const char *migration_file);

#endif