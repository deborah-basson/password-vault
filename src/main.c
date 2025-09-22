#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/password_manager.h"
#include "../include/file_operations.h"
#include "security.h"
#include "ui.h"

int main() {
    PasswordManager manager;
    
    printf("Initialisation du gestionnaire de mots de passe...\n");
    
    // Initialisation du gestionnaire
    if (!init_password_manager(&manager, "data.dat", MAX_STORAGE)) {
        printf("Erreur lors de l'initialisation du gestionnaire de mots de passe.\n");
        return 1;
    }
    
    // Démarrage de l'interface utilisateur
    start_ui(&manager);
    
    // Nettoyage final
    cleanup_password_manager(&manager);
    
    printf("Au revoir!\n");
    return 0;
}