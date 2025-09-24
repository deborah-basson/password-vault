#include "../include/ui.h"
#include "../include/file_operations.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/password_manager.h"

#ifdef _WIN32
#else
#include <unistd.h>
#include <termios.h>

// Implémentation simple de _getch() pour Linux
int _getch(void) {
    struct termios oldt, newt;
    int ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    return ch;
}
#endif

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    int ret = system("clear");
    (void)ret; // Ignorer le retour
#endif
}

void wait_key() {
    printf("\nAppuyez sur une touche pour continuer...");
#ifdef _WIN32
    _getch();
#else
    _getch(); // Utilise notre implémentation
#endif
    printf("\n");
}

void show_passwords(const PasswordManager *manager) {
    clear_screen();
    printf("=== MOTS DE PASSE ENREGISTRÉS ===\n\n");
    
    if (manager->entry_count == 0) {
        printf("Aucun mot de passe enregistré.\n");
        return;
    }
    
    for (int i = 0; i < manager->entry_count; i++) {
        printf("[%d] Site: %s | Utilisateur: %s | Mot de passe: ******\n", 
               i, manager->entries[i].website, manager->entries[i].username);
    }
}

void add_password_ui(PasswordManager *manager) {
    clear_screen();
    printf("=== AJOUTER UN MOT DE PASSE ===\n\n");
    
    PasswordEntry entry;
    memset(&entry, 0, sizeof(PasswordEntry));
    
    printf("Site web/service: ");
    if (fgets(entry.website, sizeof(entry.website), stdin) == NULL) {
        printf("Erreur de lecture.\n");
        return;
    }
    entry.website[strcspn(entry.website, "\n")] = 0;
    
    printf("Nom d'utilisateur: ");
    if (fgets(entry.username, sizeof(entry.username), stdin) == NULL) {
        printf("Erreur de lecture.\n");
        return;
    }
    entry.username[strcspn(entry.username, "\n")] = 0;
    
    printf("Mot de passe: ");
    if (fgets(entry.password, sizeof(entry.password), stdin) == NULL) {
        printf("Erreur de lecture.\n");
        return;
    }
    entry.password[strcspn(entry.password, "\n")] = 0;
    
    printf("Notes: ");
    if (fgets(entry.notes, sizeof(entry.notes), stdin) == NULL) {
        printf("Erreur de lecture.\n");
        return;
    }
    entry.notes[strcspn(entry.notes, "\n")] = 0;
    
    if (add_password_entry(manager, &entry)) {
        printf("\n✓ Mot de passe ajouté avec succès!\n");
        save_to_file(manager); // Sauvegarder après ajout
    } else {
        printf("\n✗ Erreur lors de l'ajout du mot de passe.\n");
    }
    
    wait_key();
}

// Fonction pour lire une ligne sécurisée
int read_line(char *buffer, int size) {
    if (fgets(buffer, size, stdin) == NULL) {
        return 0;
    }
    
    // Supprimer le saut de ligne
    buffer[strcspn(buffer, "\n")] = 0;
    return 1;
}

void start_ui(PasswordManager *manager) {
    int choice;
    
    // Vérifier si le coffre-fort existe en essayant de charger
    if (load_from_file(manager)) {
        printf("=== CHARGEMENT RÉUSSI ===\n");
    } else {
        printf("=== PREMIÈRE UTILISATION ===\n");
        printf("Créez un mot de passe : ");

        fgets(manager->master_key, MASTER_KEY_MAX_LENGTH , stdin);
        manager->master_key[strcspn(manager->master_key, "\n")] = 0;
        
        printf("✓ Mot de passe créé avec succès!\n\n");
        save_to_file(manager); // Sauvegarder le nouveau coffre
    }
    
    // Authentification
    char input_password[MASTER_KEY_MAX_LENGTH];
    int attempts = 0;
    int authenticated = 0;
    
    while (attempts < 3 && !authenticated) {
        printf("Mot de passe : ");
        
        if (!read_line(input_password, sizeof(input_password))) {
            printf("Erreur de lecture.\n");
            continue;
        }
        
        if (verify_master_password(manager, input_password)) {
            authenticated = 1;
            printf("✓ Authentification réussie!\n\n");
        } else {
            attempts++;
            printf("✗ Mot de passe incorrect. Tentatives restantes: %d\n\n", 3 - attempts);
        }
    }
    
    if (!authenticated) {
        printf("Trop de tentatives échouées. Fermeture du programme.\n");
        return;
    }
    
    // Menu principal
    do {
        clear_screen();
        printf("=== COFFRE-FORT DE MOTS DE PASSE ===\n\n");
        printf("1. Afficher les mots de passe\n");
        printf("2. Ajouter un mot de passe\n");
        printf("3. Modifier un mot de passe\n");
        printf("4. Supprimer un mot de passe\n");
        printf("5. Générer un mot de passe sécurisé\n");
        printf("6. Paramètres\n");
        printf("0. Quitter\n");
        printf("\nVotre choix: ");
        
        if (scanf("%d", &choice) != 1) {
            // Vider le buffer en cas d'erreur
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            choice = -1;
        }
        getchar(); // Vider le buffer
        
        switch (choice) {
            case 1:
                show_passwords(manager);
                wait_key();
                break;
            case 2:
                add_password_ui(manager);
                break;
            case 0:
                printf("Sauvegarde et fermeture...\n");
                save_to_file(manager);
                break;
            default:
                if (choice != -1) {
                    printf("Option non reconnue.\n");
                }
                wait_key();
        }
    } while (choice != 0);
}