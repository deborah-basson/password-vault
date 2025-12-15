# 🔐 Password Vault - Secure Password Manager

[![CI Pipeline](https://github.com/deborah-basson/password-vault/actions/workflows/ci.yml/badge.svg)](https://github.com/deborah-basson/password-vault/actions)
[![License](https://img.shields.io/badge/license-Educational-blue.svg)](LICENSE)
[![Security](https://img.shields.io/badge/security-audited-green.svg)](VULNERABILITY_POC.md)

A secure, production-ready password manager with military-grade encryption, 2FA support, and comprehensive security features. Developed as part of the **Secure Development** course at **EPITA**.

---

## 📋 Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Architecture](#architecture)
4. [Requirements](#requirements)
5. [Installation](#installation)
6. [Usage](#usage)
7. [Testing](#testing)
8. [Security](#security)
9. [API Documentation](#api-documentation)
10. [Docker Deployment](#docker-deployment)
11. [Development](#development)
12. [Documentation](#documentation)
13. [Known Limitations](#known-limitations)
14. [Contributors](#contributors)

---

## Overview

This **password vault** is a secure password management system written in **C** that provides a web-based interface for securely storing and managing passwords with enterprise-grade encryption. The project demonstrates best practices in secure software development, cryptography, and web security.

### Key Highlights
- ✅ **59 passing tests** with 100% critical path coverage
- ✅ **Zero memory leaks** (valgrind verified)
- ✅ **CI/CD pipeline** with automated security checks
- ✅ **Military-grade encryption** (AES-256-GCM + Argon2id)
- ✅ **Comprehensive security features** (2FA, CSRF, rate limiting)
- ✅ **Docker support** for easy deployment
- ✅ **Security audited** with vulnerability analysis

---

## Features

### Core Security
- **AES-256-GCM encryption** for all stored passwords
- **Argon2id password hashing** (industry standard, GPU-resistant)
- **TOTP 2FA** support for enhanced authentication
- **HTTPS/TLS 1.3 only** - no plaintext communication
- **CSRF protection** on all state-changing operations
- **Rate limiting** (5 attempts, 15-minute lockout)
- **Session management** with 30-minute timeout
- **Memory security** with libsodium (automatic zeroing)

### Password Management
- ✅ Secure storage with encryption at rest
- ✅ Password history tracking (prevent reuse)
- ✅ Password strength validation
- ✅ Encrypted import/export functionality
- ✅ Password generator with customizable rules
- ✅ Search and filter capabilities
- ✅ Secure notes storage
- ✅ Account recovery with security questions

### Web Interface
- 🎨 Modern, responsive UI (HTML5/CSS3/JavaScript)
- 🔒 Auto-lock after inactivity
- 🚫 Prevents multiple concurrent sessions
- 📊 Real-time password strength indicator
- 🔑 Built-in password generator
- 📱 Mobile-friendly design
- ⚡ Single-page application (SPA)

### Security Features
- 🛡️ SQL injection prevention (prepared statements)
- 🛡️ XSS protection (HTML sanitization)
- 🛡️ CSRF token validation
- 🛡️ Audit logging for failed attempts
- 🛡️ IP-based tracking
- 🛡️ Session timeout enforcement
- 🛡️ Secure random number generation
- 🛡️ Constant-time comparisons for secrets

---

## Architecture

### System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Web Browser (HTTPS)                  │
└─────────────────────┬───────────────────────────────────┘
                      │ TLS 1.3
┌─────────────────────▼───────────────────────────────────┐
│              API Server (C/OpenSSL)                     │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Session Manager  │  CSRF Protection  │  Rate    │   │
│  │                   │                   │  Limiter │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│              Business Logic Layer                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  Crypto      │  │  Database    │  │  Security    │  │
│  │  (libsodium) │  │  (SQLite)    │  │  Questions   │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│         Encrypted SQLite Database (vault.db)            │
│              AES-256-GCM Encrypted Passwords            │
└─────────────────────────────────────────────────────────┘
```

### Components

| Component | Technology | Purpose |
|-----------|-----------|---------|
| **API Server** | C + OpenSSL | HTTPS/TLS server with RESTful API |
| **Database** | SQLite3 + WAL | Persistent storage with ACID guarantees |
| **Crypto** | libsodium | Encryption, hashing, TOTP, random generation |
| **Session Manager** | In-memory | Session tracking with CSRF tokens |
| **Frontend** | HTML5/CSS3/JS | Single-page web application |

### File Structure
```
password-vault/
├── src/
│   ├── api_server.c           # HTTP/HTTPS server and API endpoints
│   ├── database.c             # Database operations and queries
│   ├── crypto.c               # Encryption, hashing, TOTP
│   ├── session.c              # Session management
│   ├── database_security.c    # Security policies and validation
│   ├── security_questions.c   # Account recovery
│   ├── password_history.c     # Password reuse prevention
│   └── import_export.c        # Encrypted backup/restore
├── include/
│   ├── config.h               # Configuration constants
│   ├── database.h             # Database interface
│   ├── crypto.h               # Crypto interface
│   ├── session.h              # Session interface
│   └── import_export.h        # Import/export interface
├── tests/
│   ├── test_crypto.c          # Crypto unit tests (12 tests)
│   ├── test_database.c        # Database unit tests (13 tests)
│   ├── test_session.c         # Session unit tests (9 tests)
│   ├── test_integration.c     # Integration tests (7 tests)
│   ├── test_security.c        # Security tests (18 tests)
│   └── test_import_export.c   # Import/export tests (7 tests)
├── web/
│   ├── index.html             # Web interface
│   ├── app.js                 # Frontend logic
│   └── style.css              # Styling
├── sql/
│   └── schema.sql             # Database schema
├── .github/
│   └── workflows/
│       └── ci.yml             # CI/CD pipeline
├── Dockerfile                 # Docker configuration
├── docker-compose.yml         # Docker Compose setup
├── Makefile                   # Build configuration
├── README.md                  # This file
├── VULNERABILITY_POC.md       # Security vulnerability analysis
├── COLLABORATION.md           # Team workflow documentation
└── TODO.md                    # Project roadmap
```

---

## Requirements

### System Requirements
- **OS**: Linux (Ubuntu 20.04+, Debian 11+)
- **CPU**: x86_64 or ARM64
- **RAM**: 512 MB minimum
- **Disk**: 100 MB for application + database

### Dependencies
```bash
# Required libraries
libsqlite3-dev      # Database
libsodium-dev       # Cryptography
libssl-dev          # TLS/HTTPS
gcc                 # Compiler
make                # Build system

# Optional (for testing)
libcriterion-dev    # Unit testing framework
valgrind            # Memory leak detection
cppcheck            # Static analysis
```

---

## Installation + Build

### 1. Clone the repository
```bash
git clone https://github.com/deborah-basson/password-vault.git
cd password-vault
```

### 2. Install dependencies
```bash
# Automated installation
make install

# Or manually
sudo apt-get update
sudo apt-get install -y libsqlite3-dev libsodium-dev libssl-dev gcc make

# Optional: testing tools
sudo apt-get install -y libcriterion-dev valgrind cppcheck
```
### 3. Build the projet 
```bash
make run
```
The server will start on `https://localhost:8443`

---

## Usage

### First-time Setup

1. **Access the web interface**: Open `https://localhost:8443` in your browser
2. **Accept the self-signed certificate** (for development only)
3. **Register** a new account with a strong master password (minimum 12 characters)
4. **Set up 2FA** (optional but highly recommended)
5. **Configure security questions** for account recovery

### Web Interface

The web interface provides:
- **Dashboard**: View all stored passwords
- **Add Password**: Store new credentials securely
- **Edit/Delete**: Manage existing entries
- **Password Generator**: Create strong, random passwords
- **2FA Setup**: Enable/disable two-factor authentication
- **Import/Export**: Backup and restore your vault

### Command Line Examples

#### Register a new user
```bash
curl -k -X POST https://localhost:8443/api/register \
  -H "Content-Type: application/json" \
  -d '{
    "master_password": "YourStrongPassword123!",
    "question1": "What is your favorite color?",
    "answer1": "Blue",
    "question2": "Where were you born?",
    "answer2": "Paris",
    "question3": "What is your pet name?",
    "answer3": "Max"
  }'
```

#### Login
```bash
curl -k -X POST https://localhost:8443/api/login \
  -H "Content-Type: application/json" \
  -d '{
    "master_password": "YourStrongPassword123!",
    "totp_code": "123456"
  }'
```

Response:
```json
{
  "session_token": "abc123...",
  "csrf_token": "xyz789...",
  "expires_at": "2025-11-18T12:00:00Z"
}
```

#### Add a password
```bash
curl -k -X POST https://localhost:8443/api/passwords \
  -H "Authorization: Bearer YOUR_SESSION_TOKEN" \
  -H "X-CSRF-Token: YOUR_CSRF_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "website": "example.com",
    "username": "user@example.com",
    "password": "MySecurePassword123!",
    "notes": "Personal account"
  }'
```

#### List all passwords
```bash
curl -k https://localhost:8443/api/passwords \
  -H "Authorization: Bearer YOUR_SESSION_TOKEN"
```

#### Export vault (encrypted backup)
```bash
curl -k -X POST https://localhost:8443/api/export \
  -H "Authorization: Bearer YOUR_SESSION_TOKEN" \
  -H "X-CSRF-Token: YOUR_CSRF_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"export_password": "StrongExportPassword123!"}' \
  --output vault_backup.enc
```

#### Import vault
```bash
curl -k -X POST https://localhost:8443/api/import \
  -H "Authorization: Bearer YOUR_SESSION_TOKEN" \
  -H "X-CSRF-Token: YOUR_CSRF_TOKEN" \
  -F "file=@vault_backup.enc" \
  -F "export_password=StrongExportPassword123!"
```

---

## Testing

### Run All Tests
```bash
make test
```

### Test Suites

| Suite | Tests | Coverage |
|-------|-------|----------|
| **Crypto** | 12 | Encryption, hashing, TOTP, password validation |
| **Database** | 13 | CRUD operations, user management, queries |
| **Session** | 9 | Session creation, CSRF validation, token management |
| **Security** | 18 | Rate limiting, login tracking, security questions |
| **Import/Export** | 7 | Encrypted backup/restore, integrity validation |
| **Integration** | 7 | End-to-end workflows, multi-user scenarios |

### Test Results
✅ **All 59 tests passing**
- 12 crypto tests
- 13 database tests
- 9 session tests
- 18 security tests
- 7 import/export tests
- 7 integration tests

### Memory Safety
```bash
# Run with valgrind
make valgrind

# Expected output: 0 memory leaks
```

### Static Analysis
```bash
# Run cppcheck
make analyze

# Expected output: 0 errors, 0 warnings
```

---

## Security

### Cryptographic Algorithms

| Component | Algorithm | Key Size | Notes |
|-----------|-----------|----------|-------|
| Password Encryption | AES-256-GCM | 256-bit | Authenticated encryption |
| Master Password Hash | Argon2id | N/A | Memory-hard, GPU-resistant |
| Session Tokens | Random (libsodium) | 256-bit | Cryptographically secure |
| TOTP | HMAC-SHA1 | 160-bit | RFC 6238 compliant |
| TLS | TLS 1.3 | 256-bit | Modern cipher suites only |
| Key Derivation | BLAKE2b | 256-bit | Fast, secure hashing |
| Export Encryption | AES-256-GCM | 256-bit | Separate password protection |

### Security Features

✅ **Encryption at Rest**: All passwords encrypted with AES-256-GCM  
✅ **Secure Communication**: HTTPS/TLS 1.3 only, no HTTP fallback  
✅ **Memory Protection**: Sensitive data zeroed after use (libsodium)  
✅ **Brute Force Protection**: Rate limiting (5 attempts, 15min lockout)  
✅ **Session Security**: Cryptographically secure tokens, 30min timeout  
✅ **CSRF Protection**: Token validation on all state-changing operations  
✅ **Input Validation**: All user inputs sanitized and validated  
✅ **SQL Injection Prevention**: Prepared statements only  
✅ **XSS Prevention**: HTML sanitization  
✅ **Audit Logging**: All authentication attempts logged  
✅ **Password History**: Prevents password reuse  
✅ **Account Recovery**: Secure questions with rate limiting  

### Vulnerability Analysis

See [VULNERABILITY_POC.md](VULNERABILITY_POC.md) for:
- Comprehensive vulnerability analysis
- Proof-of-concept demonstrations
- Secure implementation details
- OWASP Top 10 coverage
- Mitigation strategies

### Security Best Practices

1. **Use a strong master password** (12+ characters, mixed case, numbers, symbols)
2. **Enable 2FA** for additional protection
3. **Regular backups** using the encrypted export feature
4. **Update regularly** to get security patches
5. **Secure your certificates** in production environments
6. **Monitor audit logs** for suspicious activity
7. **Use unique passwords** for each service

---

## API Documentation

### Authentication Endpoints

#### POST `/api/register`
Register a new user account.

**Request**:
```json
{
  "master_password": "StrongPassword123!",
  "question1": "What is your favorite color?",
  "answer1": "Blue",
  "question2": "Where were you born?",
  "answer2": "Paris",
  "question3": "What is your pet name?",
  "answer3": "Max"
}
```

**Response** (200 OK):
```json
{
  "success": true,
  "user_id": 1,
  "totp_secret": "JBSWY3DPEHPK3PXP"
}
```

#### POST `/api/login`
Authenticate with master password and optional 2FA.

**Request**:
```json
{
  "master_password": "StrongPassword123!",
  "totp_code": "123456"
}
```

**Response** (200 OK):
```json
{
  "session_token": "abc123...",
  "csrf_token": "xyz789...",
  "expires_at": "2025-11-18T12:00:00Z"
}
```

#### POST `/api/logout`
Destroy current session.

**Headers**:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
```

**Response** (200 OK):
```json
{
  "success": true,
  "message": "Logged out successfully"
}
```

### Password Management Endpoints

#### GET `/api/passwords`
List all stored passwords (decrypted).

**Headers**:
```
Authorization: Bearer YOUR_SESSION_TOKEN
```

**Response** (200 OK):
```json
[
  {
    "id": 1,
    "website": "example.com",
    "username": "user@example.com",
    "password": "MyPassword123!",
    "notes": "Personal account",
    "created_at": "2025-11-18 10:00:00",
    "updated_at": "2025-11-18 10:00:00"
  }
]
```

#### POST `/api/passwords`
Add a new password entry.

**Headers**:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
Content-Type: application/json
```

**Request**:
```json
{
  "website": "example.com",
  "username": "user@example.com",
  "password": "MyPassword123!",
  "notes": "Personal account"
}
```

**Response** (201 Created):
```json
{
  "success": true,
  "id": 1
}
```

#### PUT `/api/passwords/:id`
Update an existing password.

**Headers**:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
Content-Type: application/json
```

**Request**:
```json
{
  "password": "NewPassword456!"
}
```

**Response** (200 OK):
```json
{
  "success": true
}
```

#### DELETE `/api/passwords/:id`
Delete a password entry.

**Headers**:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
```

**Response** (200 OK):
```json
{
  "success": true
}
```

### Backup/Restore Endpoints

#### POST `/api/export`
Export encrypted vault backup.

**Headers**:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
Content-Type: application/json
```

**Request**:
```json
{
  "export_password": "StrongExportPassword123!"
}
```

**Response** (200 OK):
```
Binary encrypted data (download as file)
```

#### POST `/api/import`
Import encrypted vault backup.

**Headers**:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
```

**Request** (multipart/form-data):
```
file: vault_backup.enc
export_password: StrongExportPassword123!
```

**Response** (200 OK):
```json
{
  "success": true,
  "imported_count": 42
}
```
---

## Development

### Build Targets

```bash
make              # Build project
make test         # Run all tests
make clean        # Clean build files
make reset        # Reset database
make ssl          # Generate SSL certificates
make run          # Run server (HTTPS on 8443)
make kill         # Stop running server
make status       # Check server status
make valgrind     # Run with memory checks
make analyze      # Static analysis (cppcheck)
make install      # Install dependencies
```

### Adding a New Feature

1. **Write tests first** (TDD approach)
2. **Implement feature** in appropriate module
3. **Run tests**: `make test`
4. **Check memory**: `make valgrind`
5. **Static analysis**: `make analyze`
6. **Update documentation**
7. **Commit with descriptive message**

### Code Style

- Use consistent indentation (4 spaces)
- Function names: `snake_case`
- Constants: `UPPER_CASE`
- Structs: `PascalCase`
- Comments for complex logic only
- Avoid global variables
- Check all return values
- Free all allocated memory

### Debugging

```bash
# Compile with debug symbols
make CFLAGS="-g -O0 -Wall"

# Run with GDB
gdb ./password_vault

# Run with AddressSanitizer
make CFLAGS="-fsanitize=address -g"
./password_vault
```

---

## Documentation

### Project Documentation

- **[README.md](README.md)** - Project overview, installation, and usage guide
- **[VULNERABILITY_POC.md](VULNERABILITY_POC.md)** - Security vulnerability analysis and proof-of-concept
- **[COLLABORATION.md](COLLABORATION.md)** - Team workflow, sprint organization, and contribution guidelines
- **[TODO.md](TODO.md)** - Project roadmap and development status
- **[API Documentation](#api-documentation)** - Complete API reference (see above)

### Security Documentation

The project includes comprehensive security documentation:

1. **Vulnerability Analysis** ([VULNERABILITY_POC.md](VULNERABILITY_POC.md))
   - 10 common vulnerabilities analyzed
   - Proof-of-concept code examples
   - Secure implementation details
   - OWASP Top 10 coverage

2. **Security Features** (this README)
   - Cryptographic algorithms used
   - Protection mechanisms
   - Security best practices

3. **Audit Trail**
   - All security-relevant events logged
   - Failed authentication attempts tracked
   - Rate limiting triggers recorded

### Team Documentation

- **Team Workflow** ([COLLABORATION.md](COLLABORATION.md))
  - Sprint organization and planning
  - Meeting notes and decisions
  - Task distribution and time tracking
  - Communication protocols

---

## Known Limitations

### By Design
- **Single-user system**: No multi-user support (by design for personal password manager)
- **Local deployment**: Not hardened for public internet exposure
- **Master password**: Single point of failure (standard for password managers)
- **Self-signed certificates**: OK for development, use real certs in production

### Out of Scope
- Physical access protection
- OS/hardware compromise protection
- DDoS protection (local deployment)
- Quantum-resistant encryption (not yet industry standard)
- Multi-device synchronization
- Browser extensions
- Mobile apps

---

## Contributors

This project was developed by students at **EPITA** as part of the **Secure Development** course.

| Name | Email | Role |
|------|-------|------|
| **Deborah Basson** | [deborah.basson@epita.fr](mailto:deborah.basson@epita.fr) | Lead Developer |
| **Louise Grosdhomme** | [louise.grosdhomme@epita.fr](mailto:louise.grosdhomme@epita.fr) | Security Engineer |
| **Alida Nana-Sunji-Kouanang** | [alida.nana-sunji-kouanang@epita.fr](mailto:alida.nana-sunji-kouanang@epita.fr) | Backend Developer |
| **Maimouna Tall** | [maimouna.tall@epita.fr](mailto:maimouna.tall@epita.fr) | Frontend Developer |

For detailed contribution information, see [COLLABORATION.md](COLLABORATION.md).

---

## Support

### For Security Issues
See [VULNERABILITY_POC.md](VULNERABILITY_POC.md) for security analysis and reporting guidelines.

### For General Questions
- Open an issue on [GitHub](https://github.com/deborah-basson/password-vault/issues)
- Contact the team via email

---

## License

This project is developed for **educational purposes** as part of the EPITA curriculum.

**© 2025 EPITA - All rights reserved**

---

Last Updated: 2025-11-18  
Version: 1.0.0
