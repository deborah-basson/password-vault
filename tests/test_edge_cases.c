#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <sodium.h>
#include "../include/crypto.h"
#include "../include/database.h"

Test(edge_cases, extremely_long_inputs) {
    printf("[TEST LIMITES] Entrées extrêmement longues\n");
    fflush(stdout);
    
    // Test avec input très long
    char long_input[10000];
    memset(long_input, 'A', sizeof(long_input) - 1);
    long_input[sizeof(long_input) - 1] = '\0';
    
    int result = validate_input(long_input, 100);
    cr_assert_eq(result, 0, "Devrait rejeter input trop long");
}

Test(edge_cases, binary_data_injection) {
    printf("[TEST LIMITES] Données binaires dans les entrées\n");
    fflush(stdout);
    
    char binary_data[] = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0x00};
    
    // Tester avec des données binaires
    int result = validate_input(binary_data, 10);
    cr_assert_eq(result, 0, "Devrait rejeter données binaires");
}

Test(edge_cases, sql_injection_attempts) {
    printf("[TEST LIMITES] Tentatives d'injection SQL\n");
    fflush(stdout);
    
    const char *sql_injections[] = {
        "'; DROP TABLE users; --",
        "' OR '1'='1",
        "'; INSERT INTO users VALUES ('hacker', 'pass'); --",
        NULL
    };
    
    for (int i = 0; sql_injections[i] != NULL; i++) {
        char *sanitized = sanitize_html(sql_injections[i]);
        cr_assert_not_null(sanitized);
        
        // Vérifier que le résultat ne contient pas de caractères dangereux
        cr_assert_null(strstr(sanitized, "DROP"));
        cr_assert_null(strstr(sanitized, "INSERT"));
        cr_assert_null(strstr(sanitized, "OR '1'='1"));
        
        sodium_free(sanitized);
    }
}

Test(edge_cases, unicode_and_special_chars) {
    printf("[TEST LIMITES] Caractères Unicode et spéciaux\n");
    fflush(stdout);
    
    const char *special_inputs[] = {
        "Mot de pâssé avéc àccénts",
        "🔐 🚀 🌟 Émojis",
        "Line1\nLine2\tTab",
        "Normal<Script>alert('xss')</Script>",
        NULL
    };
    
    for (int i = 0; special_inputs[i] != NULL; i++) {
        char *sanitized = sanitize_html(special_inputs[i]);
        cr_assert_not_null(sanitized);
        
        // Vérifier que la sanitization fonctionne
        if (strstr(special_inputs[i], "<Script>")) {
            cr_assert_null(strstr(sanitized, "<Script>"));
        }
        
        sodium_free(sanitized);
    }
}