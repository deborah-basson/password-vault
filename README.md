# Password Vault

## Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Architecture](#architecture)
4. [Installation](#installation)
5. [Usage](#usage)
6. [Testing](#testing)
7. [Security](#security)
8. [Contributors](#contributors)

---

## Overview
This **password vault** is a secure password management system developed as part of the **Secure Development** course at EPITA. The project is written in **C** and provides a web-based interface for securely storing and managing passwords with enterprise-grade encryption.

---

## Features

### Core Functionality
- Secure storage of passwords with AES-256-GCM encryption
- Master password-based authentication
- Two-Factor Authentication (2FA) with TOTP
- Add, edit, delete, and list password entries
- Password strength validation
- Secure password generation
- Session management with CSRF protection

### Security Features
- AES-256-GCM encryption with key derivation
- HTTPS/TLS support with SSL certificates
- CSRF token validation
- Rate limiting on login attempts
- SQL injection prevention with prepared statements
- XSS protection with HTML sanitization
- Secure memory handling with libsodium
- Argon2id password hashing

### Web Interface
- Modern, responsive web UI
- RESTful API server
- Session-based authentication
- Real-time password strength indicator

---

## Architecture

### Components
- **Database Layer**: SQLite with WAL mode for concurrent access
- **Crypto Layer**: libsodium for encryption, hashing, and TOTP
- **Session Manager**: In-memory session store with CSRF tokens
- **API Server**: HTTP/HTTPS server with JSON endpoints
- **Web Frontend**: HTML5/CSS3/JavaScript single-page application

### File Structure
```
password-vault/
├── src/
│   ├── api_server.c      # HTTP/HTTPS server and API endpoints
│   ├── database.c        # Database operations and queries
│   ├── crypto.c          # Encryption, hashing, TOTP
│   └── session.c         # Session management
├── include/
│   ├── config.h          # Configuration constants
│   ├── database.h        # Database interface
│   ├── crypto.h          # Crypto interface
│   └── session.h         # Session interface
├── tests/
│   ├── test_crypto.c     # Crypto unit tests
│   ├── test_database.c   # Database unit tests
│   ├── test_session.c    # Session unit tests
│   └── test_integration.c # Integration tests
├── web/
│   ├── index.html        # Web interface
│   ├── app.js            # Frontend logic
│   └── style.css         # Styling
└── sql/
    └── schema.sql        # Database schema
```

---

## Installation

### Prerequisites
- GCC compiler
- SQLite3 development libraries
- libsodium development libraries
- OpenSSL development libraries
- Criterion testing framework (for tests)

### Install Dependencies
```bash
make install
```

Or manually:
```bash
sudo apt-get update
sudo apt-get install -y libsqlite3-dev libsodium-dev libssl-dev libcriterion-dev
```

### Build
```bash
git clone https://github.com/deborah-basson/password-vault.git
cd password-vault
make
```

### Generate SSL Certificates
```bash
make ssl
```

---

## Usage

### Start Server
```bash
make run
```

The server will start on `https://localhost:8443`

### Development Mode (HTTP)
```bash
make run-dev
```

### Access Web Interface
Open your browser and navigate to:
- Production: `https://localhost:8443`
- Development: `http://localhost:8080`

### API Endpoints

#### Authentication
- `POST /api/register` - Create new user account
- `POST /api/login` - Login with master password
- `POST /api/verify-totp` - Verify 2FA code
- `POST /api/logout` - Logout and destroy session

#### Password Management
- `GET /api/passwords` - List all passwords
- `POST /api/passwords` - Add new password
- `PUT /api/passwords/:id` - Update password
- `DELETE /api/passwords/:id` - Delete password

---

## Testing

### Run All Tests
```bash
make test
```

### Test Suites
- **Crypto Tests**: Encryption, hashing, TOTP, password validation
- **Database Tests**: CRUD operations, user management, queries
- **Session Tests**: Session creation, CSRF validation, token management
- **Integration Tests**: End-to-end workflows, multi-user scenarios

### Test Results
All 41 tests passing:
- 12 crypto tests
- 13 database tests
- 9 session tests
- 7 integration tests

---

## Security

### Encryption
- AES-256-GCM for password encryption
- Unique nonce per encryption operation
- Key derivation using BLAKE2b
- Base64 encoding for database storage

### Password Hashing
- Argon2id algorithm
- Configurable work factors
- Unique salt per user

### Session Security
- Cryptographically secure random tokens
- CSRF token validation
- Session expiration (24 hours)
- Secure session storage

### Database Security
- Prepared statements (SQL injection prevention)
- Foreign key constraints
- Write-Ahead Logging (WAL) mode
- Audit logging of failed login attempts

### Network Security
- HTTPS/TLS 1.2+ required in production
- Certificate-based encryption
- Rate limiting on authentication endpoints

---

## Maintenance

### Clean Build Files
```bash
make clean
```

### Reset Database
```bash
make reset
```

### Check Server Status
```bash
make status
```

### Stop Server
```bash
make kill
```

---

## Contributors

| Name       | Email                           |
|------------|---------------------------------|
| Deborah Basson | [deborah.basson@epita.fr](mailto:deborah.basson@epita.fr) |
| Louise Grosdhomme | [louise.grosdhomme@epita.fr](mailto:louise.grosdhomme@epita.fr) |
| Alida Nana-Sunji-Kouanang | [alida.nana-sunji-kouanang@epita.fr](mailto:alida.nana-sunji-kouanang@epita.fr) |
| Maimouna Tall | [maimouna.tall@epita.fr](mailto:maimouna.tall@epita.fr) |

---

## License

This project is developed for educational purposes as part of the EPITA curriculum.
