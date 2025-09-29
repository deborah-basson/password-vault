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
                       const char *body, const char *ip, const char *session_token);
char* parse_json_field(const char *json, const char *key);
char* read_file(const char *filepath);

static ssize_t write_data(int socket, SSL *ssl, const void *buf, size_t len) {
    if (ssl) {
        return SSL_write(ssl, buf, len);
    }
    return write(socket, buf, len);
}

SSL_CTX* init_ssl_context() {
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
    
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    
    if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "Certificat SSL manquant\n");
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
    }
    
    SSL_CTX_set_cipher_list(ctx, CIPHER_LIST);
    
    return ctx;
}

int main(int argc, char *argv[]) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    SSL_CTX *ssl_ctx = NULL;
    int use_ssl = 1;
    
    if (argc > 1 && strcmp(argv[1], "--no-ssl") == 0) {
        use_ssl = 0;
        printf("Mode HTTP (developpement uniquement)\n");
    }
    
    if (!crypto_init()) {
        fprintf(stderr, "Erreur initialisation cryptographie\n");
        return 1;
    }
    
    if (!db_init(&db)) {
        fprintf(stderr, "Erreur initialisation base de donnees\n");
        return 1;
    }
    
    session_init();
    
    pthread_t cleanup_thread;
    pthread_create(&cleanup_thread, NULL, session_cleanup_thread, NULL);
    pthread_detach(cleanup_thread);
    
    if (use_ssl) {
        ssl_ctx = init_ssl_context();
        printf("SSL/TLS configure (TLS 1.3)\n");
    }
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(use_ssl ? SERVER_PORT : 8080);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        return 1;
    }
    
    printf("\n");
    printf("PASSWORD VAULT - PRODUCTION SERVER\n");
    printf("URL: %s://localhost:%d\n", use_ssl ? "https" : "http", use_ssl ? SERVER_PORT : 8080);
    printf("Securite: %s\n", use_ssl ? "TLS 1.3, Argon2id, AES-256" : "DEV MODE - NO SSL");
    printf("Protection: OWASP Top 10\n\n");
    
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        
        ClientInfo *client = malloc(sizeof(ClientInfo));
        client->socket = client_socket;
        client->ssl = NULL;
        
        inet_ntop(AF_INET, &address.sin_addr, client->ip, sizeof(client->ip));
        
        if (use_ssl) {
            client->ssl = SSL_new(ssl_ctx);
            SSL_set_fd(client->ssl, client_socket);
            
            if (SSL_accept(client->ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_free(client->ssl);
                close(client_socket);
                free(client);
                continue;
            }
        }
        
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client);
        pthread_detach(thread);
    }
    
    if (use_ssl) SSL_CTX_free(ssl_ctx);
    db_close(&db);
    close(server_fd);
    return 0;
}

void* handle_client(void* arg) {
    ClientInfo *client = (ClientInfo*)arg;
    char buffer[BUFFER_SIZE] = {0};
    int bytes;
    
    if (client->ssl) {
        bytes = SSL_read(client->ssl, buffer, BUFFER_SIZE - 1);
    } else {
        bytes = read(client->socket, buffer, BUFFER_SIZE - 1);
    }
    
    if (bytes <= 0) {
        goto cleanup;
    }
    
    buffer[bytes] = '\0';
    
    char method[16] = {0}, path[512] = {0}, body[BUFFER_SIZE] = {0};
    sscanf(buffer, "%15s %511s", method, path);
    
    char session_token[128] = {0};
    char *auth_header = strstr(buffer, "Authorization: Bearer ");
    if (auth_header) {
        sscanf(auth_header + 22, "%127s", session_token);
    }
    
    char *body_start = strstr(buffer, "\r\n\r\n");
    if (body_start) {
        strncpy(body, body_start + 4, BUFFER_SIZE - 1);
    }
    
    printf("[%s] %s %s\n", client->ip, method, path);
    
    if (strncmp(path, "/api/", 5) == 0) {
        handle_api_request(client->socket, client->ssl, method, path, body, client->ip, session_token);
    } else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        send_file(client->socket, client->ssl, "web/index.html");
    } else if (strstr(path, ".html") || strstr(path, ".css") || strstr(path, ".js")) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "web%s", path);
        send_file(client->socket, client->ssl, filepath);
    } else {
        send_response(client->socket, client->ssl, 404, "text/plain", "404 Not Found");
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

void send_response(int socket, SSL *ssl, int status, const char *content_type, const char *body) {
    char header[2048];
    const char *status_text = (status == 200) ? "OK" : 
                             (status == 400) ? "Bad Request" :
                             (status == 401) ? "Unauthorized" :
                             (status == 429) ? "Too Many Requests" :
                             (status == 404) ? "Not Found" : "Error";
    
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_text, content_type, strlen(body));
    
    write_data(socket, ssl, header, strlen(header));
    write_data(socket, ssl, body, strlen(body));
}

void send_file(int socket, SSL *ssl, const char *filepath) {
    char *content = read_file(filepath);
    if (!content) {
        send_response(socket, ssl, 404, "text/plain", "File not found");
        return;
    }
    
    const char *content_type = "text/html";
    if (strstr(filepath, ".css")) content_type = "text/css";
    if (strstr(filepath, ".js")) content_type = "application/javascript";
    
    send_response(socket, ssl, 200, content_type, content);
    free(content);
}

void handle_api_request(int socket, SSL *ssl, const char *method, const char *path, 
                       const char *body, const char *ip, const char *session_token) {
    char response[BUFFER_SIZE];
    
    pthread_mutex_lock(&db_mutex);
    int failed = db_get_failed_attempts(&db, ip, 15);
    pthread_mutex_unlock(&db_mutex);
    
    if (failed >= MAX_LOGIN_ATTEMPTS) {
        snprintf(response, sizeof(response),
                "{\"error\":\"Trop de tentatives. Reessayez dans 15 minutes.\"}");
        send_response(socket, ssl, 429, "application/json", response);
        return;
    }
    
    if (strcmp(method, "OPTIONS") == 0) {
        send_response(socket, ssl, 200, "text/plain", "");
        return;
    }
    
    if (strcmp(path, "/api/status") == 0 && strcmp(method, "GET") == 0) {
        pthread_mutex_lock(&db_mutex);
        UserAccount user;
        int has_account = db_get_user(&db, &user);
        pthread_mutex_unlock(&db_mutex);
        
        snprintf(response, sizeof(response), 
                "{\"has_account\":%s}", has_account ? "true" : "false");
        send_response(socket, ssl, 200, "application/json", response);
        return;
    }
    
    if (strcmp(path, "/api/register") == 0 && strcmp(method, "POST") == 0) {
        char *password = parse_json_field(body, "password");
        
        if (!password || !validate_input(password, 128) || !validate_password_strength(password)) {
            snprintf(response, sizeof(response),
                    "{\"error\":\"Mot de passe invalide (min 12 caracteres, 3 types requis)\"}");
            send_response(socket, ssl, 400, "application/json", response);
            if (password) free(password);
            return;
        }
        
        pthread_mutex_lock(&db_mutex);
        UserAccount existing;
        int exists = db_get_user(&db, &existing);
        pthread_mutex_unlock(&db_mutex);
        
        if (exists) {
            snprintf(response, sizeof(response), "{\"error\":\"Un compte existe deja\"}");
            send_response(socket, ssl, 400, "application/json", response);
            secure_zero_memory(password, strlen(password));
            free(password);
            return;
        }
        
        char salt[SALT_LENGTH];
        char hash[HASH_LENGTH];
        char totp_secret[TOTP_SECRET_LENGTH];
        
        generate_salt(salt, SALT_LENGTH);
        hash_password(password, salt, hash, sizeof(hash));
        generate_totp_secret(totp_secret, TOTP_SECRET_LENGTH);
        
        pthread_mutex_lock(&db_mutex);
        int success = db_create_user(&db, hash, salt, totp_secret);
        db_log_attempt(&db, "register", success, ip);
        pthread_mutex_unlock(&db_mutex);
        
        if (success) {
            snprintf(response, sizeof(response),
                    "{\"success\":true,\"totp_secret\":\"%s\",\"message\":\"Compte cree avec succes\"}", 
                    totp_secret);
            send_response(socket, ssl, 200, "application/json", response);
            printf("Nouveau compte cree depuis %s\n", ip);
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur serveur\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
        
        secure_zero_memory(password, strlen(password));
        free(password);
        return;
    }
    
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
            snprintf(response, sizeof(response), "{\"error\":\"Aucun compte trouve\"}");
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
            Session *new_session = session_create(user.id);
            
            if (new_session) {
                snprintf(response, sizeof(response),
                        "{\"success\":true,\"session_token\":\"%s\",\"expires_in\":%d}",
                        new_session->token, SESSION_TIMEOUT);
                send_response(socket, ssl, 200, "application/json", response);
                printf("Connexion reussie: %s\n", ip);
            } else {
                snprintf(response, sizeof(response), "{\"error\":\"Erreur creation session\"}");
                send_response(socket, ssl, 500, "application/json", response);
            }
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Mot de passe incorrect\"}");
            send_response(socket, ssl, 401, "application/json", response);
            printf("Tentative echouee: %s\n", ip);
        }
        
        secure_zero_memory(password, strlen(password));
        free(password);
        return;
    }
    
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
    
    if (strcmp(path, "/api/passwords") == 0 && strcmp(method, "POST") == 0) {
        Session *session = session_get(session_token);
        if (!session) {
            snprintf(response, sizeof(response), "{\"error\":\"Session invalide\"}");
            send_response(socket, ssl, 401, "application/json", response);
            return;
        }
        
        char *website = parse_json_field(body, "website");
        char *username = parse_json_field(body, "username");
        char *password = parse_json_field(body, "password");
        
        if (!website || !username || !password) {
            snprintf(response, sizeof(response), "{\"error\":\"Parametres manquants\"}");
            send_response(socket, ssl, 400, "application/json", response);
            if (website) free(website);
            if (username) free(username);
            if (password) free(password);
            return;
        }
        
        pthread_mutex_lock(&db_mutex);
        int success = db_add_password(&db, website, username, password, "");
        pthread_mutex_unlock(&db_mutex);
        
        if (success) {
            snprintf(response, sizeof(response), "{\"success\":true}");
            send_response(socket, ssl, 200, "application/json", response);
        } else {
            snprintf(response, sizeof(response), "{\"error\":\"Erreur\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
        
        secure_zero_memory(password, strlen(password));
        free(website);
        free(username);
        free(password);
        return;
    }
    
    if (strncmp(path, "/api/passwords/", 15) == 0 && strcmp(method, "DELETE") == 0) {
        Session *session = session_get(session_token);
        if (!session) {
            snprintf(response, sizeof(response), "{\"error\":\"Session invalide\"}");
            send_response(socket, ssl, 401, "application/json", response);
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
            snprintf(response, sizeof(response), "{\"error\":\"Erreur\"}");
            send_response(socket, ssl, 500, "application/json", response);
        }
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