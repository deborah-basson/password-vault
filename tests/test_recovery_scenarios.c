#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/database.h"
#include "../include/crypto.h"

void setup_recovery_test(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
}

void cleanup_recovery_test(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
}

Test(recovery, multiple_recovery_methods, .init = setup_recovery_test, .fini = cleanup_recovery_test) {
    printf("[TEST RÉCUPÉRATION] Méthodes multiples de récupération\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    
    // Créer utilisateur avec questions
    char salt[SALT_LENGTH];
    char hash[HASH_LENGTH];
    generate_salt(salt, sizeof(salt));
    hash_password("masterpass", salt, hash, sizeof(hash));
    
    int created = db_create_user_with_questions(&db, hash, salt,
                                               "Q1", "A1",
                                               "Q2", "A2", 
                                               "Q3", "A3",
                                               "RECOV123");
    cr_assert_eq(created, 1);
    
    // Test récupération par questions
    int by_questions = db_verify_security_answers(&db, "A1", "A2", "A3");
    cr_assert_eq(by_questions, 1, "Récupération par questions devrait fonctionner");
    
    // Test récupération par code
    int by_code = db_verify_recovery_code(&db, "RECOV123");
    cr_assert_eq(by_code, 1, "Récupération par code devrait fonctionner");
    
    // Test échec récupération
    int wrong_answers = db_verify_security_answers(&db, "Wrong1", "Wrong2", "Wrong3");
    cr_assert_eq(wrong_answers, 0, "Mauvaises réponses devraient échouer");
    
    int wrong_code = db_verify_recovery_code(&db, "WRONGCODE");
    cr_assert_eq(wrong_code, 0, "Mauvais code devrait échouer");
    
    db_close(&db);
}

Test(recovery, account_lockout_mechanism, .init = setup_recovery_test, .fini = cleanup_recovery_test) {
    printf("[TEST RÉCUPÉRATION] Mécanisme de verrouillage\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    
    char salt[SALT_LENGTH];
    char hash[HASH_LENGTH];
    generate_salt(salt, sizeof(salt));
    hash_password("testpass", salt, hash, sizeof(hash));
    
    db_create_user_with_questions(&db, hash, salt,
                                 "Q1", "A1", "Q2", "A2", "Q3", "A3", "CODE123");
    
    // Tentatives échouées successives
    cr_assert_eq(db_verify_recovery_code(&db, "WRONG1"), 0);
    cr_assert_eq(db_verify_recovery_code(&db, "WRONG2"), 0);
    cr_assert_eq(db_verify_recovery_code(&db, "WRONG3"), -1); // Devrait être verrouillé
    
    // Même verrouillé, le bon code devrait être rejeté
    cr_assert_eq(db_verify_recovery_code(&db, "CODE123"), -1);
    
    db_close(&db);
}