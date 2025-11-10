#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>

// Initialisation
int crypto_init(void);

// Validation
int validate_password_strength(const char *password);
int validate_input(const char *input, size_t max_length);

// Génération
void generate_salt(char *salt, size_t length);
void generate_secure_password(char *password, int length);
void generate_secure_passphrase(char *output, int length);  // AJOUT
void generate_totp_secret(char *secret, size_t length);

// Hachage
int hash_password(const char *password, const char *salt, char *output, size_t output_len);
int verify_password(const char *password, const char *salt, const char *hash);

// Chiffrement
int encrypt_data(const char *plaintext, const char *key, char *ciphertext, size_t *cipher_len);
int decrypt_data(const char *ciphertext, size_t cipher_len, const char *key, char *plaintext);

// TOTP
int generate_totp_code(const char *secret, long time_step);
int verify_totp_code(const char *secret, int code, int window);

// Sécurité
char* sanitize_html(const char *input);
void secure_zero_memory(void *ptr, size_t len);

#endif