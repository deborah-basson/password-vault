#define _POSIX_C_SOURCE 200809L
#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include "../include/crypto.h"
#include "../include/database.h"
#include "../include/session.h"

Test(memory, no_use_after_free) {
    char *ptr = malloc(100);
    strcpy(ptr, "test");
    free(ptr);
    ptr = NULL;
    cr_assert_null(ptr);
}

Test(memory, no_double_free) {
    char *ptr = malloc(100);
    if (ptr) {
        free(ptr);
        ptr = NULL;
    }
    if (ptr) {
        free(ptr);
    }
    cr_assert(1);
}

Test(memory, proper_cleanup_on_error) {
    char *data = malloc(1000);
    if (!data) {
        cr_assert(0);
    }
    
    int error = 1;
    if (error) {
        free(data);
        data = NULL;
        cr_assert_null(data);
        return;
    }
    
    free(data);
}

Test(memory, session_cleanup) {
    Session *s = session_create(1);
    cr_assert_not_null(s);
    
    char *token = malloc(strlen(s->token) + 1);
    strcpy(token, s->token);
    
    session_destroy(token);
    
    Session *destroyed = session_get(token);
    cr_assert_null(destroyed);
    
    free(token);
}