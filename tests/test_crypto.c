#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sodium.h>
#include "../include/crypto.h"
#include "../include/config.h"

Test(crypto, hash_and_verify) {
    printf("[TEST] Hachage et vérification de mot de passe\n");
    fflush(stdout);
    
    char salt[SALT_LENGTH];
    char hash[HASH_LENGTH];
    generate_salt(salt, sizeof(salt));
    int r1 = hash_password("securepass", salt, hash, sizeof(hash));
    cr_assert_eq(r1, 1);
    int r2 = verify_password("securepass", salt, hash);
    cr_assert_eq(r2, 1);
}

Test(crypto, encrypt_and_decrypt) {
    printf("[TEST] Chiffrement et déchiffrement de données\n");
    fflush(stdout);
    
    const char *plaintext = "hello world";
    const char *key = "verysecurekeyverysecurekey1234";
    char ciphertext[256];
    size_t cipher_len = sizeof(ciphertext);
    char decrypted[256];
    int enc = encrypt_data(plaintext, key, ciphertext, &cipher_len);
    cr_assert_eq(enc, 1);
    int dec = decrypt_data(ciphertext, cipher_len, key, decrypted);
    cr_assert_eq(dec, 1);
    cr_assert_str_eq(decrypted, plaintext);
}

Test(crypto, totp_generation) {
    printf("[TEST] Génération de code TOTP\n");
    fflush(stdout);
    
    char secret[TOTP_SECRET_LENGTH];
    generate_totp_secret(secret, sizeof(secret));
    int code = generate_totp_code(secret, 0);
    cr_assert(code >= 0);
    cr_assert(code < 1000000);
}

Test(crypto, password_strength_weak) {
    printf("[TEST] Validation de mot de passe faible\n");
    fflush(stdout);
    
    cr_assert_eq(validate_password_strength("short"), 0);
    cr_assert_eq(validate_password_strength("alllowercase"), 0);
    cr_assert_eq(validate_password_strength("NoDigits!@#"), 0);
}

Test(crypto, password_strength_strong) {
    printf("[TEST] Validation de mot de passe fort\n");
    fflush(stdout);
    
    cr_assert_eq(validate_password_strength("MyP@ssw0rd123"), 1);
    cr_assert_eq(validate_password_strength("C0mpl3x!Pass"), 1);
}

Test(crypto, password_verification_wrong) {
    printf("[TEST] Vérification de mot de passe incorrect\n");
    fflush(stdout);
    
    char salt[SALT_LENGTH];
    char hash[HASH_LENGTH];
    generate_salt(salt, sizeof(salt));
    hash_password("correctpass", salt, hash, sizeof(hash));
    
    int result = verify_password("wrongpass", salt, hash);
    cr_assert_eq(result, 0);
}

Test(crypto, generate_secure_password) {
    printf("[TEST] Génération de mot de passe sécurisé\n");
    fflush(stdout);
    
    char password[256];
    memset(password, 0, sizeof(password));
    generate_secure_password(password, 20);
    
    cr_assert(strlen(password) >= 12);
    cr_assert_eq(validate_password_strength(password), 1);
}

Test(crypto, salt_uniqueness) {
    printf("[TEST] Unicité des sels générés\n");
    fflush(stdout);
    
    char salt1[SALT_LENGTH];
    char salt2[SALT_LENGTH];
    
    generate_salt(salt1, sizeof(salt1));
    generate_salt(salt2, sizeof(salt2));
    
    cr_assert_neq(strcmp(salt1, salt2), 0);
}

Test(crypto, totp_verification) {
    printf("[TEST] Vérification de code TOTP\n");
    fflush(stdout);
    
    char secret[TOTP_SECRET_LENGTH];
    generate_totp_secret(secret, sizeof(secret));
    
    long current_time = time(NULL) / 30;
    int code = generate_totp_code(secret, current_time);
    
    int valid = verify_totp_code(secret, code, 1);
    cr_assert_eq(valid, 1);
    
    int invalid = verify_totp_code(secret, 999999, 0);
    cr_assert_eq(invalid, 0);
}

Test(crypto, sanitize_html_xss) {
    printf("[TEST] Sanitization HTML contre XSS\n");
    fflush(stdout);
    
    char *sanitized = sanitize_html("<script>alert('xss')</script>");
    cr_assert_not_null(sanitized);
    cr_assert_null(strstr(sanitized, "<script>"));
    
    if (sanitized) {
        sodium_free(sanitized);
    }
}

Test(crypto, validate_input_limits) {
    printf("[TEST] Validation des limites d'entrée\n");
    fflush(stdout);
    
    cr_assert_eq(validate_input("valid", 10), 1);
    cr_assert_eq(validate_input("", 10), 0);
    cr_assert_eq(validate_input(NULL, 10), 0);
    cr_assert_eq(validate_input("toolongstring", 5), 0);
}

Test(crypto, encryption_wrong_key) {
    printf("[TEST] Déchiffrement avec mauvaise clé\n");
    fflush(stdout);
    
    const char *plaintext = "secret data";
    const char *key1 = "correctkey1234567890123456789012";
    const char *key2 = "wrongkey123456789012345678901234";
    
    char ciphertext[256];
    size_t cipher_len = sizeof(ciphertext);
    
    encrypt_data(plaintext, key1, ciphertext, &cipher_len);
    
    char decrypted[256];
    int result = decrypt_data(ciphertext, cipher_len, key2, decrypted);
    
    cr_assert_eq(result, 0);
}
