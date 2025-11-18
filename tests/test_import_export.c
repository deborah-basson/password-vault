#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/database.h"
#include "../include/crypto.h"
#include "../include/import_export.h"

void setup_import_export_test(void) {
    crypto_init();
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
    unlink("test_export.enc");
}

void cleanup_import_export_test(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
    unlink("test_export.enc");
}

Test(import_export, export_to_memory, .init = setup_import_export_test, .fini = cleanup_import_export_test) {
    printf("[TEST IMPORT/EXPORT] Export vers mémoire\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    
    db_add_password(&db, "github.com", "user1", "encrypted_pass1", "note1");
    db_add_password(&db, "google.com", "user2", "encrypted_pass2", "note2");
    db_add_password(&db, "facebook.com", "user3", "encrypted_pass3", "note3");
    
    char *exported_data = NULL;
    size_t exported_len = 0;
    
    int result = export_passwords_encrypted(&db, "ExportPassword123!", &exported_data, &exported_len);
    
    cr_assert_eq(result, 1, "Export devrait réussir");
    cr_assert_not_null(exported_data, "Données exportées ne doivent pas être NULL");
    cr_assert_gt(exported_len, 0, "Taille exportée doit être > 0");
    
    secure_zero_memory(exported_data, exported_len);
    free(exported_data);
    db_close(&db);
}

Test(import_export, export_and_import_memory, .init = setup_import_export_test, .fini = cleanup_import_export_test) {
    printf("[TEST IMPORT/EXPORT] Export puis import depuis mémoire\n");
    fflush(stdout);
    
    Database db1, db2;
    db_init(&db1);
    
    db_add_password(&db1, "site1.com", "user1", "pass1", "notes1");
    db_add_password(&db1, "site2.com", "user2", "pass2", "notes2");
    
    char *exported_data = NULL;
    size_t exported_len = 0;
    
    int export_result = export_passwords_encrypted(&db1, "TestPass123!", &exported_data, &exported_len);
    cr_assert_eq(export_result, 1, "Export devrait réussir");
    
    db_close(&db1);
    
    unlink("vault.db");
    db_init(&db2);
    
    int imported = import_passwords_encrypted(&db2, exported_data, exported_len, "TestPass123!");
    cr_assert_eq(imported, 2, "Devrait importer 2 mots de passe");
    
    PasswordEntry *entries = NULL;
    int count = 0;
    db_get_all_passwords(&db2, &entries, &count);
    
    cr_assert_eq(count, 2, "Devrait avoir 2 mots de passe");
    cr_assert_str_eq(entries[0].website, "site1.com");
    cr_assert_str_eq(entries[1].website, "site2.com");
    
    free(entries);
    secure_zero_memory(exported_data, exported_len);
    free(exported_data);
    db_close(&db2);
}

Test(import_export, export_and_import_file, .init = setup_import_export_test, .fini = cleanup_import_export_test) {
    printf("[TEST IMPORT/EXPORT] Export puis import depuis fichier\n");
    fflush(stdout);
    
    Database db1, db2;
    db_init(&db1);
    
    db_add_password(&db1, "website1.com", "username1", "encpass1", "note1");
    db_add_password(&db1, "website2.com", "username2", "encpass2", "note2");
    db_add_password(&db1, "website3.com", "username3", "encpass3", "note3");
    
    int export_result = export_vault_to_file(&db1, "test_export.enc", "FilePassword456!");
    cr_assert_eq(export_result, 1, "Export vers fichier devrait réussir");
    
    db_close(&db1);
    
    unlink("vault.db");
    db_init(&db2);
    
    int imported = import_vault_from_file(&db2, "test_export.enc", "FilePassword456!");
    cr_assert_eq(imported, 3, "Devrait importer 3 mots de passe");
    
    PasswordEntry *entries = NULL;
    int count = 0;
    db_get_all_passwords(&db2, &entries, &count);
    
    cr_assert_eq(count, 3, "Devrait avoir 3 mots de passe");
    
    free(entries);
    db_close(&db2);
}

Test(import_export, import_wrong_password, .init = setup_import_export_test, .fini = cleanup_import_export_test) {
    printf("[TEST IMPORT/EXPORT] Import avec mauvais mot de passe\n");
    fflush(stdout);
    
    Database db1, db2;
    db_init(&db1);
    
    db_add_password(&db1, "test.com", "user", "pass", "note");
    
    char *exported_data = NULL;
    size_t exported_len = 0;
    
    export_passwords_encrypted(&db1, "CorrectPass!", &exported_data, &exported_len);
    db_close(&db1);
    
    db_init(&db2);
    
    int imported = import_passwords_encrypted(&db2, exported_data, exported_len, "WrongPass!");
    cr_assert_eq(imported, 0, "Import avec mauvais mot de passe devrait échouer");
    
    secure_zero_memory(exported_data, exported_len);
    free(exported_data);
    db_close(&db2);
}

Test(import_export, export_empty_vault, .init = setup_import_export_test, .fini = cleanup_import_export_test) {
    printf("[TEST IMPORT/EXPORT] Export d'un coffre vide\n");
    fflush(stdout);
    
    Database db;
    db_init(&db);
    
    char *exported_data = NULL;
    size_t exported_len = 0;
    
    int result = export_passwords_encrypted(&db, "Password123!", &exported_data, &exported_len);
    cr_assert_eq(result, 1, "Export d'un coffre vide devrait réussir");
    cr_assert_not_null(exported_data);
    
    secure_zero_memory(exported_data, exported_len);
    free(exported_data);
    db_close(&db);
}

Test(import_export, import_preserves_encryption, .init = setup_import_export_test, .fini = cleanup_import_export_test) {
    printf("[TEST IMPORT/EXPORT] Import préserve le chiffrement\n");
    fflush(stdout);
    
    Database db1, db2;
    db_init(&db1);
    
    const char *original_encrypted = "AES_ENCRYPTED_DATA_HERE";
    db_add_password(&db1, "secure.com", "user", original_encrypted, "");
    
    char *exported_data = NULL;
    size_t exported_len = 0;
    
    export_passwords_encrypted(&db1, "ExportPass!", &exported_data, &exported_len);
    db_close(&db1);
    
    unlink("vault.db");
    db_init(&db2);
    
    import_passwords_encrypted(&db2, exported_data, exported_len, "ExportPass!");
    
    PasswordEntry entry;
    db_get_password_by_id(&db2, 1, &entry);
    
    cr_assert_str_eq(entry.encrypted_password, original_encrypted, 
                     "Le mot de passe chiffré devrait être préservé");
    
    secure_zero_memory(exported_data, exported_len);
    free(exported_data);
    db_close(&db2);
}

Test(import_export, large_vault_export_import, .init = setup_import_export_test, .fini = cleanup_import_export_test) {
    printf("[TEST IMPORT/EXPORT] Export/Import d'un grand coffre\n");
    fflush(stdout);
    
    Database db1, db2;
    db_init(&db1);
    
    for (int i = 0; i < 50; i++) {
        char website[256], username[256], password[256], notes[256];
        snprintf(website, sizeof(website), "site%d.com", i);
        snprintf(username, sizeof(username), "user%d", i);
        snprintf(password, sizeof(password), "encrypted_pass%d", i);
        snprintf(notes, sizeof(notes), "note%d", i);
        
        db_add_password(&db1, website, username, password, notes);
    }
    
    char *exported_data = NULL;
    size_t exported_len = 0;
    
    int export_result = export_passwords_encrypted(&db1, "LargeVaultPass!", &exported_data, &exported_len);
    cr_assert_eq(export_result, 1);
    
    db_close(&db1);
    
    unlink("vault.db");
    db_init(&db2);
    
    int imported = import_passwords_encrypted(&db2, exported_data, exported_len, "LargeVaultPass!");
    cr_assert_eq(imported, 50, "Devrait importer 50 mots de passe");
    
    secure_zero_memory(exported_data, exported_len);
    free(exported_data);
    db_close(&db2);
}