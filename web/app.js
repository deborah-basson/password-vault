const API_URL = window.location.origin + '/api';
let currentSession = null;
let autoLockTimer = null;
let clipboardTimeout = null;
let lastActivity = Date.now();
let isLocked = false;

// ==================== INITIALISATION HTTPS ====================

window.addEventListener('DOMContentLoaded', async () => {
    try {
        const response = await fetch(`${API_URL}/status`);
        const data = await response.json();
        
        if (data.has_account) {
            const token = localStorage.getItem('session_token');
            if (token) {
                currentSession = { token };
                showDashboard();
            } else {
                showScreen('login-screen');
            }
        } else {
            showScreen('register-screen');
        }
    } catch (err) {
        console.error('Erreur serveur HTTPS:', err);
        
        // En cas d'erreur HTTPS, afficher un message spécifique
        if (err.message && err.message.includes('SSL')) {
            notify('Erreur de connexion sécurisée. Vérifiez le certificat SSL.', 'error');
        }
        showScreen('register-screen');
    }
    
    updateStrengthBar('register-password', 'strength-bar', 'strength-text');
    updateStrengthBar('add-password', 'add-strength-bar', null);
    resetActivityTimer();
});

// ==================== VÉRIFICATION MULTI-ONGLETS ====================

const TAB_ID = Math.random().toString(36).substr(2, 9);
localStorage.setItem('current_tab', TAB_ID);

function checkOtherTabs() {
    const currentTab = localStorage.getItem('current_tab');
    if (currentTab !== TAB_ID && currentSession) {
        notify('🔒 Session utilisée dans un autre onglet', 'warning');
        logout();
    }
}

setInterval(checkOtherTabs, 2000);

// ==================== VERROUILLAGE RÉEL ====================

function resetActivityTimer() {
    if (isLocked) return;
    
    lastActivity = Date.now();
    if (autoLockTimer) clearTimeout(autoLockTimer);
    
    autoLockTimer = setTimeout(() => {
        if (currentSession && !isLocked) {
            realLockApp();
        }
    }, 180000); // 3 minutes
}

function realLockApp() {
    if (!currentSession || isLocked) return;
    
    console.log('🔒 Verrouillage réel de l application');
    
    // Nettoyer le clipboard
    if (clipboardTimeout) {
        clearTimeout(clipboardTimeout);
        navigator.clipboard.writeText('').catch(() => {});
        clipboardTimeout = null;
    }
    
    // Appeler l'API lock
    const token = localStorage.getItem('session_token');
    if (token) {
        fetch(`${API_URL}/lock`, {
            method: 'POST',
            headers: { 
                'Authorization': `Bearer ${token}`,
                'Content-Type': 'application/json'
            }
        }).catch(() => {});
    }
    
    isLocked = true;
    showScreen('lock-screen');
    document.getElementById('unlock-password').value = '';
    document.getElementById('unlock-password').focus();
    
    localStorage.removeItem('session_token');
    localStorage.removeItem('csrf_token');
    
    notify('🔒 Application verrouillée - Entrez votre mot de passe', 'warning');
}

function lockAppManual() {
    realLockApp();
}

// ==================== DÉVERROUILLAGE ====================

function showLockScreen() {
    showScreen('lock-screen');
}

document.getElementById('unlock-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    
    const password = document.getElementById('unlock-password').value;
    
    if (!password) {
        notify('Veuillez entrer votre mot de passe', 'error');
        return;
    }
    
    try {
        const response = await fetch(`${API_URL}/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ password })
        });
        
        const data = await response.json();
        
        if (response.ok) {
            currentSession = {
                token: data.session_token,
                csrf_token: data.csrf_token,
                expires_in: data.expires_in
            };
            localStorage.setItem('session_token', data.session_token);
            localStorage.setItem('csrf_token', data.csrf_token);
            
            isLocked = false;
            resetActivityTimer();
            document.getElementById('unlock-password').value = '';
            
            notify('🔓 Application déverrouillée', 'success');
            showDashboard();
        } else {
            document.getElementById('unlock-password').value = '';
            notify(data.error || 'Mot de passe incorrect', 'error');
        }
    } catch (err) {
        document.getElementById('unlock-password').value = '';
        notify('Erreur réseau', 'error');
        console.error(err);
    }
});

// ==================== CHARGEMENT DES MOTS DE PASSE ====================

async function loadPasswords() {
    const token = localStorage.getItem('session_token');
    if (!token) {
        showLogin();
        return;
    }
    
    try {
        const response = await fetch(`${API_URL}/passwords`, {
            headers: { 'Authorization': `Bearer ${token}` }
        });
        
        if (response.status === 401) {
            notify('Session expirée', 'error');
            logout();
            return;
        }
        
        const data = await response.json();
        const container = document.getElementById('passwords-list');
        
        if (!data.passwords || data.passwords.length === 0) {
            container.innerHTML = '<div class="loading">Aucun mot de passe enregistré</div>';
            return;
        }
        
        container.innerHTML = data.passwords.map(pwd => `
            <div class="password-card" data-id="${pwd.id}">
                <h3>${escapeHtml(pwd.website)}</h3>
                <div class="username">${escapeHtml(pwd.username)}</div>
                <div class="password-actions">
                    <button class="btn btn-secondary" onclick="copyPassword(${pwd.id})">📋 Copier</button>
                    <button class="btn btn-danger" onclick="deletePassword(${pwd.id})">🗑️ Supprimer</button>
                </div>
            </div>
        `).join('');
        
        console.log(`✅ ${data.passwords.length} mot(s) de passe chargé(s)`);
    } catch (err) {
        console.error('Erreur chargement mots de passe:', err);
        notify('Erreur chargement', 'error');
    }
}

function showDashboard() {
    if (isLocked) {
        showLockScreen();
        return;
    }
    
    showScreen('dashboard-screen');
    loadPasswords();
    resetActivityTimer();
    
    // Vérifier et ajouter le bouton de verrouillage
    let lockBtn = document.getElementById('btn-lock');
    if (!lockBtn) {
        const actionsDiv = document.querySelector('.actions');
        if (actionsDiv) {
            lockBtn = document.createElement('button');
            lockBtn.id = 'btn-lock';
            lockBtn.className = 'btn btn-warning';
            lockBtn.innerHTML = '🔒 Verrouiller';
            lockBtn.onclick = lockAppManual;
            actionsDiv.appendChild(lockBtn);
        }
    }
    
    const searchInput = document.getElementById('search-input');
    if (searchInput) searchInput.value = '';
    
    console.log('✅ Tableau de bord affiché');
}

// ==================== GESTION DES ÉCRANS ====================

function showScreen(screenId) {
    document.querySelectorAll('.screen').forEach(s => s.classList.add('hidden'));
    const screen = document.getElementById(screenId);
    if (screen) screen.classList.remove('hidden');
}

function showLogin() {
    showScreen('login-screen');
}

function showDashboard() {
    if (isLocked) {
        showLockScreen();
        return;
    }
    
    showScreen('dashboard-screen');
    loadPasswords();
    resetActivityTimer();
    
    const existingLockBtn = document.getElementById('btn-lock');
    if (existingLockBtn) {
        existingLockBtn.remove();
    }
    
    const actionsDiv = document.querySelector('.actions');
    const lockBtn = document.createElement('button');
    lockBtn.id = 'btn-lock';
    lockBtn.className = 'btn btn-warning';
    lockBtn.innerHTML = '🔒 Verrouiller';
    lockBtn.onclick = lockAppManual;
    actionsDiv.appendChild(lockBtn);
    
    const searchInput = document.getElementById('search-input');
    if (searchInput) searchInput.value = '';
}

// ==================== COPIE SÉCURISÉE ====================

async function copyPassword(id) {
    if (isLocked) {
        notify('Application verrouillée', 'error');
        showLockScreen();
        return;
    }
    
    const token = localStorage.getItem('session_token');
    const csrfToken = localStorage.getItem('csrf_token');
    
    if (!token) {
        notify('Session expirée', 'error');
        logout();
        return;
    }
    
    try {
        const response = await fetch(`${API_URL}/passwords/${id}/decrypt`, {
            method: 'GET',
            headers: { 
                'Authorization': `Bearer ${token}`,
                'X-CSRF-Token': csrfToken
            }
        });
        
        if (response.status === 401) {
            notify('Session expirée', 'error');
            logout();
            return;
        }
        
        const data = await response.json();
        
        if (response.ok && data.password) {
            await navigator.clipboard.writeText(data.password);
            notify('✓ Mot de passe copié avec succès', 'success');
            
        } else {
            notify(data.error || 'Erreur déchiffrement', 'error');
        }
    } catch (err) {
        notify('Erreur réseau', 'error');
        console.error(err);
    }
}

// ==================== NOTIFICATIONS ====================

function notify(message, type = 'success') {
    const notif = document.getElementById('notification');
    notif.textContent = message;
    notif.className = `notification ${type}`;
    notif.classList.remove('hidden');
    
    setTimeout(() => {
        notif.classList.add('hidden');
    }, 4000);
}

// ==================== VÉRIFICATION MOT DE PASSE ====================

function checkPasswordStrength(password) {
    let strength = 0;
    const checks = {
        length: password.length >= 12,
        upper: /[A-Z]/.test(password),
        lower: /[a-z]/.test(password),
        number: /[0-9]/.test(password),
        special: /[^A-Za-z0-9]/.test(password)
    };
    
    strength = Object.values(checks).filter(Boolean).length;
    
    let color, width, text;
    if (strength < 3) {
        color = '#ef4444';
        width = '33%';
        text = 'Faible';
    } else if (strength < 4) {
        color = '#f59e0b';
        width = '66%';
        text = 'Moyen';
    } else {
        color = '#10b981';
        width = '100%';
        text = 'Fort';
    }
    
    return { strength, color, width, text, valid: strength >= 3 };
}

function updateStrengthBar(inputId, barId, textId) {
    const input = document.getElementById(inputId);
    const bar = document.getElementById(barId);
    const text = textId ? document.getElementById(textId) : null;
    
    if (!input || !bar) return;
    
    input.addEventListener('input', () => {
        const result = checkPasswordStrength(input.value);
        bar.style.setProperty('--strength', result.width);
        bar.style.setProperty('--strength-color', result.color);
        if (text) text.textContent = result.text;
    });
}

// ==================== INSCRIPTION ====================

document.getElementById('register-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    
    const password = document.getElementById('register-password').value;
    const confirm = document.getElementById('register-confirm').value;
    
    if (password !== confirm) {
        notify('Les mots de passe ne correspondent pas', 'error');
        return;
    }
    
    const strength = checkPasswordStrength(password);
    if (!strength.valid) {
        notify('Mot de passe trop faible (min 12 caractères)', 'error');
        return;
    }
    
    try {
        const response = await fetch(`${API_URL}/register`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ password })
        });
        
        const data = await response.json();
        
        if (response.ok) {
            notify('Compte créé avec succès !', 'success');
            document.getElementById('register-password').value = '';
            document.getElementById('register-confirm').value = '';
            showLogin();
        } else {
            document.getElementById('register-password').value = '';
            document.getElementById('register-confirm').value = '';
            notify(data.error || 'Erreur lors de l\'inscription', 'error');
        }
    } catch (err) {
        document.getElementById('register-password').value = '';
        document.getElementById('register-confirm').value = '';
        notify('Erreur réseau', 'error');
        console.error(err);
    }
});

// ==================== CONNEXION ====================

document.getElementById('login-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    
    const password = document.getElementById('login-password').value;
    
    if (!password) {
        notify('Veuillez entrer votre mot de passe', 'error');
        return;
    }
    
    try {
        const response = await fetch(`${API_URL}/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ password })
        });
        
        const data = await response.json();
        
        if (response.ok) {
            currentSession = {
                token: data.session_token,
                csrf_token: data.csrf_token,
                expires_in: data.expires_in
            };
            localStorage.setItem('session_token', data.session_token);
            localStorage.setItem('csrf_token', data.csrf_token);
            
            resetActivityTimer();
            document.getElementById('login-password').value = '';
            
            notify('Connexion réussie! Verrouillage auto: 3min', 'success');
            showDashboard();
        } else {
            document.getElementById('login-password').value = '';
            notify(data.error || 'Mot de passe incorrect', 'error');
        }
    } catch (err) {
        document.getElementById('login-password').value = '';
        notify('Erreur réseau', 'error');
        console.error(err);
    }
});

// ==================== RECHERCHE ====================

function setupSearch() {
    const searchInput = document.getElementById('search-input');
    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            const query = e.target.value.toLowerCase();
            document.querySelectorAll('.password-card').forEach(card => {
                const website = card.querySelector('h3').textContent.toLowerCase();
                const username = card.querySelector('.username').textContent.toLowerCase();
                const shouldShow = website.includes(query) || username.includes(query);
                card.style.display = shouldShow ? 'block' : 'none';
            });
        });
    }
}

// Appeler cette fonction dans showDashboard
function showDashboard() {
    if (isLocked) {
        showLockScreen();
        return;
    }
    
    showScreen('dashboard-screen');
    loadPasswords();
    resetActivityTimer();
    setupSearch(); // ← Ajouter cet appel
    
    // ... reste du code ...
}

// ==================== AJOUT MOT DE PASSE ====================

function showAddPassword() {
    document.getElementById('add-modal').classList.remove('hidden');
}

function closeModal() {
    document.getElementById('add-modal').classList.add('hidden');
    document.getElementById('add-password-form').reset();
}

function closeGenerateModal() {
    document.getElementById('generate-modal').classList.add('hidden');
}

document.getElementById('add-password-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    
    const website = document.getElementById('add-website').value;
    const username = document.getElementById('add-username').value;
    const password = document.getElementById('add-password').value;
    const notes = document.getElementById('add-notes').value;
    const token = localStorage.getItem('session_token');
    const csrfToken = localStorage.getItem('csrf_token');
    
    const strength = checkPasswordStrength(password);
    if (!strength.valid) {
        notify('Mot de passe trop faible', 'error');
        return;
    }
    
    try {
        const response = await fetch(`${API_URL}/passwords`, {
            method: 'POST',
            headers: { 
                'Content-Type': 'application/json',
                'Authorization': `Bearer ${token}`,
                'X-CSRF-Token': csrfToken
            },
            body: JSON.stringify({ website, username, password, notes })
        });
        
        if (response.status === 401) {
            notify('Session expirée', 'error');
            logout();
            return;
        }
        
        if (response.status === 403) {
            notify('Erreur CSRF - Reconnectez-vous', 'error');
            logout();
            return;
        }
        
        const data = await response.json();
        
        if (response.ok) {
            notify('Mot de passe ajouté et chiffré ! 🔒', 'success');
            closeModal();
            loadPasswords();
        } else {
            notify(data.error || 'Erreur', 'error');
        }
    } catch (err) {
        notify('Erreur réseau', 'error');
        console.error(err);
    }
});

// ==================== SUPPRESSION MOT DE PASSE ====================

async function deletePassword(id) {
    if (!confirm('Supprimer définitivement ce mot de passe ?')) return;
    
    const token = localStorage.getItem('session_token');
    const csrfToken = localStorage.getItem('csrf_token');
    
    try {
        const response = await fetch(`${API_URL}/passwords/${id}`, {
            method: 'DELETE',
            headers: { 
                'Authorization': `Bearer ${token}`,
                'X-CSRF-Token': csrfToken
            }
        });
        
        if (response.status === 403) {
            notify('Erreur CSRF - Reconnectez-vous', 'error');
            logout();
            return;
        }
        
        if (response.ok) {
            notify('🗑️ Mot de passe supprimé', 'success');
            loadPasswords();
        } else {
            notify('Erreur suppression', 'error');
        }
    } catch (err) {
        notify('Erreur réseau', 'error');
        console.error(err);
    }
}

// ==================== GÉNÉRATEUR MOT DE PASSE ====================

function generatePasswordPopup() {
    document.getElementById('generate-modal').classList.remove('hidden');
    generateNewPassword();
}

function generateNewPassword() {
    const length = document.getElementById('password-length').value;
    const password = generateSecurePassword(parseInt(length));
    document.getElementById('generated-password-display').textContent = password;
    window.lastGeneratedPassword = password;
}

function generateSecurePassword(length) {
    const lowercase = 'abcdefghijklmnopqrstuvwxyz';
    const uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
    const digits = '0123456789';
    const symbols = '!@#$%^&*()_+-=[]{}|;:,.<>?';
    const all = lowercase + uppercase + digits + symbols;
    
    const array = new Uint8Array(length);
    crypto.getRandomValues(array);
    
    let password = '';
    password += lowercase[array[0] % lowercase.length];
    password += uppercase[array[1] % uppercase.length];
    password += digits[array[2] % digits.length];
    password += symbols[array[3] % symbols.length];
    
    for (let i = 4; i < length; i++) {
        password += all[array[i] % all.length];
    }
    
    password = password.split('').sort(() => array[length - 1] - 128).join('');
    
    return password;
}

function copyGenerated() {
    const password = window.lastGeneratedPassword;
    if (!password) return;
    
    navigator.clipboard.writeText(password).then(() => {
        notify('📋 Mot de passe copié!', 'success');
    }).catch(() => {
        notify('Erreur copie', 'error');
    });
}

function autoGeneratePassword() {
    const password = generateSecurePassword(16);
    document.getElementById('add-password').value = password;
    document.getElementById('add-password').type = 'text';
    
    const result = checkPasswordStrength(password);
    const bar = document.getElementById('add-strength-bar');
    bar.style.setProperty('--strength', result.width);
    bar.style.setProperty('--strength-color', result.color);
    
    notify('🎲 Mot de passe généré !', 'success');
}

// ==================== UTILITAIRES ====================

function togglePassword(inputId) {
    const input = document.getElementById(inputId);
    input.type = input.type === 'password' ? 'text' : 'password';
}

function escapeHtml(text) {
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#x27;',
        '/': '&#x2F;'
    };
    return String(text).replace(/[&<>"'/]/g, m => map[m]);
}

// ==================== DÉCONNEXION SÉCURISÉE ====================

async function logout() {
    console.log('🚪 Déconnexion sécurisée');
    
    if (autoLockTimer) {
        clearTimeout(autoLockTimer);
        autoLockTimer = null;
    }
    
    if (clipboardTimeout) {
        clearTimeout(clipboardTimeout);
        navigator.clipboard.writeText('').catch(() => {});
        clipboardTimeout = null;
    }
    
    const token = localStorage.getItem('session_token');
    if (token) {
        try {
            await fetch(`${API_URL}/logout`, {
                method: 'POST',
                headers: { 
                    'Authorization': `Bearer ${token}`,
                    'Content-Type': 'application/json'
                }
            });
        } catch (err) {}
    }
    
    currentSession = null;
    isLocked = false;
    
    localStorage.removeItem('session_token');
    localStorage.removeItem('csrf_token');
    localStorage.removeItem('current_tab');
    
    document.querySelectorAll('input, textarea').forEach(input => {
        input.value = '';
    });
    
    showScreen('login-screen');
    
    setTimeout(() => {
        window.location.href = window.location.origin;
    }, 100);
    
    notify('🔒 Déconnecté - Cache nettoyé', 'success');
}

// ==================== GESTION DES FORMULAIRES SÉCURISÉE ====================

// ==================== GESTION DES FORMULAIRES SÉCURISÉE ====================

document.addEventListener('DOMContentLoaded', function() {
    const forms = document.querySelectorAll('form');
    forms.forEach(form => {
        form.setAttribute('autocomplete', 'off');
        form.setAttribute('novalidate', 'novalidate');
        form.setAttribute('data-lpignore', 'true');
        form.setAttribute('data-form-type', 'other');
        
        // Identifier les champs de confirmation pour les protéger différemment
        const confirmationFields = ['register-confirm', 'add-password-confirm'];
        
        // Protection renforcée seulement pour les champs sensibles
        form.querySelectorAll('input[type="password"]').forEach(input => {
            // Ne pas appliquer la protection aux champs de confirmation
            if (confirmationFields.includes(input.id)) {
                input.setAttribute('autocomplete', 'new-password');
                input.setAttribute('data-lpignore', 'true');
                return; // Skip protection for confirmation fields
            }
            
            // Protection normale pour les autres champs password
            input.setAttribute('autocomplete', 'new-password');
            input.setAttribute('data-lpignore', 'true');
            input.setAttribute('readonly', 'true');
            input.setAttribute('spellcheck', 'false');
            input.setAttribute('data-1p-ignore', 'true');
            input.setAttribute('data-bwignore', 'true');
            
            input.addEventListener('focus', function() {
                this.removeAttribute('readonly');
            });
            
            input.addEventListener('blur', function() {
                this.setAttribute('readonly', 'true');
                // NE PAS VIDER LE CHAMP
            });
            
            input.addEventListener('contextmenu', function(e) {
                e.preventDefault();
                return false;
            });
        });
    });
    
    document.body.setAttribute('data-lpignore', 'true');
});

// Empêcher le drag & drop de données sensibles
document.addEventListener('dragstart', (e) => {
    if (e.target.tagName === 'INPUT') {
        e.preventDefault();
        return false;
    }
});

// Empêcher la copie depuis les inputs via le clavier
document.addEventListener('keydown', (e) => {
    const target = e.target;
    if (target.tagName === 'INPUT' && (e.ctrlKey || e.metaKey) && 
        (e.key === 'c' || e.key === 'v' || e.key === 'x')) {
        e.preventDefault();
        return false;
    }
});

// ==================== GESTION CLAVIER ET FENÊTRE ====================

document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        closeModal();
        closeGenerateModal();
    }
});

document.getElementById('password-length').addEventListener('input', function() {
    document.getElementById('length-value').textContent = this.value;
});

// Détecteurs d'activité
['mousemove', 'keydown', 'click', 'scroll', 'touchstart'].forEach(event => {
    document.addEventListener(event, resetActivityTimer, { passive: true });
});

// Verrouiller quand la fenêtre perd le focus
window.addEventListener('blur', () => {
    if (currentSession && !isLocked) {
        setTimeout(() => {
            if (!document.hasFocus() && currentSession && !isLocked) {
                realLockApp();
            }
        }, 10000);
    }
});

// Empêcher les multi-onglets
window.addEventListener('storage', (e) => {
    if (e.key === 'session_token' || e.key === 'current_tab') {
        if (currentSession) {
            notify('🔒 Session utilisée ailleurs', 'warning');
            logout();
        }
    }
});

// Verrouiller avant fermeture
window.addEventListener('beforeunload', () => {
    if (clipboardTimeout) {
        clearTimeout(clipboardTimeout);
        navigator.clipboard.writeText('').catch(() => {});
    }
});