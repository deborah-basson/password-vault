#include "../include/crypto.h"
#include <sodium.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

int crypto_init(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "❌ Erreur initialisation libsodium\n");
        return 0;
    }
    printf("✓ Libsodium initialisé\n");
    return 1;
}

int validate_password_strength(const char *password) {
    if (!password) return 0;
    
    size_t len = strlen(password);
    if (len < 12) return 0;
    
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;
    
    for (size_t i = 0; i < len; i++) {
        if (isupper(password[i])) has_upper = 1;
        else if (islower(password[i])) has_lower = 1;
        else if (isdigit(password[i])) has_digit = 1;
        else if (ispunct(password[i])) has_special = 1;
    }
    
    int score = has_upper + has_lower + has_digit + has_special;
    return score >= 3;
}

int validate_input(const char *input, size_t max_length) {
    if (!input) return 0;
    
    size_t len = strlen(input);
    if (len == 0 || len > max_length) return 0;
    
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '\0') return 0;
    }
    
    return 1;
}

void generate_salt(char *salt, size_t length) {
    randombytes_buf(salt, length - 1);
    
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (size_t i = 0; i < length - 1; i++) {
        salt[i] = charset[((unsigned char)salt[i]) % 64];
    }
    salt[length - 1] = '\0';
}

void generate_secure_password(char *password, int length) {
    if (length < 12) length = 12;
    
    const char lowercase[] = "abcdefghijklmnopqrstuvwxyz";
    const char uppercase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char digits[] = "0123456789";
    const char symbols[] = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    unsigned char random_bytes[256];
    randombytes_buf(random_bytes, length);
    
    password[0] = lowercase[random_bytes[0] % 26];
    password[1] = uppercase[random_bytes[1] % 26];
    password[2] = digits[random_bytes[2] % 10];
    password[3] = symbols[random_bytes[3] % 28];
    
    const char all[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
    int all_len = sizeof(all) - 1;
    
    for (int i = 4; i < length - 1; i++) {
        password[i] = all[random_bytes[i] % all_len];
    }
    password[length - 1] = '\0';
    
    for (int i = length - 2; i > 0; i--) {
        int j = random_bytes[i + length] % (i + 1);
        char temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }
}

void generate_totp_secret(char *secret, size_t length) {
    const char base32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    unsigned char random_bytes[32];
    
    randombytes_buf(random_bytes, length - 1);
    
    for (size_t i = 0; i < length - 1; i++) {
        secret[i] = base32[random_bytes[i] % 32];
    }
    secret[length - 1] = '\0';
}

int hash_password(const char *password, const char *salt, char *output, size_t output_len) {
    if (!password || !salt || !output || output_len < crypto_pwhash_STRBYTES) {
        return 0;
    }
    
    if (crypto_pwhash_str(
        output,
        password,
        strlen(password),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE
    ) != 0) {
        return 0;
    }
    
    return 1;
}

int verify_password(const char *password, const char *salt, const char *hash) {
    (void)salt;
    
    if (!password || !hash) return 0;
    
    return crypto_pwhash_str_verify(hash, password, strlen(password)) == 0;
}

int encrypt_data(const char *plaintext, const char *key, char *ciphertext, size_t *cipher_len) {
    if (!plaintext || !key || !ciphertext || !cipher_len) return 0;
    
    size_t plaintext_len = strlen(plaintext);
    
    unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
    randombytes_buf(nonce, sizeof(nonce));
    
    unsigned char derived_key[crypto_aead_aes256gcm_KEYBYTES];
    crypto_generichash(derived_key, sizeof(derived_key),
                       (unsigned char*)key, strlen(key),
                       NULL, 0);
    
    unsigned char binary_cipher[512];
    unsigned long long ciphertext_len;
    
    memcpy(binary_cipher, nonce, sizeof(nonce));
    
    if (crypto_aead_aes256gcm_encrypt(
        binary_cipher + sizeof(nonce),
        &ciphertext_len,
        (unsigned char*)plaintext,
        plaintext_len,
        NULL, 0,
        NULL,
        nonce,
        derived_key
    ) != 0) {
        return 0;
    }
    
    size_t total_binary_len = sizeof(nonce) + ciphertext_len;
    
    size_t b64_maxlen = sodium_base64_ENCODED_LEN(total_binary_len, sodium_base64_VARIANT_ORIGINAL);
    if (b64_maxlen > *cipher_len) {
        return 0;
    }
    
    sodium_bin2base64(ciphertext, *cipher_len, binary_cipher, total_binary_len, sodium_base64_VARIANT_ORIGINAL);
    
    *cipher_len = strlen(ciphertext);
    return 1;
}

int decrypt_data(const char *ciphertext, size_t cipher_len, const char *key, char *plaintext) {
    if (!ciphertext || !key || !plaintext) return 0;
    
    unsigned char binary_cipher[512];
    size_t binary_len;
    
    if (sodium_base642bin(binary_cipher, sizeof(binary_cipher), ciphertext, cipher_len,
                         NULL, &binary_len, NULL, sodium_base64_VARIANT_ORIGINAL) != 0) {
        return 0;
    }
    
    if (binary_len < crypto_aead_aes256gcm_NPUBBYTES + crypto_aead_aes256gcm_ABYTES) {
        return 0;
    }
    
    unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
    memcpy(nonce, binary_cipher, sizeof(nonce));
    
    unsigned char derived_key[crypto_aead_aes256gcm_KEYBYTES];
    crypto_generichash(derived_key, sizeof(derived_key),
                       (unsigned char*)key, strlen(key),
                       NULL, 0);
    
    unsigned long long plaintext_len;
    
    if (crypto_aead_aes256gcm_decrypt(
        (unsigned char*)plaintext,
        &plaintext_len,
        NULL,
        binary_cipher + sizeof(nonce),
        binary_len - sizeof(nonce),
        NULL, 0,
        nonce,
        derived_key
    ) != 0) {
        return 0;
    }
    
    plaintext[plaintext_len] = '\0';
    return 1;
}

static void hmac_sha256(const unsigned char *key, size_t key_len,
                       const unsigned char *data, size_t data_len,
                       unsigned char *out) {
    crypto_auth_hmacsha256_state state;
    crypto_auth_hmacsha256_init(&state, key, key_len);
    crypto_auth_hmacsha256_update(&state, data, data_len);
    crypto_auth_hmacsha256_final(&state, out);
}

static int base32_decode(const char *encoded, unsigned char *result, size_t *result_len) {
    const char *base32_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    size_t len = strlen(encoded);
    size_t buffer = 0;
    int bits_left = 0;
    size_t count = 0;
    
    for (size_t i = 0; i < len; i++) {
        char *p = strchr(base32_chars, encoded[i]);
        if (!p) continue;
        
        buffer = (buffer << 5) | (p - base32_chars);
        bits_left += 5;
        
        if (bits_left >= 8) {
            result[count++] = (buffer >> (bits_left - 8)) & 0xFF;
            bits_left -= 8;
        }
    }
    
    *result_len = count;
    return 1;
}

int generate_totp_code(const char *secret, long time_step) {
    unsigned char key[64];
    size_t key_len;
    
    base32_decode(secret, key, &key_len);
    
    unsigned char time_bytes[8];
    for (int i = 7; i >= 0; i--) {
        time_bytes[i] = time_step & 0xFF;
        time_step >>= 8;
    }
    
    unsigned char hmac[32];
    hmac_sha256(key, key_len, time_bytes, 8, hmac);
    
    int offset = hmac[31] & 0x0F;
    int code = ((hmac[offset] & 0x7F) << 24) |
               ((hmac[offset + 1] & 0xFF) << 16) |
               ((hmac[offset + 2] & 0xFF) << 8) |
               (hmac[offset + 3] & 0xFF);
    
    return code % 1000000;
}

int verify_totp_code(const char *secret, int code, int window) {
    long current_time = time(NULL) / 30;
    
    for (int i = -window; i <= window; i++) {
        int generated = generate_totp_code(secret, current_time + i);
        if (generated == code) {
            return 1;
        }
    }
    
    return 0;
}

char* sanitize_html(const char *input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    char *output = sodium_malloc(len * 6 + 1);
    if (!output) return NULL;
    
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (input[i]) {
            case '<':
                strcpy(&output[j], "&lt;");
                j += 4;
                break;
            case '>':
                strcpy(&output[j], "&gt;");
                j += 4;
                break;
            case '&':
                strcpy(&output[j], "&amp;");
                j += 5;
                break;
            case '"':
                strcpy(&output[j], "&quot;");
                j += 6;
                break;
            case '\'':
                strcpy(&output[j], "&#x27;");
                j += 6;
                break;
            case '/':
                strcpy(&output[j], "&#x2F;");
                j += 6;
                break;
            default:
                output[j++] = input[i];
        }
    }
    output[j] = '\0';
    
    return output;
}

void secure_zero_memory(void *ptr, size_t len) {
    sodium_memzero(ptr, len);
}