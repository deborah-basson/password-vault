#include "../include/password_manager.h"
#include "../include/security.h"
#include <string.h>
#include <stdlib.h>

int init_password_manager(PasswordManager *manager, const char *filename, int max_entries) {
    if (!manager || !filename) return 0;
    
    memset(manager, 0, sizeof(PasswordManager));
    strncpy(manager->data_file, filename, sizeof(manager->data_file) - 1);
    manager->entry_count = 0;
    
    // Générer un salt unique si premier démarrage
    generate_salt(manager->salt, SALT_LENGTH);
    
    // Utiliser max_entries pour éviter l'avertissement
    (void)max_entries; // Marquer comme utilisé
    
    return 1;
}

int add_password_entry(PasswordManager *manager, const PasswordEntry *entry) {
    if (!manager || !entry || manager->entry_count >= MAX_STORAGE) {
        return 0;
    }
    
    manager->entries[manager->entry_count] = *entry;
    manager->entry_count++;
    return 1;
}

int delete_password_entry(PasswordManager *manager, int index) {
    if (!manager || index < 0 || index >= manager->entry_count) {
        return 0;
    }
    
    // Décaler les entrées
    for (int i = index; i < manager->entry_count - 1; i++) {
        manager->entries[i] = manager->entries[i + 1];
    }
    
    manager->entry_count--;
    return 1;
}

void cleanup_password_manager(PasswordManager *manager) {
    if (!manager) return;
    
    // Effacer les données sensibles de la mémoire
    memset(manager->master_key, 0, sizeof(manager->master_key));
    memset(manager->salt, 0, sizeof(manager->salt));
    
    // Effacer tous les mots de passe en mémoire
    for (int i = 0; i < manager->entry_count; i++) {
        memset(&manager->entries[i], 0, sizeof(PasswordEntry));
    }
    
    manager->entry_count = 0;
}


int verify_master_password(const PasswordManager *manager, const char *password) {
    if (!manager || !password) return 0;
    
    // Ici vous devriez utiliser un hash sécurisé comme bcrypt ou argon2
    // Pour l'exemple, on fait une simple comparaison
    return strcmp(manager->master_key, password) == 0;
}