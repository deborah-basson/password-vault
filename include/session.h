#ifndef SESSION_H
#define SESSION_H

#include <time.h>

// Définition complète de la structure Session
typedef struct {
    char token[65];
    time_t created;
    time_t last_used;
    int user_id;
    int active;
} Session;

// Fonctions
void session_init(void);
Session* session_create(int user_id);
Session* session_get(const char *token);
void session_destroy(const char *token);
void session_cleanup_expired(void);
void* session_cleanup_thread(void *arg);

#endif