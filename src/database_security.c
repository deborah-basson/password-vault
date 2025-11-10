#include "../include/config.h"
#include "../include/crypto.h"
#include "../include/database.h"
#include <sodium.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>  // Pour strcasecmp

// ==================== VÉRIFICATION DE FAIBLESSE DES MOTS DE PASSE ====================

int is_common_password(const char *password) {
    if (!password) return 1;
    
    const char *common_passwords[] = {
        "password", "123456", "12345678", "123456789", "12345",
        "qwerty", "abc123", "password1", "1234567", "1234567890",
        "admin", "welcome", "monkey", "letmein", "master",
        "azerty", "000000", "111111", "sunshine", "iloveyou",
        NULL
    };
    
    for (int i = 0; common_passwords[i] != NULL; i++) {
        if (strcasecmp(password, common_passwords[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

// Fonction utilitaire pour recherche insensible à la casse
static const char *case_insensitive_strstr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    
    size_t needle_len = strlen(needle);
    size_t haystack_len = strlen(haystack);
    
    if (needle_len > haystack_len) return NULL;
    
    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        int match = 1;
        for (size_t j = 0; j < needle_len; j++) {
            if (tolower((unsigned char)haystack[i + j]) != tolower((unsigned char)needle[j])) {
                match = 0;
                break;
            }
        }
        if (match) {
            return haystack + i;
        }
    }
    
    return NULL;
}

int check_password_breach(const char *password) {
    if (!password) return 1;
    
    size_t len = strlen(password);
    if (len < 8) return 1;
    
    // Vérifier les séquences simples
    if (strstr(password, "123") != NULL || 
        strstr(password, "abc") != NULL ||
        strstr(password, "000") != NULL ||
        strstr(password, "111") != NULL ||
        strstr(password, "aaa") != NULL) {
        return 1;
    }
    
    // Vérifier les répétitions excessives
    size_t repeats = 0;
    for (size_t i = 1; i < len; i++) {
        if (password[i] == password[i-1]) repeats++;
    }
    if (len > 0 && repeats > len / 2) {
        return 1;
    }
    
    // Vérifier les motifs de clavier (version insensible à la casse)
    const char *keyboard_patterns[] = {
        "qwerty", "azerty", "yxcvbn", "asdfgh", "zxcvbn", "qazwsx",
        NULL
    };
    
    for (int i = 0; keyboard_patterns[i] != NULL; i++) {
        if (case_insensitive_strstr(password, keyboard_patterns[i]) != NULL) {
            return 1;
        }
    }
    
    return 0;
}

// ==================== VALIDATION DE MOT DE PASSE RENFORCÉE ====================

int validate_enhanced_password(const char *password) {
    if (!password) return 0;
    
    size_t len = strlen(password);
    if (len < 12) return 0;
    
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;
    size_t unique_chars = 0;
    char unique_set[256] = {0};
    
    for (size_t i = 0; i < len; i++) {
        unsigned char c = password[i];
        
        if (isupper(c)) has_upper = 1;
        else if (islower(c)) has_lower = 1;
        else if (isdigit(c)) has_digit = 1;
        else if (ispunct(c)) has_special = 1;
        
        if (!unique_set[c]) {
            unique_set[c] = 1;
            unique_chars++;
        }
    }
    
    // Au moins 3 des 4 catégories
    int categories = has_upper + has_lower + has_digit + has_special;
    if (categories < 3) return 0;
    
    // Au moins 50% de caractères uniques
    if (unique_chars < len / 2) return 0;
    
    // Vérifier les mots de passe courants
    if (is_common_password(password)) return 0;
    
    // Vérifier les patterns de violation
    if (check_password_breach(password)) return 0;
    
    return 1;
}