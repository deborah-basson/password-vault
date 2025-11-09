const API_URL = window.location.origin + '/api';
let currentSession = null;

function showScreen(screenId) {
    document.querySelectorAll('.screen').forEach(s => s.classList.add('hidden'));
    const screen = document.getElementById(screenId);
    if (screen) screen.classList.remove('hidden');
}

function showLogin() {
    showScreen('login-screen');
}

function showDashboard() {
    showScreen('dashboard-screen');
    loadPasswords();
}

function logout() {
    currentSession = null;
    localStorage.removeItem('session_token');
    localStorage.removeItem('csrf_token');
    showScreen('login-screen');
    notify('Déconnecté avec succès', 'success');
}

function notify(message, type = 'success') {
    const notif = document.getElementById('notification');
    notif.textContent = message;
    notif.className = `notification ${type}`;
    
    setTimeout(() => {
        notif.classList.add('hidden');
    }, 4000);
}

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
            showLogin();
        } else {
            notify(data.error || 'Erreur lors de l\'inscription', 'error');
        }
    } catch (err) {
        notify('Erreur réseau', 'error');
        console.error(err);
    }
});

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
            notify('Connexion réussie ! 🔒', 'success');
            showDashboard();
        } else {
            notify(data.error || 'Mot de passe incorrect', 'error');
        }
    } catch (err) {
        notify('Erreur réseau', 'error');
        console.error(err);
    }
});

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
                    <button class="btn btn-secondary" onclick="copyPassword(${pwd.id})">Copier</button>
                    <button class="btn btn-danger" onclick="deletePassword(${pwd.id})">Supprimer</button>
                </div>
            </div>
        `).join('');
    } catch (err) {
        notify('Erreur chargement', 'error');
        console.error(err);
    }
}

const searchInput = document.getElementById('search-input');
if (searchInput) {
    searchInput.addEventListener('input', (e) => {
        const query = e.target.value.toLowerCase();
        document.querySelectorAll('.password-card').forEach(card => {
            const website = card.querySelector('h3').textContent.toLowerCase();
            const username = card.querySelector('.username').textContent.toLowerCase();
            card.style.display = (website.includes(query) || username.includes(query)) ? 'block' : 'none';
        });
    });
}

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

async function deletePassword(id) {
    if (!confirm('Supprimer ce mot de passe ?')) return;
    
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
            notify('Supprimé', 'success');
            loadPasswords();
        } else {
            notify('Erreur', 'error');
        }
    } catch (err) {
        notify('Erreur réseau', 'error');
        console.error(err);
    }
}

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
        notify('Copié !', 'success');
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
    
    notify('Mot de passe généré !', 'success');
}

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

function copyPassword(id) {
    notify('Fonctionnalité à implémenter', 'error');
}

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
        console.error('Erreur serveur:', err);
        showScreen('register-screen');
    }
    
    updateStrengthBar('register-password', 'strength-bar', 'strength-text');
    updateStrengthBar('add-password', 'add-strength-bar', null);
});

document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        closeModal();
        closeGenerateModal();
    }
});