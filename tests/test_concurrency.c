#include <criterion/criterion.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "../include/session.h"
#include "../include/database.h"

#define NUM_THREADS 10
#define ITERATIONS_PER_THREAD 20

// Renommer les fonctions pour ce fichier
void setup_database_concurrent(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
}

void cleanup_database_concurrent(void) {
    unlink("vault.db");
    unlink("vault.db-shm");
    unlink("vault.db-wal");
}

Database test_db;
pthread_mutex_t test_mutex = PTHREAD_MUTEX_INITIALIZER;

void* concurrent_session_creation(void *arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < ITERATIONS_PER_THREAD; i++) {
        Session *session = session_create(thread_id * 1000 + i);
        cr_assert_not_null(session, "Session création devrait réussir en concurrence");
        
        // Utiliser brièvement la session
        Session *retrieved = session_get(session->token);
        cr_assert_not_null(retrieved, "Devrait retrouver session créée");
        cr_assert_eq(retrieved->user_id, session->user_id, "User ID devrait correspondre");
        
        // Remplacer usleep par nanosleep
        struct timespec ts = {0, 1000000}; // 1ms
        nanosleep(&ts, NULL);
        
        session_destroy(session->token);
    }
    
    return NULL;
}

Test(concurrency, multiple_session_creation, .init = session_init) {
    printf("[TEST CONCURRENCE] Création multiple de sessions\n");
    fflush(stdout);
    
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    // Créer les threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        int result = pthread_create(&threads[i], NULL, concurrent_session_creation, &thread_ids[i]);
        cr_assert_eq(result, 0, "Création thread devrait réussir");
    }
    
    // Attendre la fin des threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("✓ %d threads ont créé %d sessions chacun\n", NUM_THREADS, ITERATIONS_PER_THREAD);
}

void* concurrent_password_operations(void *arg) {
    int thread_id = *(int*)arg;
    char website[50];
    char username[50];
    char password[50];
    
    for (int i = 0; i < 5; i++) {
        snprintf(website, sizeof(website), "site-thread%d-iter%d.com", thread_id, i);
        snprintf(username, sizeof(username), "user%d@thread%d.com", i, thread_id);
        snprintf(password, sizeof(password), "pass%d-thread%d", i, thread_id);
        
        pthread_mutex_lock(&test_mutex);
        int result = db_add_password(&test_db, website, username, password, "Test concurrent");
        pthread_mutex_unlock(&test_mutex);
        
        cr_assert_eq(result, 1, "Ajout password devrait réussir");
        
        // Remplacer usleep
        struct timespec ts = {0, 50000000}; // 50ms
        nanosleep(&ts, NULL);
    }
    
    return NULL;
}

Test(concurrency, database_concurrent_access, .init = setup_database_concurrent, .fini = cleanup_database_concurrent) {
    printf("[TEST CONCURRENCE] Accès concurrent à la base\n");
    fflush(stdout);
    
    int init_result = db_init(&test_db);
    cr_assert_eq(init_result, 1, "Initialisation DB devrait réussir");
    
    // Créer un utilisateur de test d'abord
    const char *hash = "$argon2id$test$hash";
    const char *salt = "testsalt123";
    const char *totp = "TESTTOTP123";
    int user_created = db_create_user(&test_db, hash, salt, totp);
    cr_assert_eq(user_created, 1, "Création utilisateur devrait réussir");
    
    pthread_t threads[5];
    int thread_ids[5];
    
    // Créer des threads pour opérations concurrentes
    for (int i = 0; i < 5; i++) {
        thread_ids[i] = i + 1;
        int result = pthread_create(&threads[i], NULL, concurrent_password_operations, &thread_ids[i]);
        cr_assert_eq(result, 0, "Création thread devrait réussir");
    }
    
    // Attendre la fin
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Vérifier que tous les passwords ont été créés
    PasswordEntry *entries = NULL;
    int count = 0;
    int get_result = db_get_all_passwords(&test_db, &entries, &count);
    
    cr_assert_eq(get_result, 1, "Récupération passwords devrait réussir");
    cr_assert_eq(count, 25, "Devrait avoir 25 passwords (5 threads × 5 iterations)");
    
    if (entries) {
        free(entries);
    }
    db_close(&test_db);
}