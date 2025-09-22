#ifndef PASSWORD_MANAGER_H
#define PASSWORD_MANAGER_H

#define MAX_STORAGE 50
#define MAX_LENGTH 100
#define MASTER_KEY_MAX_LENGTH 256
#define SALT_LENGTH 32

typedef struct {
    char website[MAX_LENGTH];
    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
    char notes[MAX_LENGTH];
    int category; // 0=Personnel, 1=Travail, 2=Finance, etc.
} PasswordEntry;

typedef struct {
    PasswordEntry entries[MAX_STORAGE];
    char master_key[MASTER_KEY_MAX_LENGTH];
    char salt[SALT_LENGTH];
    int entry_count;
    char data_file[256];
} PasswordManager;

// Fonctions de gestion
int init_password_manager(PasswordManager *manager, const char *filename, int max_entries);
int add_password_entry(PasswordManager *manager, const PasswordEntry *entry);
int delete_password_entry(PasswordManager *manager, int index);
int edit_password_entry(PasswordManager *manager, int index, const PasswordEntry *new_entry);
void cleanup_password_manager(PasswordManager *manager);
int verify_master_password(const PasswordManager *manager, const char *password);

#endif