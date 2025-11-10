#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/database.h"
#include "../include/crypto.h"

// Renommer pour ce fichier
void setup_security_test_enhanced(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
}

void cleanup_security_test_enhanced(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
}

Test(security, recovery_code_lockout, .init = setup_security_test_enhanced, .fini = cleanup_security_test_enhanced) {
    printf("[TEST SECURITE] Verrouillage après tentatives échouées\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    
    // Créer un utilisateur avec questions
    char salt[SALT_LENGTH];
    char hash[HASH_LENGTH];
    generate_salt(salt, sizeof(salt));
    hash_password("testpass", salt, hash, sizeof(hash));
    
    int created = db_create_user_with_questions(&db, hash, salt,
                                               "Question 1", "Answer1",
                                               "Question 2", "Answer2", 
                                               "Question 3", "Answer3",
                                               "123456");
    cr_assert_eq(created, 1, "Création devrait réussir");
    
    // Tentatives échouées
    cr_assert_eq(db_verify_recovery_code(&db, "wrong1"), 0);
    cr_assert_eq(db_verify_recovery_code(&db, "wrong2"), 0);
    cr_assert_eq(db_verify_recovery_code(&db, "wrong3"), -1); // Devrait être verrouillé
    
    db_close(&db);
}