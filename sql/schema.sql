-- ============================================
-- SCHEMA BASE DE DONNÉES - PASSWORD VAULT
-- ============================================

-- Table des utilisateurs
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    master_hash TEXT NOT NULL,
    salt TEXT NOT NULL,
    totp_secret TEXT,
    is_2fa_enabled INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Table des mots de passe
CREATE TABLE IF NOT EXISTS passwords (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER DEFAULT 1,
    website TEXT NOT NULL,
    username TEXT NOT NULL,
    encrypted_password TEXT NOT NULL,
    notes TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Table des tentatives de connexion (audit & rate limiting)
CREATE TABLE IF NOT EXISTS login_attempts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    action TEXT NOT NULL,
    success INTEGER NOT NULL,
    ip_address TEXT,
    user_agent TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Table des sessions actives
CREATE TABLE IF NOT EXISTS sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    token TEXT UNIQUE NOT NULL,
    user_id INTEGER NOT NULL,
    ip_address TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_activity DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Table d'historique des mots de passe (améliorée)
CREATE TABLE IF NOT EXISTS password_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    password_id INTEGER NOT NULL,
    encrypted_password TEXT NOT NULL,
    website TEXT NOT NULL,
    username TEXT NOT NULL,
    changed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (password_id) REFERENCES passwords(id) ON DELETE CASCADE
);

-- Table des politiques de sécurité utilisateur
CREATE TABLE IF NOT EXISTS user_security_policies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER DEFAULT 1,
    require_2fa INTEGER DEFAULT 1,
    password_min_length INTEGER DEFAULT 12,
    require_mixed_case INTEGER DEFAULT 1,
    require_numbers INTEGER DEFAULT 1,
    require_symbols INTEGER DEFAULT 1,
    password_history_size INTEGER DEFAULT 5,
    max_login_attempts INTEGER DEFAULT 3,
    session_timeout INTEGER DEFAULT 900,
    auto_lock_timeout INTEGER DEFAULT 300,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Table des questions de sécurité
CREATE TABLE IF NOT EXISTS security_questions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER DEFAULT 1,
    question1 TEXT NOT NULL,
    answer1_hash TEXT NOT NULL,
    answer1_salt TEXT NOT NULL,
    question2 TEXT NOT NULL,
    answer2_hash TEXT NOT NULL,
    answer2_salt TEXT NOT NULL,
    question3 TEXT NOT NULL,
    answer3_hash TEXT NOT NULL,
    answer3_salt TEXT NOT NULL,
    recovery_code TEXT NOT NULL,
    recovery_attempts INTEGER DEFAULT 0,
    locked_until DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- ============================================
-- INDEX POUR PERFORMANCES
-- ============================================

CREATE INDEX IF NOT EXISTS idx_attempts_timestamp 
    ON login_attempts(timestamp DESC);

CREATE INDEX IF NOT EXISTS idx_attempts_ip 
    ON login_attempts(ip_address, timestamp DESC);

CREATE INDEX IF NOT EXISTS idx_attempts_success 
    ON login_attempts(success, timestamp DESC);

CREATE INDEX IF NOT EXISTS idx_passwords_website 
    ON passwords(website);

CREATE INDEX IF NOT EXISTS idx_passwords_user 
    ON passwords(user_id);

CREATE INDEX IF NOT EXISTS idx_sessions_token 
    ON sessions(token);

CREATE INDEX IF NOT EXISTS idx_sessions_expiry 
    ON sessions(expires_at);

CREATE INDEX IF NOT EXISTS idx_password_history_composite 
    ON password_history(password_id, changed_at DESC);

CREATE INDEX IF NOT EXISTS idx_user_policies 
    ON user_security_policies(user_id);

-- ============================================
-- TRIGGERS
-- ============================================

-- Trigger pour sauvegarder l'historique avant modification
CREATE TRIGGER IF NOT EXISTS backup_password_history 
    BEFORE UPDATE OF encrypted_password ON passwords
    FOR EACH ROW
BEGIN
    INSERT INTO password_history (password_id, encrypted_password, website, username)
    VALUES (OLD.id, OLD.encrypted_password, OLD.website, OLD.username);
END;

-- Trigger pour mettre à jour updated_at sur passwords
CREATE TRIGGER IF NOT EXISTS update_passwords_timestamp 
    AFTER UPDATE OF encrypted_password ON passwords
    FOR EACH ROW
    WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE passwords SET updated_at = CURRENT_TIMESTAMP WHERE id = NEW.id;
END;

-- Trigger pour mettre à jour updated_at sur users
CREATE TRIGGER IF NOT EXISTS update_users_timestamp 
    AFTER UPDATE ON users
    FOR EACH ROW
    WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE users SET updated_at = CURRENT_TIMESTAMP WHERE id = NEW.id;
END;

CREATE TABLE IF NOT EXISTS recovery_tokens (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    token_hash TEXT NOT NULL UNIQUE,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    expires_at INTEGER NOT NULL,
    used INTEGER DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_recovery_tokens_hash 
    ON recovery_tokens(token_hash);

CREATE INDEX IF NOT EXISTS idx_recovery_tokens_expiry 
    ON recovery_tokens(expires_at);

CREATE INDEX IF NOT EXISTS idx_recovery_tokens_user 
    ON recovery_tokens(user_id);