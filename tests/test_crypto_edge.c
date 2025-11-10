#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <sodium.h>
#include "../include/crypto.h"

Test(crypto_edge, empty_and_null_inputs) {
    printf("[TEST CRYPTO LIMITES] Entrées vides et NULL\n");
    fflush(stdout);
    
    char output[256];
    size_t out_len = sizeof(output);
    
    // Test avec entrées NULL
    cr_assert_eq(encrypt_data(NULL, "key", output, &out_len), 0);
    cr_assert_eq(decrypt_data(NULL, 10, "key", output), 0);
    cr_assert_eq(hash_password(NULL, "salt", output, sizeof(output)), 0);
    
    // Test avec clés vides
    cr_assert_eq(encrypt_data("data", "", output, &out_len), 0);
    cr_assert_eq(decrypt_data("data", 4, "", output), 0);
    
    // Test avec buffers trop petits
    char small_buf[5];
    size_t small_len = sizeof(small_buf);
    cr_assert_eq(encrypt_data("long data here", "key", small_buf, &small_len), 0);
}

Test(crypto_edge, key_derivation_consistency) {
    printf("[TEST CRYPTO LIMITES] Consistance dérivation de clés\n");
    fflush(stdout);
    
    const char *master_key = "MyMasterPassword123!";
    char salt1[SALT_LENGTH], salt2[SALT_LENGTH];
    char hash1[HASH_LENGTH], hash2[HASH_LENGTH];
    
    // Même mot de passe, sels différents -> hash différents
    generate_salt(salt1, sizeof(salt1));
    generate_salt(salt2, sizeof(salt2));
    
    int result1 = hash_password(master_key, salt1, hash1, sizeof(hash1));
    int result2 = hash_password(master_key, salt2, hash2, sizeof(hash2));
    
    cr_assert_eq(result1, 1, "Hash 1 devrait réussir");
    cr_assert_eq(result2, 1, "Hash 2 devrait réussir");
    cr_assert_neq(strcmp(hash1, hash2), 0, "Hash avec sels différents devraient être différents");
    
    // Vérification devrait fonctionner avec le bon sel
    cr_assert_eq(verify_password(master_key, salt1, hash1), 1);
    cr_assert_eq(verify_password(master_key, salt2, hash2), 1);
    cr_assert_eq(verify_password("wrong", salt1, hash1), 0);
}

Test(crypto_edge, encryption_integrity) {
    printf("[TEST CRYPTO LIMITES] Intégrité chiffrement\n");
    fflush(stdout);
    
    const char *plaintext = "Sensitive data: credit card 1234-5678-9012-3456";
    const char *key = "encryption_key_32_bytes_long_1234";
    char ciphertext[512];
    size_t cipher_len = sizeof(ciphertext);
    char decrypted[512];
    
    // Chiffrement normal
    int enc_result = encrypt_data(plaintext, key, ciphertext, &cipher_len);
    cr_assert_eq(enc_result, 1, "Chiffrement devrait réussir");
    cr_assert(cipher_len > 0, "Taille chiffrée devrait être > 0");
    
    // Déchiffrement avec bonne clé
    int dec_result = decrypt_data(ciphertext, cipher_len, key, decrypted);
    cr_assert_eq(dec_result, 1, "Déchiffrement devrait réussir");
    cr_assert_str_eq(decrypted, plaintext, "Texte déchiffré devrait correspondre");
    
    // Tentative de déchiffrement avec mauvaise clé
    char wrong_decrypted[512];
    int wrong_result = decrypt_data(ciphertext, cipher_len, "wrong_key_32_bytes_long_1234", wrong_decrypted);
    cr_assert_eq(wrong_result, 0, "Déchiffrement avec mauvaise clé devrait échouer");
}

Test(crypto_edge, salt_uniqueness) {
    printf("[TEST CRYPTO LIMITES] Unicité des sels\n");
    fflush(stdout);
    
    char salt1[SALT_LENGTH], salt2[SALT_LENGTH], salt3[SALT_LENGTH];
    
    generate_salt(salt1, sizeof(salt1));
    generate_salt(salt2, sizeof(salt2));
    generate_salt(salt3, sizeof(salt3));
    
    // Les sels devraient tous être différents
    cr_assert_neq(strcmp(salt1, salt2), 0);
    cr_assert_neq(strcmp(salt1, salt3), 0);
    cr_assert_neq(strcmp(salt2, salt3), 0);
    
    // Les sels devraient avoir la bonne longueur
    cr_assert_eq(strlen(salt1), SALT_LENGTH - 1);
    cr_assert_eq(strlen(salt2), SALT_LENGTH - 1);
    cr_assert_eq(strlen(salt3), SALT_LENGTH - 1);
}