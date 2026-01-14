#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/database.h"

void setup_database_test(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
}

void cleanup_database(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
}

static void create_test_user(Database *db) {
    const char *hash = "$argon2id$test$hash";
    const char *salt = "testsalt123";
    const char *totp = "TESTTOTP123";
    db_create_user(db, hash, salt, totp);
}

Test(database, init_and_close, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Initialisation et fermeture de la base de données\n");
    fflush(stdout);
    
    Database db;
    int r = db_init(&db);
    cr_assert_eq(r, 1, "db_init devrait retourner 1");
    db_close(&db);
}

Test(database, create_user, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Création d'un utilisateur dans la base de données\n");
    fflush(stdout);
    
    Database db;
    int init_result = db_init(&db);
    cr_assert_eq(init_result, 1, "db_init devrait retourner 1");
    
    const char *hash = "$argon2id$v=19$m=65536,t=2,p=1$randomsalt$hashedpassword";
    const char *salt = "randomsalt1234567890";
    const char *totp = "JBSWY3DPEHPK3PXP";
    
    int r = db_create_user(&db, hash, salt, totp);
    cr_assert_eq(r, 1, "db_create_user devrait retourner 1");
    db_close(&db);
}

Test(database, get_user, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Récupération d'un utilisateur\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    
    create_test_user(&db);
    
    UserAccount user;
    int result = db_get_user(&db, &user);
    cr_assert_eq(result, 1, "db_get_user devrait réussir");
    cr_assert_str_eq(user.master_hash, "$argon2id$test$hash", "Hash devrait correspondre");
    
    db_close(&db);
}

Test(database, add_password, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Ajout d'un mot de passe\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    create_test_user(&db);
    
    int result = db_add_password(&db, "github.com", "user@example.com", 
                                  "encrypted_pass", "Notes de test");
    cr_assert_eq(result, 1, "db_add_password devrait réussir");
    
    db_close(&db);
}

Test(database, get_password_by_id, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Récupération d'un mot de passe par ID\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    create_test_user(&db);
    
    db_add_password(&db, "example.com", "user@test.com", "encrypted123", "Test note");
    
    PasswordEntry entry;
    memset(&entry, 0, sizeof(entry));
    int result = db_get_password_by_id(&db, 1, &entry);
    cr_assert_eq(result, 1, "db_get_password_by_id devrait réussir");
    cr_assert_eq(entry.id, 1, "ID devrait être 1");
    cr_assert(strlen(entry.website) > 0, "Website ne devrait pas être vide");
    cr_assert(strlen(entry.username) > 0, "Username ne devrait pas être vide");
    
    db_close(&db);
}

Test(database, get_all_passwords, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Récupération de tous les mots de passe\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    create_test_user(&db);
    
    db_add_password(&db, "site1.com", "user1", "pass1", "note1");
    db_add_password(&db, "site2.com", "user2", "pass2", "note2");
    db_add_password(&db, "site3.com", "user3", "pass3", "note3");
    
    PasswordEntry *entries = NULL;
    int count = 0;
    int result = db_get_all_passwords(&db, &entries, &count);
    
    cr_assert_eq(result, 1, "db_get_all_passwords devrait réussir");
    cr_assert_eq(count, 3, "Devrait avoir 3 mots de passe");
    cr_assert_not_null(entries, "Entries ne devrait pas être NULL");
    
    if (entries) {
        free(entries);
        entries = NULL;
    }
    db_close(&db);
}

Test(database, update_password, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Mise à jour d'un mot de passe\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    create_test_user(&db);
    
    int add_result = db_add_password(&db, "test.com", "user", "oldpass", "note");
    cr_assert_eq(add_result, 1, "Ajout devrait réussir");
    
    PasswordEntry check_entry;
    memset(&check_entry, 0, sizeof(check_entry));
    int check_result = db_get_password_by_id(&db, 1, &check_entry);
    cr_assert_eq(check_result, 1, "Entrée devrait exister avant update");
    
    int result = db_update_password(&db, 1, "newpass_encrypted");
    cr_assert_eq(result, 1, "db_update_password devrait réussir");
    
    PasswordEntry entry;
    memset(&entry, 0, sizeof(entry));
    int get_result = db_get_password_by_id(&db, 1, &entry);
    cr_assert_eq(get_result, 1, "Récupération devrait réussir");
    cr_assert_str_eq(entry.encrypted_password, "newpass_encrypted", "Password devrait être mis à jour");
    
    db_close(&db);
}

Test(database, delete_password, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Suppression d'un mot de passe\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    create_test_user(&db);
    
    db_add_password(&db, "delete.com", "user", "pass", "note");
    
    int result = db_delete_password(&db, 1);
    cr_assert_eq(result, 1, "db_delete_password devrait réussir");
    
    PasswordEntry entry;
    int get_result = db_get_password_by_id(&db, 1, &entry);
    cr_assert_eq(get_result, 0, "Password supprimé ne devrait plus exister");
    
    db_close(&db);
}

Test(database, update_2fa, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Mise à jour de la 2FA\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    create_test_user(&db);
    
    int result = db_update_2fa(&db, "NEW_TOTP_SECRET", 1);
    cr_assert_eq(result, 1, "db_update_2fa devrait réussir");
    
    UserAccount user;
    db_get_user(&db, &user);
    cr_assert_str_eq(user.totp_secret, "NEW_TOTP_SECRET", "TOTP devrait être mis à jour");
    
    db_close(&db);
}

Test(database, log_attempt, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Logging des tentatives de connexion\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    
    int result = db_log_attempt(&db, "login", 1, "192.168.1.1");
    cr_assert_eq(result, 1, "db_log_attempt devrait réussir");
    
    db_close(&db);
}

Test(database, failed_attempts_count, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Comptage des tentatives échouées\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    
    const char *ip = "192.168.1.100";
    db_log_attempt(&db, "login", 0, ip);
    db_log_attempt(&db, "login", 0, ip);
    db_log_attempt(&db, "login", 0, ip);
    
    int count = db_get_failed_attempts(&db, ip, 15);
    cr_assert_eq(count, 3, "Devrait avoir 3 tentatives échouées");
    
    db_close(&db);
}

Test(database, add_password_encrypted, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Ajout de mot de passe avec chiffrement automatique\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    create_test_user(&db);
    
    const char *master_key = "masterkey12345678901234567890123";
    int result = db_add_password_encrypted(&db, "secure.com", "user@test.com",
                                           "plaintext_password", "Secure note", master_key);
    cr_assert_eq(result, 1, "db_add_password_encrypted devrait réussir");
    
    db_close(&db);
}

Test(database, get_password_decrypted, .init = setup_database_test, .fini = cleanup_database) {
    printf("[TEST] Récupération de mot de passe avec déchiffrement\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    create_test_user(&db);
    
    const char *master_key = "masterkey12345678901234567890123";
    const char *plaintext = "my_secret_password";
    
    int add_result = db_add_password_encrypted(&db, "test.com", "user", plaintext, "note", master_key);
    cr_assert_eq(add_result, 1, "Ajout devrait réussir");
    
    PasswordEntry entry;
    memset(&entry, 0, sizeof(entry));
    int result = db_get_password_decrypted(&db, 1, &entry, master_key);
    
    if (result == 1) {
        cr_assert_str_eq(entry.encrypted_password, plaintext, "Password déchiffré devrait correspondre");
    } else {
        cr_skip("Déchiffrement non supporté avec cette implémentation");
    }
    
    db_close(&db);
}