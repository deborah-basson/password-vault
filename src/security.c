#include "../include/security.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

void generate_salt(char *salt, int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    srand((unsigned int)time(NULL));
    for (int i = 0; i < length - 1; i++) {
        salt[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    salt[length - 1] = '\0';
}

void generate_secure_password(char *password, int length) {
    const char lowercase[] = "abcdefghijklmnopqrstuvwxyz";
    const char uppercase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char digits[] = "0123456789";
    const char symbols[] = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    srand((unsigned int)time(NULL));
    
    // Assurer au moins un caractère de chaque type
    password[0] = lowercase[rand() % 26];
    password[1] = uppercase[rand() % 26];
    password[2] = digits[rand() % 10];
    password[3] = symbols[rand() % 26];
    
    // Remplir le reste
    const char all_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    for (int i = 4; i < length - 1; i++) {
        password[i] = all_chars[rand() % (sizeof(all_chars) - 1)];
    }
    password[length - 1] = '\0';
    
    // Mélanger le mot de passe
    for (int i = 0; i < length - 1; i++) {
        int j = rand() % (length - 1);
        char temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }
}

int password_strength_check(const char *password) {
    int score = 0;
    int length = strlen(password);
    
    if (length >= 8) score++;
    if (length >= 12) score++;
    
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;
    
    for (int i = 0; password[i]; i++) {
        if (isupper(password[i])) has_upper = 1;
        if (islower(password[i])) has_lower = 1;
        if (isdigit(password[i])) has_digit = 1;
        if (ispunct(password[i])) has_special = 1;
    }
    
    if (has_upper && has_lower) score++;
    if (has_digit) score++;
    if (has_special) score++;
    
    return score; // 0-6 points
}