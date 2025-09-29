-- ============================================
-- SCHEMA BASE DE DONNÉES - PASSWORD VAULT
-- Version: 1.0
-- Description: Structure sécurisée pour gestionnaire de mots de passe
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

-- ============================================
-- TRIGGERS POUR MISE À JOUR AUTOMATIQUE
-- ============================================

-- Trigger pour mettre à jour updated_at sur users
CREATE TRIGGER IF NOT EXISTS update_users_timestamp 
    AFTER UPDATE ON users
    FOR EACH ROW
BEGIN
    UPDATE users SET updated_at = CURRENT_TIMESTAMP WHERE id = NEW.id;
END;

-- Trigger pour mettre à jour updated_at sur passwords
CREATE TRIGGER IF NOT EXISTS update_passwords_timestamp 
    AFTER UPDATE ON passwords
    FOR EACH ROW
BEGIN
    UPDATE passwords SET updated_at = CURRENT_TIMESTAMP WHERE id = NEW.id;
END;

-- ============================================
-- VUES POUR STATISTIQUES (optionnel)
-- ============================================

CREATE VIEW IF NOT EXISTS v_failed_login_stats AS
SELECT 
    ip_address,
    COUNT(*) as attempt_count,
    MAX(timestamp) as last_attempt,
    datetime('now', '-15 minutes') as cutoff_time
FROM login_attempts
WHERE success = 0 
  AND timestamp > datetime('now', '-15 minutes')
GROUP BY ip_address
HAVING COUNT(*) >= 3;

CREATE VIEW IF NOT EXISTS v_password_summary AS
SELECT 
    COUNT(*) as total_passwords,
    COUNT(DISTINCT website) as unique_websites,
    MAX(created_at) as last_added,
    MIN(created_at) as first_added
FROM passwords;