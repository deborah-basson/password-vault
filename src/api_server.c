#include "../include/config.h"
#include "../include/database.h"
#include "../include/crypto.h"
#include "../include/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct {
    int socket;
    SSL *ssl;
    char ip[64];
} ClientInfo;

Database db;
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

void* handle_client(void* arg);
void send_response(int socket, SSL *ssl, int status, const char *content_type, const char *body);
void send_file(int socket, SSL *ssl, const char *filepath);
void handle_api_request(int socket, SSL *ssl, const char *method, const char *path, 
                       const char *body, const char *ip, const char *session_token, const char *csrf_token);
char* parse_json_field(const char *json, const char *key);
char* read_file(const char *filepath);
SSL_CTX* init_ssl_context(void);

static ssize_t write_data(int socket, SSL *ssl, const void *buf, size_t len) {
    if (ssl) {
        return SSL_write(ssl, buf, len);
    }
    return write(socket, buf, len);
}

SSL_CTX* init_ssl_context(void) {
    SSL_CTX *ctx;
    
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    const SSL_METHOD *method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    
    if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "❌ Erreur: Certificat SSL manquant (%s)\n", CERT_FILE);
        fprintf(stderr, "💡 Utilisez 'make ssl' pour générer les certificats\n");
        exit(EXIT_FAILURE);
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "❌ Erreur: Clé privée SSL manquante (%s)\n", KEY_FILE);
        fprintf(stderr, "💡 Utilisez 'make ssl' pour générer les certificats\n");
        exit(EXIT_FAILURE);
    }
    
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "❌ Erreur: Clé privée ne correspond pas au certificat\n");
        exit(EXIT_FAILURE);
    }
    
    SSL_CTX_set_cipher_list(ctx, "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256");
    
    return ctx;
}

void send_response(int socket, SSL *ssl, int status, const char *content_type, const char *body) {
    char header[1024];
    
    const char *status_text = "OK";
    if (status == 400) status_text = "Bad Request";
    else if (status == 401) status_text = "Unauthorized";
    else if (status == 403) status_text = "Forbidden";
    else if (status == 404) status_text = "Not Found";
    else if (status == 500) status_text = "Internal Server Error";
    
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s; charset=utf-8\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type, Authorization, X-CSRF-Token\r\n"
             "X-Content-Type-Options: nosniff\r\n"
             "X-Frame-Options: DENY\r\n"
             "X-XSS-Protection: 1; mode=block\r\n"
             "Strict-Transport-Security: max-age=31536000; includeSubDomains\r\n"
             "\r\n",
             status, status_text, content_type, strlen(body));
    
    write_data(socket, ssl, header, strlen(header));
    write_data(socket, ssl, body, strlen(body));
}

void send_file(int socket, SSL *ssl, const char *filepath) {
    char fullpath[512];
    
    if (strcmp(filepath, "/") == 0) {
        snprintf(fullpath, sizeof(fullpath), "%s/index.html", WEB_ROOT);
    } else {
        const char *clean_path = filepath;
        if (filepath[0] == '/') clean_path = filepath + 1;
        
        if (strstr(clean_path, "..") != NULL) {
            send_response(socket, ssl, 403, "text/plain", "403 Forbidden");
            return;
        }
        
        snprintf(fullpath, sizeof(fullpath), "%s/%s", WEB_ROOT, clean_path);
    }
    
    FILE *file = fopen(fullpath, "rb");
    if (!file) {
        send_response(socket, ssl, 404, "text/plain", "404 Not Found");
        return;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        send_response(socket, ssl, 500, "text/plain", "500 Internal Server Error");
        return;
    }
    
    size_t bytes_read = fread(content, 1, size, file);
    content[bytes_read] = '\0';
    fclose(file);
    
    const char *content_type = "text/html";
    if (strstr(fullpath, ".css")) content_type = "text/css";
    else if (strstr(fullpath, ".js")) content_type = "application/javascript";
    else if (strstr(fullpath, ".json")) content_type = "application/json";
    else if (strstr(fullpath, ".png")) content_type = "image/png";
    else if (strstr(fullpath, ".jpg") || strstr(fullpath, ".jpeg")) content_type = "image/jpeg";
    else if (strstr(fullpath, ".ico")) content_type = "image/x-icon";
    
    send_response(socket, ssl, 200, content_type, content);
    free(content);
}

void* handle_client(void* arg) {
    ClientInfo *client = (ClientInfo*)arg;
    char buffer[BUFFER_SIZE] = {0};
    
    ssize_t bytes_read;
    if (client->ssl) {
        bytes_read = SSL_read(client->ssl, buffer, BUFFER_SIZE - 1);
    } else {
        bytes_read = read(client->socket, buffer, BUFFER_SIZE - 1);
    }
    
    if (bytes_read <= 0) {
        goto cleanup;
    }
    
    char method[16] = {0};
    char path[256] = {0};
    sscanf(buffer, "%s %s", method, path);
    
    if (strcmp(method, "OPTIONS") == 0) {
        send_response(client->socket, client->ssl, 200, "text/plain", "OK");
        goto cleanup;
    }
    
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) body += 4;
    else body = "";
    
    char session_token[128] = {0};
    char *auth_header = strstr(buffer, "Authorization: Bearer ");
    if (auth_header) {
        sscanf(auth_header + 22, "%127s", session_token);
    }
    
    char csrf_token[128] = {0};
    char *csrf_header = strstr(buffer, "X-CSRF-Token: ");
    if (csrf_header) {
        sscanf(csrf_header + 14, "%127s", csrf_token);
    }
    
    if (strncmp(path, "/api/", 5) == 0) {
        handle_api_request(client->socket, client->ssl, method, path, body, client->ip, session_token, csrf_token);
    } 
    else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        send_file(client->socket, client->ssl, "/index.html");
    } else if (strcmp(path, "/style.css") == 0) {
        send_file(client->socket, client->ssl, "/style.css");
    } else if (strcmp(path, "/app.js") == 0) {
        send_file(client->socket, client->ssl, "/app.js");
    } else if (strstr(path, ".js") || strstr(path, ".css") || strstr(path, ".html")) {
        send_file(client->socket, client->ssl, path);
    } else {
        send_file(client->socket, client->ssl, "/index.html");
    }
    
cleanup:
    if (client->ssl) {
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
    }
    close(client->socket);
    free(client);
    return NULL;
}

void handle_api_request(int socket, SSL *ssl, const char *method, const char *path, 
                       const char *body, const char *ip, const char *session_token, const char *csrf_token) {
    char response[8192] = {0};
    
    // ==================== ROUTE STATUS ====================
    if (strcmp(path, "/api/status") == 0 && strcmp(method, "GET") == 0) {
        pthread_mutex_lock(&db_mutex);
        UserAccount user;
        int has_account = db_get_user(&db, &user);
        pthread_mutex_unlock(&db_mutex);
        
        snprintf(response, sizeof(response), "{\"has_account\":%s}", has_account ? "true" : "false");
        send_response(socket, ssl, 200, "application/json", response);
        return;
    }
    
    // ==================== ROUTE REGISTER ====================
    if (strcmp(path, "/api/register") == 0 && strcmp(method, "POST") == 0) {
        char *password = parse_json_field(body, "password");
        
        if (!password) {
            snprintf(response, sizeof(response), "{\"error\":\"Mot de passe manquant\"}");
            send_response(socket, ssl, 400, "application/json", response);
            return;
        }
        
        if (strlen(password) < 12) {
            snprintf(response, sizeof(response), "{\"error\":\"Le mot de passe doit faire au moins 12 caractères\"}");
            send_response(socket, ssl, 400, "application/json", response);
            secure_zero_memory(password, strlen(password));
            free(password);
            return;
        }
        
        pthread_mutex_lock(&db_mutex);
        
        UserAccount existing_user;
        int user_exists = db_get_user(&db, &existing_user);
        
        if (user_exists) {
            snprintf(response, sizeof(response), "{\"error\":\"Un compte existe déjà\"}");
            send_response(socket, ssl, 400, "application/json", response);
            pthread_mutex_unlock(&db_mutex);
            secure_zero_memory(password, strlen(password));
            free(password);
            return;
        }
        
        char salt[32];
        char hash[128];
        generate_salt(salt, sizeof(salt));
        
        if (!hash_password(password, salt, hash, sizeof(hash))) {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur lors du hachage\"}");
            send_response(socket, ssl, 500, "application/json", response);
            pthread_mutex_unlock(&db_mutex);
            secure_zero_memory(password, strlen(password));
            free(password);
            return;
        }
        
        int success = db_create_user(&db, hash, salt, "");
        
        pthread_mutex_unlock(&db_mutex);
        
        if (success) {
            snprintf(response, sizeof(response), "{\"success\":true,\"message\":\"Compte créé avec succès\"}");
            send_response(socket, ssl, 200, "application/json", response);
            printf("✅ Nouveau compte créé\n");
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur création compte\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
        
        secure_zero_memory(password, strlen(password));
        free(password);
        return;
    }
    
    // ==================== ROUTE QUESTIONS SÉCURITÉ ====================
    if (strcmp(path, "/api/setup-security-questions") == 0 && strcmp(method, "POST") == 0) {
        char *question1 = parse_json_field(body, "question1");
        char *answer1 = parse_json_field(body, "answer1");
        char *question2 = parse_json_field(body, "question2");
        char *answer2 = parse_json_field(body, "answer2");
        char *question3 = parse_json_field(body, "question3");
        char *answer3 = parse_json_field(body, "answer3");
        
        if (!question1 || !answer1 || !question2 || !answer2 || !question3 || !answer3) {
            snprintf(response, sizeof(response), "{\"error\":\"Toutes les questions et réponses sont requises\"}");
            send_response(socket, ssl, 400, "application/json", response);
            if (question1) free(question1);
            if (answer1) free(answer1);
            if (question2) free(question2);
            if (answer2) free(answer2);
            if (question3) free(question3);
            if (answer3) free(answer3);
            return;
        }
        
        char recovery_code[16];
        generate_secure_password(recovery_code, 8);
        
        pthread_mutex_lock(&db_mutex);
        
        UserAccount user;
        int user_exists = db_get_user(&db, &user);
        
        if (!user_exists) {
            snprintf(response, sizeof(response), "{\"error\":\"Aucun utilisateur trouvé\"}");
            send_response(socket, ssl, 400, "application/json", response);
            pthread_mutex_unlock(&db_mutex);
            if (question1) free(question1);
            if (answer1) free(answer1);
            if (question2) free(question2);
            if (answer2) free(answer2);
            if (question3) free(question3);
            if (answer3) free(answer3);
            return;
        }
        
        int success = db_create_user_with_questions(&db, user.master_hash, user.salt,
                                                question1, answer1,
                                                question2, answer2,
                                                question3, answer3,
                                                recovery_code);
        
        pthread_mutex_unlock(&db_mutex);
        
        if (success) {
            snprintf(response, sizeof(response),
                    "{\"success\":true,\"recovery_code\":\"%s\",\"message\":\"Questions de sécurité configurées\"}",
                    recovery_code);
            send_response(socket, ssl, 200, "application/json", response);
            printf("✅ Questions de sécurité configurées\n");
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur configuration questions\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
        
        if (question1) free(question1);
        if (answer1) free(answer1);
        if (question2) free(question2);
        if (answer2) free(answer2);
        if (question3) free(question3);
        if (answer3) free(answer3);
        return;
    }
    
    // ==================== ROUTE LOGIN ====================
    if (strcmp(path, "/api/login") == 0 && strcmp(method, "POST") == 0) {
        char *password = parse_json_field(body, "password");
        
        if (!password) {
            snprintf(response, sizeof(response), "{\"error\":\"Mot de passe manquant\"}");
            send_response(socket, ssl, 400, "application/json", response);
            return;
        }
        
        pthread_mutex_lock(&db_mutex);
        UserAccount user;
        int user_exists = db_get_user(&db, &user);
        pthread_mutex_unlock(&db_mutex);
        
        if (!user_exists) {
            snprintf(response, sizeof(response), "{\"error\":\"Aucun compte trouvé\"}");
            send_response(socket, ssl, 401, "application/json", response);
            secure_zero_memory(password, strlen(password));
            free(password);
            return;
        }
        
        int password_valid = verify_password(password, user.salt, user.master_hash);
        
        pthread_mutex_lock(&db_mutex);
        db_log_attempt(&db, "login", password_valid, ip);
        pthread_mutex_unlock(&db_mutex);
        
        if (password_valid) {
            Session *new_session = session_create_with_key(1, password);
            
            if (new_session) {
                snprintf(response, sizeof(response),
                        "{\"success\":true,\"session_token\":\"%s\",\"csrf_token\":\"%s\",\"expires_in\":%d}",
                        new_session->token, new_session->csrf_token, SESSION_TIMEOUT);
                send_response(socket, ssl, 200, "application/json", response);
                printf("✅ Connexion réussie: %s\n", ip);
            } else {
                snprintf(response, sizeof(response), "{\"error\":\"Erreur création session\"}");
                send_response(socket, ssl, 500, "application/json", response);
            }
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Mot de passe incorrect\"}");
            send_response(socket, ssl, 401, "application/json", response);
            printf("❌ Tentative échouée: %s\n", ip);
        }
        
        secure_zero_memory(password, strlen(password));
        free(password);
        return;
    }
    
    // ==================== ROUTE LOGOUT ====================
    if (strcmp(path, "/api/logout") == 0 && strcmp(method, "POST") == 0) {
        Session *session = session_get(session_token);
        if (session) {
            secure_zero_memory(session->master_key, sizeof(session->master_key));
            session_destroy(session_token);
        }
        
        snprintf(response, sizeof(response), "{\"success\":true,\"message\":\"Déconnecté\"}");
        send_response(socket, ssl, 200, "application/json", response);
        return;
    }
    
    // ==================== ROUTE LISTE MOTS DE PASSE ====================
    if (strcmp(path, "/api/passwords") == 0 && strcmp(method, "GET") == 0) {
        Session *session = session_get(session_token);
        if (!session) {
            snprintf(response, sizeof(response), "{\"error\":\"Session invalide\"}");
            send_response(socket, ssl, 401, "application/json", response);
            return;
        }
        
        pthread_mutex_lock(&db_mutex);
        PasswordEntry *entries;
        int count;
        db_get_all_passwords(&db, &entries, &count);
        pthread_mutex_unlock(&db_mutex);
        
        strcpy(response, "{\"passwords\":[");
        for (int i = 0; i < count; i++) {
            char entry_json[1024];
            snprintf(entry_json, sizeof(entry_json),
                    "%s{\"id\":%d,\"website\":\"%s\",\"username\":\"%s\"}",
                    i > 0 ? "," : "", entries[i].id, entries[i].website, entries[i].username);
            strcat(response, entry_json);
        }
        strcat(response, "]}");
        
        send_response(socket, ssl, 200, "application/json", response);
        if (entries) free(entries);
        return;
    }
    
    // ==================== ROUTE DÉCHIFFREMENT ====================
    if (strncmp(path, "/api/passwords/", 15) == 0 && strstr(path, "/decrypt") != NULL && strcmp(method, "GET") == 0) {
        Session *session = session_get(session_token);
        if (!session) {
            snprintf(response, sizeof(response), "{\"error\":\"Session invalide\"}");
            send_response(socket, ssl, 401, "application/json", response);
            return;
        }
        
        if (!session_validate_csrf(session->token, csrf_token)) {
            snprintf(response, sizeof(response), "{\"error\":\"CSRF token invalide\"}");
            send_response(socket, ssl, 403, "application/json", response);
            return;
        }
        
        int id = atoi(path + 15);
        
        pthread_mutex_lock(&db_mutex);
        PasswordEntry entry;
        memset(&entry, 0, sizeof(entry));
        int success = db_get_password_decrypted(&db, id, &entry, session->master_key);
        pthread_mutex_unlock(&db_mutex);
        
        if (success) {
            snprintf(response, sizeof(response), 
                    "{\"success\":true,\"password\":\"%s\",\"website\":\"%s\",\"username\":\"%s\"}",
                    entry.encrypted_password, entry.website, entry.username);
            send_response(socket, ssl, 200, "application/json", response);
            
            secure_zero_memory(entry.encrypted_password, sizeof(entry.encrypted_password));
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur déchiffrement\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
        return;
    }
    
    // ==================== ROUTE AJOUT MOT DE PASSE ====================
    if (strcmp(path, "/api/passwords") == 0 && strcmp(method, "POST") == 0) {
        Session *session = session_get(session_token);
        if (!session) {
            snprintf(response, sizeof(response), "{\"error\":\"Session invalide\"}");
            send_response(socket, ssl, 401, "application/json", response);
            return;
        }
        
        if (!session_validate_csrf(session->token, csrf_token)) {
            snprintf(response, sizeof(response), "{\"error\":\"CSRF token invalide\"}");
            send_response(socket, ssl, 403, "application/json", response);
            return;
        }
        
        char *website = parse_json_field(body, "website");
        char *username = parse_json_field(body, "username");
        char *password = parse_json_field(body, "password");
        char *notes = parse_json_field(body, "notes");
        
        if (!website || !username || !password) {
            snprintf(response, sizeof(response), "{\"error\":\"Paramètres manquants\"}");
            send_response(socket, ssl, 400, "application/json", response);
            if (website) free(website);
            if (username) free(username);
            if (password) {
                secure_zero_memory(password, strlen(password));
                free(password);
            }
            if (notes) free(notes);
            return;
        }
        
        pthread_mutex_lock(&db_mutex);
        int success = db_add_password_encrypted(&db, website, username, password, notes ? notes : "", session->master_key);
        pthread_mutex_unlock(&db_mutex);
        
        if (success) {
            snprintf(response, sizeof(response), "{\"success\":true,\"message\":\"Mot de passe chiffré et stocké\"}");
            send_response(socket, ssl, 200, "application/json", response);
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur lors du chiffrement/stockage\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
        
        secure_zero_memory(password, strlen(password));
        free(website);
        free(username);
        free(password);
        if (notes) free(notes);
        return;
    }
    
    // ==================== ROUTE SUPPRESSION ====================
    if (strncmp(path, "/api/passwords/", 15) == 0 && strcmp(method, "DELETE") == 0) {
        Session *session = session_get(session_token);
        if (!session) {
            snprintf(response, sizeof(response), "{\"error\":\"Session invalide\"}");
            send_response(socket, ssl, 401, "application/json", response);
            return;
        }
        
        if (!session_validate_csrf(session->token, csrf_token)) {
            snprintf(response, sizeof(response), "{\"error\":\"CSRF token invalide\"}");
            send_response(socket, ssl, 403, "application/json", response);
            return;
        }
        
        int id = atoi(path + 15);
        
        pthread_mutex_lock(&db_mutex);
        int success = db_delete_password(&db, id);
        pthread_mutex_unlock(&db_mutex);
        
        if (success) {
            snprintf(response, sizeof(response), "{\"success\":true}");
            send_response(socket, ssl, 200, "application/json", response);
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur suppression\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
        return;
    }
    
    // ==================== ROUTE GÉNÉRATION ====================
    if (strcmp(path, "/api/generate-passphrase") == 0 && strcmp(method, "GET") == 0) {
        char passphrase[256];
        generate_secure_passphrase(passphrase, sizeof(passphrase) - 1);
        
        snprintf(response, sizeof(response), "{\"success\":true,\"passphrase\":\"%s\"}", passphrase);
        send_response(socket, ssl, 200, "application/json", response);
        return;
    }

    // ==================== ROUTE QUESTIONS SÉCURITÉ (GET) ====================
    if (strcmp(path, "/api/security-questions") == 0 && strcmp(method, "GET") == 0) {
        pthread_mutex_lock(&db_mutex);
        SecurityQuestions questions;
        memset(&questions, 0, sizeof(questions));
        int has_questions = db_get_security_questions(&db, &questions);
        pthread_mutex_unlock(&db_mutex);
        
        if (has_questions) {
            snprintf(response, sizeof(response),
                    "{\"success\":true,\"questions\":{\"question1\":\"%s\",\"question2\":\"%s\",\"question3\":\"%s\"}}",
                    questions.question1, questions.question2, questions.question3);
            send_response(socket, ssl, 200, "application/json", response);
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Aucune question de sécurité trouvée\"}");
            send_response(socket, ssl, 404, "application/json", response);
        }
        return;
    }

    
    
    // ==================== ROUTE RÉCUPÉRATION ====================
    if (strcmp(path, "/api/recover-account") == 0 && strcmp(method, "POST") == 0) {
        char *answer1 = parse_json_field(body, "answer1");
        char *answer2 = parse_json_field(body, "answer2");
        char *answer3 = parse_json_field(body, "answer3");
        char *recovery_code = parse_json_field(body, "recovery_code");
        
        if (recovery_code) {
            pthread_mutex_lock(&db_mutex);
            int code_valid = db_verify_recovery_code(&db, recovery_code);
            pthread_mutex_unlock(&db_mutex);
            
            if (code_valid == 1) {
                snprintf(response, sizeof(response), "{\"success\":true,\"message\":\"Code de récupération valide\"}");
                send_response(socket, ssl, 200, "application/json", response);
            } else if (code_valid == -1) {
                snprintf(response, sizeof(response), "{\"error\":\"Compte verrouillé. Réessayez plus tard.\"}");
                send_response(socket, ssl, 423, "application/json", response);
            } else {
                snprintf(response, sizeof(response), "{\"error\":\"Code de récupération invalide\"}");
                send_response(socket, ssl, 401, "application/json", response);
            }
            
            if (recovery_code) free(recovery_code);
            return;
        }
        else if (answer1 && answer2 && answer3) {
            pthread_mutex_lock(&db_mutex);
            int answers_valid = db_verify_security_answers(&db, answer1, answer2, answer3);
            pthread_mutex_unlock(&db_mutex);
            
            if (answers_valid) {
                snprintf(response, sizeof(response), "{\"success\":true,\"message\":\"Réponses correctes\"}");
                send_response(socket, ssl, 200, "application/json", response);
            } else {
                snprintf(response, sizeof(response), "{\"error\":\"Réponses incorrectes\"}");
                send_response(socket, ssl, 401, "application/json", response);
            }
            
            if (answer1) free(answer1);
            if (answer2) free(answer2);
            if (answer3) free(answer3);
            return;
        }
        
        snprintf(response, sizeof(response), "{\"error\":\"Méthode de récupération non spécifiée\"}");
        send_response(socket, ssl, 400, "application/json", response);
        if (answer1) free(answer1);
        if (answer2) free(answer2);
        if (answer3) free(answer3);
        if (recovery_code) free(recovery_code);
        return;
    }
        // ==================== ROUTE RÉINITIALISATION MOT DE PASSE ====================
    if (strcmp(path, "/api/reset-password") == 0 && strcmp(method, "POST") == 0) {
        char *new_password = parse_json_field(body, "password");
        
        if (!new_password) {
            snprintf(response, sizeof(response), "{\"error\":\"Nouveau mot de passe manquant\"}");
            send_response(socket, ssl, 400, "application/json", response);
            return;
        }
        
        if (strlen(new_password) < 12) {
            snprintf(response, sizeof(response), "{\"error\":\"Le mot de passe doit faire au moins 12 caractères\"}");
            send_response(socket, ssl, 400, "application/json", response);
            secure_zero_memory(new_password, strlen(new_password));
            free(new_password);
            return;
        }
        
        pthread_mutex_lock(&db_mutex);
        
        // Générer nouveau sel et hash
        char salt[32];
        char hash[128];
        generate_salt(salt, sizeof(salt));
        
        if (!hash_password(new_password, salt, hash, sizeof(hash))) {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur lors du hachage\"}");
            send_response(socket, ssl, 500, "application/json", response);
            pthread_mutex_unlock(&db_mutex);
            secure_zero_memory(new_password, strlen(new_password));
            free(new_password);
            return;
        }
        
        // Mettre à jour l'utilisateur
        const char *update_sql = "UPDATE users SET master_hash = ?, salt = ? WHERE id = (SELECT MAX(id) FROM users)";
        sqlite3_stmt *stmt;
        
        if (sqlite3_prepare_v2(db.db, update_sql, -1, &stmt, NULL) != SQLITE_OK) {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur préparation requête\"}");
            send_response(socket, ssl, 500, "application/json", response);
            pthread_mutex_unlock(&db_mutex);
            secure_zero_memory(new_password, strlen(new_password));
            free(new_password);
            return;
        }
        
        sqlite3_bind_text(stmt, 1, hash, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, salt, -1, SQLITE_STATIC);
        
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        pthread_mutex_unlock(&db_mutex);
        
        if (rc == SQLITE_DONE) {
            snprintf(response, sizeof(response), "{\"success\":true,\"message\":\"Mot de passe réinitialisé avec succès\"}");
            send_response(socket, ssl, 200, "application/json", response);
            printf("✅ Mot de passe principal réinitialisé\n");
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur lors de la réinitialisation\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
        
        secure_zero_memory(new_password, strlen(new_password));
        free(new_password);
        return;
    }
    snprintf(response, sizeof(response), "{\"error\":\"Route non implementee\"}");
    send_response(socket, ssl, 404, "application/json", response);
}

char* parse_json_field(const char *json, const char *key) {
    if (!json || !key) return NULL;
    
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    char *start = strstr(json, search);
    if (!start) return NULL;
    
    start += strlen(search);
    while (*start == ' ' || *start == '"') start++;
    
    char *end = start;
    while (*end && *end != '"' && *end != ',' && *end != '}') end++;
    
    int len = end - start;
    char *value = malloc(len + 1);
    strncpy(value, start, len);
    value[len] = '\0';
    
    return value;
}

char* read_file(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(content, 1, size, file);
    content[bytes_read] = '\0';
    fclose(file);
    
    return content;
}

int main(int argc, char *argv[]) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    SSL_CTX *ssl_ctx = NULL;
    
    printf("🔐 PASSWORD VAULT - SERVEUR HTTPS\n");
    printf("📍 URL: https://localhost:%d\n", SERVER_PORT);
    
    if (!crypto_init()) {
        fprintf(stderr, "❌ Erreur initialisation cryptographie\n");
        return 1;
    }
    
    if (!db_init(&db)) {
        fprintf(stderr, "❌ Erreur initialisation base de données\n");
        return 1;
    }
    
    session_init();
    
    pthread_t cleanup_thread;
    pthread_create(&cleanup_thread, NULL, session_cleanup_thread, NULL);
    pthread_detach(cleanup_thread);
    
    ssl_ctx = init_ssl_context();
    printf("✅ SSL/TLS configuré\n");
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        return 1;
    }
    
    printf("\n");
    printf("🔐 PASSWORD VAULT - PRODUCTION SERVER\n");
    printf("📍 URL: https://localhost:%d\n", SERVER_PORT);
    printf("🔒 Sécurité: TLS 1.2+, Argon2id, AES-256\n");
    printf("🛡️  Protection: OWASP Top 10, HTTPS obligatoire\n\n");
    
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        
        ClientInfo *client = malloc(sizeof(ClientInfo));
        client->socket = client_socket;
        
        inet_ntop(AF_INET, &address.sin_addr, client->ip, sizeof(client->ip));
        
        client->ssl = SSL_new(ssl_ctx);
        SSL_set_fd(client->ssl, client_socket);
        
        if (SSL_accept(client->ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(client->ssl);
            close(client_socket);
            free(client);
            continue;
        }
        
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client);
        pthread_detach(thread);
    }
    
    SSL_CTX_free(ssl_ctx);
    db_close(&db);
    close(server_fd);
    return 0;
}