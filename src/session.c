#include "../include/config.h"
#include "../include/crypto.h"
#include "../include/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

static Session active_sessions[MAX_CLIENTS];
static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

void session_init() {
    pthread_mutex_lock(&session_mutex);
    memset(active_sessions, 0, sizeof(active_sessions));
    pthread_mutex_unlock(&session_mutex);
    printf("✓ Gestionnaire de sessions initialisé\n");
}

// Fonction originale sans master key
Session* session_create(int user_id) {
    return session_create_with_key(user_id, NULL);
}

// Fonction avec master key - UNE SEULE DÉFINITION
Session* session_create_with_key(int user_id, const char *master_password) {
    pthread_mutex_lock(&session_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!active_sessions[i].active) {
            generate_secure_password(active_sessions[i].token, 64);
            generate_secure_password(active_sessions[i].csrf_token, 64);
            
            if (master_password) {
                // Stocker temporairement pour déchiffrement
                strncpy(active_sessions[i].master_key, master_password, 
                       sizeof(active_sessions[i].master_key) - 1);
                active_sessions[i].master_key[sizeof(active_sessions[i].master_key) - 1] = '\0';
            } else {
                memset(active_sessions[i].master_key, 0, sizeof(active_sessions[i].master_key));
            }
            
            active_sessions[i].created = time(NULL);
            active_sessions[i].last_used = time(NULL);
            active_sessions[i].user_id = user_id;
            active_sessions[i].active = 1;
            
            pthread_mutex_unlock(&session_mutex);
            printf("✓ Session créée pour user_id=%d (avec CSRF)\n", user_id);
            return &active_sessions[i];
        }
    }
    
    pthread_mutex_unlock(&session_mutex);
    fprintf(stderr, "❌ Limite de sessions atteinte\n");
    return NULL;
}

Session* session_get(const char *token) {
    if (!token) return NULL;
    
    pthread_mutex_lock(&session_mutex);
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_sessions[i].active && 
            strcmp(active_sessions[i].token, token) == 0) {
            
            if (now - active_sessions[i].last_used > SESSION_TIMEOUT) {
                // NETTOYAGE SÉCURISÉ à l'expiration
                secure_zero_memory(active_sessions[i].master_key, sizeof(active_sessions[i].master_key));
                active_sessions[i].active = 0;
                secure_zero_memory(&active_sessions[i], sizeof(Session));
                pthread_mutex_unlock(&session_mutex);
                return NULL;
            }
            
            active_sessions[i].last_used = now;
            pthread_mutex_unlock(&session_mutex);
            return &active_sessions[i];
        }
    }
    
    pthread_mutex_unlock(&session_mutex);
    return NULL;
}

void session_destroy(const char *token) {
    if (!token) return;
    
    pthread_mutex_lock(&session_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_sessions[i].active && 
            strcmp(active_sessions[i].token, token) == 0) {
            // NETTOYAGE SÉCURISÉ avant destruction
            secure_zero_memory(active_sessions[i].master_key, sizeof(active_sessions[i].master_key));
            active_sessions[i].active = 0;
            secure_zero_memory(&active_sessions[i], sizeof(Session));
            printf("✓ Session détruite (mémoire nettoyée)\n");
            break;
        }
    }
    
    pthread_mutex_unlock(&session_mutex);
}

void session_cleanup_expired() {
    pthread_mutex_lock(&session_mutex);
    time_t now = time(NULL);
    int cleaned = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_sessions[i].active && 
            now - active_sessions[i].last_used > SESSION_TIMEOUT) {
            // NETTOYAGE SÉCURISÉ des sessions expirées
            secure_zero_memory(active_sessions[i].master_key, sizeof(active_sessions[i].master_key));
            active_sessions[i].active = 0;
            secure_zero_memory(&active_sessions[i], sizeof(Session));
            cleaned++;
        }
    }
    
    pthread_mutex_unlock(&session_mutex);
    
    if (cleaned > 0) {
        printf("🧹 %d session(s) expirée(s) nettoyée(s)\n", cleaned);
    }
}

//: Nettoyage complet de toutes les sessions
void session_cleanup_all() {
    pthread_mutex_lock(&session_mutex);
    int cleaned = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_sessions[i].active) {
            // NETTOYAGE SÉCURISÉ de toutes les sessions
            secure_zero_memory(active_sessions[i].master_key, sizeof(active_sessions[i].master_key));
            active_sessions[i].active = 0;
            secure_zero_memory(&active_sessions[i], sizeof(Session));
            cleaned++;
        }
    }
    
    pthread_mutex_unlock(&session_mutex);
    printf("✓ Toutes les sessions nettoyées (%d sessions)\n", cleaned);
}

void* session_cleanup_thread(void *arg) {
    (void)arg;
    
    while (1) {
        sleep(300); // Vérifie toutes les 5 minutes
        session_cleanup_expired();
    }
    
    return NULL;
}

int session_validate_csrf(const char *token, const char *csrf_token) {
    if (!token || !csrf_token) return 0;
    
    pthread_mutex_lock(&session_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_sessions[i].active && 
            strcmp(active_sessions[i].token, token) == 0) {
            int valid = strcmp(active_sessions[i].csrf_token, csrf_token) == 0;
            pthread_mutex_unlock(&session_mutex);
            return valid;
        }
    }
    
    pthread_mutex_unlock(&session_mutex);
    return 0;
}