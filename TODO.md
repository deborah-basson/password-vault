# TODO - Password Vault Project

This document lists the main tasks and steps to be completed for the Password Vault project. It is meant as a roadmap for the development process.

---

## 1. Project Setup
- [ ] Initialize Git repository
- [ ] Create main and develop branches
- [ ] Set up .gitignore for binaries, object files, and sensitive data
- [ ] Set up initial project structure:
  - `src/` for source files
  - `include/` for header files
  - `tests/` for unit tests
  - `docs/` for documentation

---

## 2. Core Functionality
### Account Management
- [ ] Add new account (username + password)
- [ ] Edit existing account
- [ ] Delete account
- [ ] List all stored accounts

### Password Security
- [ ] Implement password encryption
- [ ] Implement secure password storage
- [ ] Implement password retrieval with decryption

### File Management
- [ ] Create storage file if it doesn't exist
- [ ] Load existing passwords on startup
- [ ] Save passwords to file after any change

---

## 3. User Interface (CLI)
- [ ] Display menu options (add, edit, delete, list, exit)
- [ ] Handle user input safely
- [ ] Show clear messages for errors or success

---

## 4. Testing
- [ ] Unit tests for account management
- [ ] Unit tests for encryption/decryption
- [ ] Unit tests for file operations
- [ ] Test CLI flows end-to-end

---

## 5. Documentation
- [ ] Complete README.md with:
  - Project overview
  - Installation instructions
  - Usage examples
  - ...
- [ ] Write inline code comments
- [ ] Write TODO.md (this document)

---

## 6. Optional / Advanced Features
- [ ] Implement master password to unlock vault
- [ ] Add password strength checking
- [ ] Add automatic backup of the vault file
- [ ] Improve CLI with colored output or better formatting


