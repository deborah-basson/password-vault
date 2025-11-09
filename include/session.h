#ifndef SESSION_H
#define SESSION_H

#include <time.h>

typedef struct {
    char token[65];
    char master_key[256];
    char csrf_token[65];
    time_t created;
    time_t last_used;
    int user_id;
    int active;
} Session;

void session_init(void);
Session* session_create(int user_id);
Session* session_create_with_key(int user_id, const char *master_password);
Session* session_get(const char *token);
int session_validate_csrf(const char *token, const char *csrf_token);
void session_destroy(const char *token);
void session_cleanup_expired(void);
void* session_cleanup_thread(void *arg);

#endif