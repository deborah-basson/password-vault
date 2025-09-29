#ifndef CONFIG_H
#define CONFIG_H

// Configuration serveur
#define SERVER_PORT 8443
#define MAX_CLIENTS 50
#define BUFFER_SIZE 16384
#define SESSION_TIMEOUT 1800  // 30 minutes

// Sécurité
#define MAX_LOGIN_ATTEMPTS 5
#define LOCKOUT_DURATION 900  // 15 minutes
#define MIN_PASSWORD_LENGTH 12
#define BCRYPT_COST 12

// Chemins
#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"
#define WEB_ROOT "web"

// TLS Configuration
#define MIN_TLS_VERSION TLS1_3_VERSION
#define CIPHER_LIST "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-CHACHA20-POLY1305"

#endif