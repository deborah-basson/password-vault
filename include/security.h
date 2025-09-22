#ifndef SECURITY_H
#define SECURITY_H

void generate_salt(char *salt, int length);
void hash_password(const char *password, const char *salt, char *hash);
int encrypt_data(const unsigned char *plaintext, int plaintext_len, unsigned char *key,
                unsigned char *iv, unsigned char *ciphertext);
int decrypt_data(const unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                unsigned char *iv, unsigned char *plaintext);
void generate_secure_password(char *password, int length);
int password_strength_check(const char *password);

#endif