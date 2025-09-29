# [DEVSEC] Password Vault

## Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Installation](#installation)
4. [Usage](#usage)
5. [Contributors](#contributors)

---

## Update
- Double authentification pas encore faite
- Quand tu arrive sur l'appli les boutton ne fonctionne pas encore bien 1 fois sur 2 
- BDD en clair encore - Implémenter AES-256-GCM avec clé dérivée du master password

- HTTPS obligatoire
Générer de vrais certificats avec Let's Encrypt
Configurer nginx/Caddy en reverse proxy
Rediriger HTTP → HTTPS

- CSRF Protection
Générer des tokens CSRF
Valider les tokens sur toutes les routes POST/PUT/DELETE

- Headers de sécurité supplémentaires
Ajouter X-Content-Type-Options: nosniff
Ajouter X-Frame-Options: DENY
Renforcer Content-Security-Policy


## Overview
This **password vault** is a project developed as part of the **Secure Development** course at EPITA.  
The project is written in **C** and allows users to securely store and manage passwords.

---

## Features
- Secure storage of passwords
- Add, edit, and delete account entries
- Simple and intuitive command-line interface
- Password encryption for maximum protection
- **[TODO]**

---

## Installation
1. Clone the repository :

```bash
git clone https://github.com/deborah-basson/password-vault.git
```

2. Compile and run the project :
```bash
cd password-manager
make run

```
---

## Usage

**[TODO]**

---

## Contributors

| Name       | Email                           |
|------------|---------------------------------|
| Deborah Basson | [deborah.basson@epita.fr](mailto:deborah@epita.fr) |
| Louise Grosdhomme | [louise.grosdhomme@epita.fr](mailto:louise@epita.fr) |
| Alida Nana-Sunji-Kouanang | [alida.nana-sunji-kouanang@epita.fr](mailto:alida@epita.fr) |
| Maimouna Tall | [maimouna.tall@epita.fr](mailto:maimouna@epita.fr) |

