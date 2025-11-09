#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/session.h"

Test(session, create_and_destroy) {
    printf("[TEST] Création et destruction de session utilisateur\n");
    fflush(stdout);
    
    session_init();
    
    Session *session = session_create(42);
    cr_assert_not_null(session);
    cr_assert_eq(session->user_id, 42);
    cr_assert_eq(session->active, 1);
    cr_assert(strlen(session->token) > 0);
    
    session_destroy(session->token);
}

Test(session, csrf_validation) {
    printf("[TEST] Validation du token CSRF\n");
    fflush(stdout);
    
    session_init();
    
    Session *session = session_create(100);
    cr_assert_not_null(session);
    
    int valid = session_validate_csrf(session->token, session->csrf_token);
    cr_assert_eq(valid, 1);
    
    int invalid = session_validate_csrf(session->token, "invalid_csrf_token");
    cr_assert_eq(invalid, 0);
    
    session_destroy(session->token);
}

Test(session, create_with_master_password) {
    printf("[TEST] Création de session avec master password\n");
    fflush(stdout);
    
    session_init();
    
    Session *session = session_create_with_key(200, "master_password_123");
    cr_assert_not_null(session);
    cr_assert_eq(session->user_id, 200);
    cr_assert(strlen(session->master_key) > 0);
    
    session_destroy(session->token);
}

Test(session, get_session_by_token) {
    printf("[TEST] Récupération de session par token\n");
    fflush(stdout);
    
    session_init();
    
    Session *created = session_create(300);
    cr_assert_not_null(created);
    
    char token_copy[65];
    strncpy(token_copy, created->token, sizeof(token_copy));
    
    Session *retrieved = session_get(token_copy);
    cr_assert_not_null(retrieved);
    cr_assert_eq(retrieved->user_id, 300);
    cr_assert_str_eq(retrieved->token, token_copy);
    
    session_destroy(token_copy);
}

Test(session, invalid_token) {
    printf("[TEST] Récupération avec token invalide\n");
    fflush(stdout);
    
    session_init();
    
    Session *session = session_get("invalid_nonexistent_token_12345");
    cr_assert_null(session);
}

Test(session, destroyed_session_not_retrievable) {
    printf("[TEST] Session détruite non récupérable\n");
    fflush(stdout);
    
    session_init();
    
    Session *session = session_create(400);
    char token_copy[65];
    strncpy(token_copy, session->token, sizeof(token_copy));
    
    session_destroy(token_copy);
    
    Session *retrieved = session_get(token_copy);
    cr_assert_null(retrieved);
}

Test(session, multiple_sessions_same_user) {
    printf("[TEST] Multiples sessions pour le même utilisateur\n");
    fflush(stdout);
    
    session_init();
    
    Session *session1 = session_create(500);
    Session *session2 = session_create(500);
    
    cr_assert_not_null(session1);
    cr_assert_not_null(session2);
    cr_assert_neq(strcmp(session1->token, session2->token), 0);
    
    session_destroy(session1->token);
    session_destroy(session2->token);
}

Test(session, csrf_different_per_session) {
    printf("[TEST] Tokens CSRF différents par session\n");
    fflush(stdout);
    
    session_init();
    
    Session *session1 = session_create(600);
    Session *session2 = session_create(600);
    
    cr_assert_neq(strcmp(session1->csrf_token, session2->csrf_token), 0);
    
    session_destroy(session1->token);
    session_destroy(session2->token);
}

Test(session, session_timestamps) {
    printf("[TEST] Timestamps de création et utilisation\n");
    fflush(stdout);
    
    session_init();
    
    Session *session = session_create(700);
    cr_assert_not_null(session);
    cr_assert(session->created > 0);
    cr_assert(session->last_used > 0);
    cr_assert(session->last_used >= session->created);
    
    session_destroy(session->token);
}
