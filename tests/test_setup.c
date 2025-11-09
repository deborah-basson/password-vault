#include <criterion/criterion.h>
#include <sodium.h>
#include <stdio.h>

__attribute__((constructor(101)))
static void init_crypto_once(void) {
    int result = sodium_init();
    
    if (result == 0) {
        // Initialisation de Libsodium pour tous les tests
        fflush(stdout);
    } else if (result < 0) {
        fprintf(stderr, "❌ Erreur initialisation libsodium\n");
        exit(1);
    }
}