# Password Vault - Secure Password Manager

[![CI Pipeline](https://github.com/deborah-basson/password-vault/actions/workflows/ci.yml/badge.svg)](https://github.com/deborah-basson/password-vault/actions)
[![License](https://img.shields.io/badge/license-Educational-blue.svg)](LICENSE)
[![Security](https://img.shields.io/badge/security-audited-green.svg)](VULNERABILITY_POC.md)

A secure password management system written in C with web-based interface, implementing enterprise-grade encryption, 2FA support, and comprehensive security features. Developed as part of the Secure Development course at EPITA.

---

## Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Architecture](#architecture)
4. [Requirements](#requirements)
5. [Installation](#installation)
6. [Usage](#usage)
7. [Testing](#testing)
8. [Security](#security)
9. [API Documentation](#api-documentation)
10. [Development](#development)
11. [Documentation](#documentation)
12. [Known Limitations](#known-limitations)
13. [Contributors](#contributors)

---

## Overview

This password vault v2 is a secure password management system written in C that provides a web-based interface for securely storing and managing passwords with enterprise-grade encryption. The project demonstrates secure software development practices, cryptography implementation, and web security.

### Technical Highlights
- 66 passing tests with complete critical path coverage
- Zero memory leaks (valgrind verified)
- CI/CD pipeline with automated security checks
- AES-256-GCM encryption with Argon2id password hashing
- Comprehensive security features (2FA, CSRF, rate limiting)
- Security audited with vulnerability analysis
- Password history tracking and reuse prevention

---

## Features

### Core Security
- AES-256-GCM encryption for all stored passwords
- Argon2id password hashing (industry standard, GPU-resistant)
- TOTP 2FA support for enhanced authentication
- HTTPS/TLS 1.3 only - no plaintext communication
- CSRF protection on all state-changing operations
- Rate limiting (5 attempts, 15-minute lockout)
- Session management with 30-minute timeout
- Memory security with libsodium (automatic zeroing)
- Constant-time comparisons to prevent timing attacks
- Secure random number generation for all cryptographic operations

### Password Management
- Secure storage with encryption at rest
- Password history tracking (prevents reuse of last 5-10 passwords)
- Password strength validation with configurable policies
- Encrypted import/export functionality with integrity verification
- Password generator with customizable rules
- Passphrase generator for memorable passwords
- Search and filter capabilities
- Secure notes storage
- Account recovery with security questions
- Password recovery token system
- Automatic password history archiving (trigger-based)
- Configurable password history size per user

### Web Interface
- Modern, responsive UI (HTML5/CSS3/JavaScript)
- Auto-lock after inactivity
- Prevents multiple concurrent sessions
- Real-time password strength indicator
- Built-in password and passphrase generator
- Mobile-friendly design
- Single-page application (SPA)
- Secure cookie handling (HttpOnly, Secure, SameSite)

### Security Features
- SQL injection prevention (prepared statements)
- XSS protection (HTML sanitization)
- CSRF token validation
- Audit logging for all security events
- IP-based tracking and rate limiting
- Session timeout enforcement
- Secure random number generation
- Constant-time comparisons for secrets
- Recovery token system with expiration
- Security question lockout after failed attempts
- Comprehensive security headers (HSTS, X-Frame-Options, CSP)
- Database integrity verification with checksums

### User Security Policies
- Configurable minimum password length
- Require mixed case characters
- Require numbers and symbols
- Configurable password history size
- Adjustable max login attempts
- Custom session timeout
- Auto-lock timeout configuration
- 2FA requirement enforcement

### Database Optimization
- Write-Ahead Logging (WAL) mode for concurrent access
- Strategic indexes on frequently queried columns
- Automatic triggers for password history
- Foreign key constraints with CASCADE
- Full ACID compliance with synchronous=FULL

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
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  Password    │  │  Import/     │  │  Recovery    │  │
│  │  History     │  │  Export      │  │  Tokens      │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│         Encrypted SQLite Database (vault.db)            │
│              AES-256-GCM Encrypted Passwords            │
│              WAL Mode + Strategic Indexes               │
└─────────────────────────────────────────────────────────┘
```

### Components

| Component | Technology | Purpose |
|-----------|-----------|---------|
| API Server | C + OpenSSL | HTTPS/TLS server with RESTful API |
| Database | SQLite3 + WAL | Persistent storage with ACID guarantees |
| Crypto | libsodium | Encryption, hashing, TOTP, random generation |
| Session Manager | In-memory | Session tracking with CSRF tokens |
| Frontend | HTML5/CSS3/JS | Single-page web application |
| Password History | Database triggers | Automatic archiving and reuse prevention |
| Recovery System | Token-based | Secure account recovery with expiration |

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
│   ├── database_security.h    # Security policies interface
│   └── import_export.h        # Import/export interface
├── tests/
│   ├── test_crypto.c          # Crypto unit tests (12 tests)
│   ├── test_crypto_edge.c     # Crypto edge cases (8 tests)
│   ├── test_database.c        # Database unit tests (13 tests)
│   ├── test_session.c         # Session unit tests (9 tests)
│   ├── test_integration.c     # Integration tests (7 tests)
│   ├── test_security_questions.c # Security questions tests (6 tests)
│   ├── test_import_export.c   # Import/export tests (7 tests)
│   ├── test_concurrency.c     # Concurrent access tests (4 tests)
│   ├── test_memory_safety.c   # Memory leak tests (3 tests)
│   ├── test_edge_cases.c      # Edge case tests (4 tests)
│   └── test_recovery_scenarios.c # Recovery tests (3 tests)
├── web/
│   ├── index.html             # Web interface
│   ├── app.js                 # Frontend logic
│   └── style.css              # Styling
├── sql/
│   ├── schema.sql             # Database schema with triggers
│   └── migrate_security_questions.sql # Migration script
├── .github/
│   └── workflows/
│       └── ci.yml             # CI/CD pipeline
├── Makefile                   # Build configuration
├── README.md                  # This file
├── VULNERABILITY_POC.md       # Security vulnerability analysis
└── COLLABORATION.md           # Team workflow documentation
```

---

## Requirements

### System Requirements
- OS: Linux (Ubuntu 20.04+, Debian 11+)
- CPU: x86_64 or ARM64
- RAM: 512 MB minimum
- Disk: 100 MB for application + database

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

## Installation

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

### 3. Build the project 
```bash
make run
```
The server will start on `https://localhost:8443`

---

## Usage

### First-time Setup

1. Access the web interface: Open `https://localhost:8443` in your browser
2. Accept the self-signed certificate (for development only)
3. Register a new account with a strong master password (minimum 12 characters)
4. Set up 2FA (optional but recommended)
5. Configure security questions for account recovery

### Web Interface

The web interface provides:
- Dashboard: View all stored passwords
- Add Password: Store new credentials securely
- Edit/Delete: Manage existing entries
- Password Generator: Create strong, random passwords
- Passphrase Generator: Create memorable passphrases
- Password History: View password change history
- 2FA Setup: Enable/disable two-factor authentication
- Import/Export: Backup and restore your vault
- Security Logs: View authentication attempts and security events

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
  "expires_at": "2026-01-14T12:00:00Z"
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
curl -k -X GET https://localhost:8443/api/passwords \
  -H "Authorization: Bearer YOUR_SESSION_TOKEN"
```

#### Get password history
```bash
curl -k -X GET https://localhost:8443/api/passwords/1/history \
  -H "Authorization: Bearer YOUR_SESSION_TOKEN"
```

#### Generate a passphrase
```bash
curl -k -X GET https://localhost:8443/api/generate-passphrase \
  -H "Authorization: Bearer YOUR_SESSION_TOKEN"
```

---

## Testing

### Run All Tests
```bash
make test
```

### Test Categories

#### Unit Tests (42 tests)
- Crypto Tests (12): Encryption, decryption, hashing, TOTP
- Crypto Edge Cases (4): Boundary conditions, malformed input
- Database Tests (13): CRUD operations, transactions, constraints
- Session Tests (9): Session creation, validation, expiration
- Security Questions (5): Question setup, recovery, lockout

#### Integration Tests (6 tests)
- End-to-end user registration flow
- Complete login and password management
- Import/export functionality
- Recovery scenarios

#### Security Tests (20 tests)
- Import/Export (7): Encryption, integrity, corruption handling
- Concurrency (2): Thread safety, race conditions
- Memory Safety (4): Memory leaks, buffer overflows
- Edge Cases (4): Invalid input, boundary conditions
- Recovery Scenarios (2): Token validation, expiration, lockout

### Test Execution

```bash
# Run all tests
make test

# Run with memory checking
make test-valgrind

# Run with AddressSanitizer
make test-asan

# Static analysis
make analyze
```

### Test Output
```
[====] Synthesis: Tested: 66 | Passing: 66 | Failing: 0 | Crashing: 0
```

---

## Security

### Threat Model

Protected Against:
- SQL Injection (prepared statements)
- XSS attacks (HTML sanitization)
- CSRF (token validation)
- Session hijacking (cryptographically random tokens)
- Brute force (rate limiting + lockout)
- Timing attacks (constant-time comparisons)
- Memory disclosure (secure zeroing)
- Weak password storage (Argon2id)
- Man-in-the-middle (TLS 1.3 only)
- Password reuse (history tracking)

Known Limitations (See below):
- Single-user system by design
- Physical access attacks out of scope
- Requires secure master password from user

### Cryptographic Details

#### Password Encryption (AES-256-GCM)
```
- Algorithm: AES-256 in GCM mode
- Key derivation: Master password → Argon2id → 256-bit key
- Unique nonce per encryption
- Authenticated encryption (integrity + confidentiality)
```

#### Master Password Hashing (Argon2id)
```
- Algorithm: Argon2id (hybrid mode)
- Iterations: crypto_pwhash_OPSLIMIT_MODERATE (3)
- Memory: crypto_pwhash_MEMLIMIT_MODERATE (256 MB)
- Salt: 32 bytes (unique per user)
- Output: 128 bytes hash
```

#### TOTP 2FA
```
- Algorithm: TOTP (RFC 6238)
- Hash: SHA-1 (standard for TOTP)
- Time step: 30 seconds
- Code length: 6 digits
- Secret: 32 bytes base32-encoded
```

#### Session Tokens
```
- Length: 32 bytes (256 bits)
- Generation: libsodium's randombytes_buf
- Entropy: 2^256 possible values
- Format: 64 character hexadecimal string
```

#### Recovery Tokens
```
- Length: 32 bytes raw, hashed with crypto_generichash
- Expiration: 1 hour from generation
- One-time use: Invalidated after successful reset
- Stored: SHA-256 hash of token
```

### Security Audit Log

All security-relevant events are logged:
```
- Failed login attempts (with IP address)
- Successful authentications
- Rate limiting triggers
- CSRF validation failures
- Session creation/destruction
- Password changes
- Recovery attempts
- 2FA setup/disable
```

Log format:
```
[2026-01-14 14:32:15] SECURITY: Failed login attempt from 192.168.1.100 (attempt 3/5)
[2026-01-14 14:32:45] SECURITY: IP 192.168.1.100 locked out for 15 minutes
[2026-01-14 14:33:01] SECURITY: CSRF validation failed for session abc123...
[2026-01-14 14:35:00] SECURITY: Password changed for user ID: 1
```

---

## API Documentation

### Base URL
```
https://localhost:8443
```

### Authentication

Most endpoints require authentication via session token (obtained from login).

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN  (for state-changing operations)
```

---

### User Management Endpoints

#### POST `/api/register`
Register a new user account with security questions.

Request:
```json
{
  "master_password": "YourStrongPassword123!",
  "question1": "What is your favorite color?",
  "answer1": "Blue",
  "question2": "Where were you born?",
  "answer2": "Paris",
  "question3": "What is your pet name?",
  "answer3": "Max"
}
```

Response (201 Created):
```json
{
  "success": true,
  "session_token": "abc123...",
  "csrf_token": "xyz789..."
}
```

#### POST `/api/login`
Authenticate with master password and optional TOTP code.

Request:
```json
{
  "master_password": "YourStrongPassword123!",
  "totp_code": "123456"
}
```

Response (200 OK):
```json
{
  "session_token": "abc123...",
  "csrf_token": "xyz789...",
  "expires_at": "2026-01-14T12:00:00Z"
}
```

#### POST `/api/logout`
End the current session.

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
```

Response (200 OK):
```json
{
  "success": true
}
```

---

### Password Management Endpoints

#### GET `/api/passwords`
List all stored passwords (decrypted).

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
```

Response (200 OK):
```json
[
  {
    "id": 1,
    "website": "example.com",
    "username": "user@example.com",
    "password": "MyPassword123!",
    "notes": "Personal account",
    "created_at": "2026-01-14 10:00:00",
    "updated_at": "2026-01-14 10:00:00"
  }
]
```

#### POST `/api/passwords`
Add a new password entry.

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
Content-Type: application/json
```

Request:
```json
{
  "website": "example.com",
  "username": "user@example.com",
  "password": "MyPassword123!",
  "notes": "Personal account"
}
```

Response (201 Created):
```json
{
  "success": true,
  "id": 1
}
```

#### GET `/api/passwords/:id/decrypt`
Get a specific password (decrypted).

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
```

Response (200 OK):
```json
{
  "id": 1,
  "website": "example.com",
  "username": "user@example.com",
  "password": "MyPassword123!",
  "notes": "Personal account"
}
```

#### PUT `/api/passwords/:id/update`
Update an existing password.

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
Content-Type: application/json
```

Request:
```json
{
  "password": "NewPassword456!"
}
```

Response (200 OK):
```json
{
  "success": true
}
```

Response (409 Conflict) - If password was recently used:
```json
{
  "error": "Ce mot de passe a déjà été utilisé récemment"
}
```

#### DELETE `/api/passwords/:id`
Delete a password entry.

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
```

Response (200 OK):
```json
{
  "success": true
}
```

#### GET `/api/passwords/:id/history`
Get password change history (last 10 changes).

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
```

Response (200 OK):
```json
[
  {
    "id": 5,
    "password_id": 1,
    "website": "example.com",
    "username": "user@example.com",
    "changed_at": "2026-01-14 10:00:00"
  },
  {
    "id": 4,
    "password_id": 1,
    "website": "example.com",
    "username": "user@example.com",
    "changed_at": "2026-01-13 15:30:00"
  }
]
```

---

### Security Endpoints

#### GET `/api/security-questions`
Get security questions for the current user.

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
```

Response (200 OK):
```json
{
  "question1": "What is your favorite color?",
  "question2": "Where were you born?",
  "question3": "What is your pet name?"
}
```

#### POST `/api/recover-account`
Verify security answers and get recovery code.

Request:
```json
{
  "answer1": "Blue",
  "answer2": "Paris",
  "answer3": "Max"
}
```

Response (200 OK):
```json
{
  "success": true,
  "recovery_code": "XXXX-XXXX-XXXX-XXXX"
}
```

Response (423 Locked) - After too many failed attempts:
```json
{
  "error": "Compte verrouillé. Réessayez plus tard."
}
```

#### POST `/api/request-password-reset`
Request a password reset token (sent via recovery code).

Request:
```json
{
  "username": "1"
}
```

Response (200 OK):
```json
{
  "success": true,
  "recovery_token": "abc123def456...",
  "message": "Token de récupération généré. Utilisez-le dans l'heure qui suit."
}
```

#### POST `/api/reset-password`
Reset password using recovery token.

Request:
```json
{
  "recovery_token": "abc123def456...",
  "password": "NewStrongPassword123!"
}
```

Response (200 OK):
```json
{
  "success": true,
  "message": "Mot de passe réinitialisé avec succès"
}
```

#### GET `/api/security-logs`
Get security audit logs.

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
```

Response (200 OK):
```json
[
  {
    "id": 15,
    "action": "LOGIN_SUCCESS",
    "success": 1,
    "ip_address": "192.168.1.100",
    "timestamp": "2026-01-14 14:30:00"
  },
  {
    "id": 14,
    "action": "LOGIN_FAILED",
    "success": 0,
    "ip_address": "192.168.1.101",
    "timestamp": "2026-01-14 14:25:00"
  }
]
```

---

### Utility Endpoints

#### GET `/api/status`
Check API health status.

Response (200 OK):
```json
{
  "status": "ok",
  "version": "2.0"
}
```

#### GET `/api/generate-passphrase`
Generate a memorable passphrase.

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
```

Response (200 OK):
```json
{
  "passphrase": "correct-horse-battery-staple"
}
```

---

### Security Setup Endpoints

#### POST `/api/setup-security-questions`
Configure security questions after registration (alternative to setup during registration).

Headers:
```
Authorization: Bearer YOUR_SESSION_TOKEN
X-CSRF-Token: YOUR_CSRF_TOKEN
```

Request:
```json
{
  "question1": "What is your favorite color?",
  "answer1": "Blue",
  "question2": "Where were you born?",
  "answer2": "Paris",
  "question3": "What is your pet name?",
  "answer3": "Max"
}
```

---

## Development

### Build Targets

```bash
make              # Build project
make test         # Run all 66 tests
make clean        # Clean build files (preserve database)
make reset        # Reset database and certificates
make ssl          # Generate SSL certificates
make run          # Run server (HTTPS on 8443)
make kill         # Stop running server
make status       # Check server status
make valgrind     # Run tests with memory checks
make analyze      # Static analysis (cppcheck)
make install      # Install dependencies
```

### Adding a New Feature

1. Write tests first (TDD approach)
2. Implement feature in appropriate module
3. Run tests: `make test`
4. Check memory: `make valgrind`
5. Static analysis: `make analyze`
6. Update documentation
7. Commit with descriptive message

### Code Style

- Use consistent indentation (4 spaces)
- Function names: `snake_case`
- Constants: `UPPER_CASE`
- Structs: `PascalCase`
- Comments for complex logic only
- Avoid global variables
- Check all return values
- Free all allocated memory
- Use secure_zero_memory for sensitive data

### Database Schema

The database uses strategic indexes and triggers for performance and automation:

Indexes:
- `idx_attempts_timestamp`: Optimizes security log queries
- `idx_attempts_ip`: Enables fast IP-based rate limiting
- `idx_passwords_website`: Accelerates password searches
- `idx_sessions_token`: Fast session lookup
- `idx_password_history_composite`: Efficient history retrieval

Triggers:
- `backup_password_history`: Automatically archives old passwords before updates
- `update_passwords_timestamp`: Auto-updates timestamps on modifications
- `update_users_timestamp`: Maintains user record timestamps

### Debugging

```bash
# Compile with debug symbols
make CFLAGS="-g -O0 -Wall"

# Run with GDB
gdb ./password_vault

# Run with AddressSanitizer
make CFLAGS="-fsanitize=address -g"
./password_vault

# Run with Valgrind
valgrind --leak-check=full ./password_vault
```

---

## Documentation

### Project Documentation

- README.md - Project overview, installation, and usage guide
- VULNERABILITY_POC.md - Security vulnerability analysis and proof-of-concept
- COLLABORATION.md - Team workflow, sprint organization, and contribution guidelines
- API Documentation - Complete API reference (see above)

### Security Documentation

The project includes comprehensive security documentation:

1. Vulnerability Analysis (VULNERABILITY_POC.md)
   - 12 common vulnerabilities analyzed
   - Proof-of-concept code examples
   - Secure implementation details
   - OWASP Top 10 coverage

2. Security Features (this README)
   - Cryptographic algorithms used
   - Protection mechanisms
   - Security best practices

3. Audit Trail
   - All security-relevant events logged
   - Failed authentication attempts tracked
   - Rate limiting triggers recorded
   - Password changes and recovery attempts

### Team Documentation

- Team Workflow (COLLABORATION.md)
  - Sprint organization and planning
  - Task distribution and time tracking
  - Communication protocols
  - Individual contributions

---

## Known Limitations

### By Design
- Single-user system: No multi-user support (by design for personal password manager)
- Local deployment: Not hardened for public internet exposure
- Master password: Single point of failure (standard for password managers)
- Self-signed certificates: OK for development, use real certs in production

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

This project was developed by students at EPITA as part of the Secure Development course.

| Name | Email | Role |
|------|-------|------|
| Deborah Basson | deborah.basson@epita.fr | Lead Developer |
| Louise Grosdhomme | louise.grosdhomme@epita.fr | Security Engineer |
| Alida Nana-Sunji-Kouanang | alida.nana-sunji-kouanang@epita.fr | Backend Developer |
| Maimouna Tall | maimouna.tall@epita.fr | Frontend Developer |

For detailed contribution information, see COLLABORATION.md.

---

## Support

### For Security Issues
See VULNERABILITY_POC.md for security analysis and reporting guidelines.

### For General Questions
- Open an issue on GitHub
- Contact the team via email

---

## License

This project is developed for educational purposes as part of the EPITA curriculum.

Copyright 2025 EPITA - All rights reserved

---

Last Updated: 2026-01-14  
Version: 2.0.0
