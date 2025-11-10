// ==================== CONFIGURATION ET CONSTANTES ====================
const API_URL = window.location.origin + '/api';
let currentSession = null;
let autoLockTimer = null;
let clipboardTimeout = null;
let lastActivity = Date.now();
let isLocked = false;

const LOCK_TIMEOUT = 180000; // 3 minutes
const CLIPBOARD_TIMEOUT = 30000; // 30 secondes

// ==================== INITIALISATION ====================

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
    
    initializeEventListeners();
    resetActivityTimer();
});

// ==================== GESTION DES ÉCRANS ====================

function showScreen(screenId) {
    console.log('Affichage écran:', screenId);
    document.querySelectorAll('.screen').forEach(s => s.classList.add('hidden'));
    const screen = document.getElementById(screenId);
    if (screen) {
        screen.classList.remove('hidden');
    }
}

function showDashboard() {
    if (isLocked) {
        showScreen('lock-screen');
        return;
    }
    
    showScreen('dashboard-screen');
    loadPasswords();
    resetActivityTimer();
    
    const searchInput = document.getElementById('search-input');
    if (searchInput) {
        searchInput.value = '';
        searchInput.addEventListener('input', filterPasswords);
    }
}

// ==================== INSCRIPTION ====================

function initializeRegisterForm() {
    const registerForm = document.getElementById('register-form');
    if (registerForm) {
        registerForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const password = document.getElementById('register-password').value;
            const confirm = document.getElementById('register-confirm').value;
            
            if (password !== confirm) {
                notify('Les mots de passe ne correspondent pas', 'error');
                return;
            }
            
            const strength = checkPasswordStrength(password);
            if (!strength.isStrong) {
                notify('Mot de passe trop faible. Utilisez des majuscules, minuscules, chiffres et symboles.', 'error');
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
                    showScreen('security-questions-screen');
                    notify('✅ Compte créé - Configurez maintenant vos questions de sécurité', 'success');
                } else {
                    notify(data.error || 'Erreur création compte', 'error');
                }
            } catch (err) {
                console.error('Erreur inscription:', err);
                notify('Erreur réseau lors de la création du compte', 'error');
            }
        });
    }
}

// ==================== QUESTIONS DE SÉCURITÉ ====================

function initializeSecurityQuestionsForm() {
    const securityForm = document.getElementById('security-questions-form');
    if (securityForm) {
        securityForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const question1 = document.getElementById('question1').value;
            const answer1 = document.getElementById('answer1').value;
            const question2 = document.getElementById('question2').value;
            const answer2 = document.getElementById('answer2').value;
            const question3 = document.getElementById('question3').value;
            const answer3 = document.getElementById('answer3').value;
            
            if (!question1 || !answer1 || !question2 || !answer2 || !question3 || !answer3) {
                notify('Veuillez remplir toutes les questions et réponses', 'error');
                return;
            }
            
            try {
                const response = await fetch(`${API_URL}/setup-security-questions`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        question1,
                        answer1,
                        question2,
                        answer2,
                        question3,
                        answer3
                    })
                });
                
                const data = await response.json();
                
                if (response.ok) {
                    document.getElementById('recovery-code-value').textContent = data.recovery_code;
                    showScreen('recovery-code-screen');
                    notify('✅ Questions de sécurité configurées', 'success');
                } else {
                    notify(data.error || 'Erreur configuration questions', 'error');
                }
            } catch (err) {
                console.error('Erreur configuration questions:', err);
                notify('Erreur réseau lors de la configuration', 'error');
            }
        });
    }
}
// ==================== GESTION DES QUESTIONS PRÉDÉFINIES ====================

function updateQuestionPlaceholder(questionNumber) {
    const select = document.getElementById(`question${questionNumber}`);
    const answerInput = document.getElementById(`answer${questionNumber}`);
    
    if (select.value) {
        // Donner un indice basé sur la question sélectionnée
        const question = select.value;
        let placeholder = "Votre réponse";
        
        if (question.includes("ville")) {
            placeholder = "Ex: Paris, Lyon, Marseille...";
        } else if (question.includes("animal")) {
            placeholder = "Ex: Médor, Félix, Rex...";
        } else if (question.includes("école")) {
            placeholder = "Ex: École élémentaire Jean Moulin...";
        } else if (question.includes("mère") || question.includes("père")) {
            placeholder = "Ex: Marie, Pierre, Sophie...";
        } else if (question.includes("métier")) {
            placeholder = "Ex: Médecin, Enseignant, Ingénieur...";
        } else if (question.includes("plat")) {
            placeholder = "Ex: Pizza, Sushi, Couscous...";
        } else if (question.includes("ami")) {
            placeholder = "Ex: Thomas, Sarah, Mohamed...";
        } else if (question.includes("film")) {
            placeholder = "Ex: Star Wars, Le Seigneur des Anneaux...";
        } else if (question.includes("voiture")) {
            placeholder = "Ex: Peugeot 205, Renault Clio...";
        } else if (question.includes("professeur")) {
            placeholder = "Ex: M. Dupont, Mme Martin...";
        } else if (question.includes("série")) {
            placeholder = "Ex: Breaking Bad, Game of Thrones...";
        } else if (question.includes("sport")) {
            placeholder = "Ex: Football, Tennis, Natation...";
        } else if (question.includes("livre")) {
            placeholder = "Ex: Harry Potter, L'Alchimiste...";
        } else if (question.includes("destination")) {
            placeholder = "Ex: Japon, Maldives, New York...";
        } else if (question.includes("artiste")) {
            placeholder = "Ex: Beatles, Beyoncé, Stromae...";
        }
        
        answerInput.placeholder = placeholder;
    }
}

// ==================== QUESTIONS DE SÉCURITÉ ====================

function initializeSecurityQuestionsForm() {
    const securityForm = document.getElementById('security-questions-form');
    if (securityForm) {
        securityForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const question1 = document.getElementById('question1').value;
            const answer1 = document.getElementById('answer1').value;
            const question2 = document.getElementById('question2').value;
            const answer2 = document.getElementById('answer2').value;
            const question3 = document.getElementById('question3').value;
            const answer3 = document.getElementById('answer3').value;
            
            // Validation
            if (!question1 || !answer1 || !question2 || !answer2 || !question3 || !answer3) {
                notify('Veuillez remplir toutes les questions et réponses', 'error');
                return;
            }
            
            // Vérifier que les questions sont différentes
            const questions = [question1, question2, question3];
            const uniqueQuestions = new Set(questions);
            if (uniqueQuestions.size !== 3) {
                notify('Veuillez choisir des questions différentes', 'error');
                return;
            }
            
            try {
                const response = await fetch(`${API_URL}/setup-security-questions`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        question1,
                        answer1,
                        question2,
                        answer2,
                        question3,
                        answer3
                    })
                });
                
                const data = await response.json();
                
                if (response.ok) {
                    document.getElementById('recovery-code-value').textContent = data.recovery_code;
                    showScreen('recovery-code-screen');
                    notify('✅ Questions de sécurité configurées', 'success');
                } else {
                    notify(data.error || 'Erreur configuration questions', 'error');
                }
            } catch (err) {
                console.error('Erreur configuration questions:', err);
                notify('Erreur réseau lors de la configuration', 'error');
            }
        });
    }
}

function validateSecurityQuestions() {
    const question1 = document.getElementById('question1').value;
    const answer1 = document.getElementById('answer1').value;
    const question2 = document.getElementById('question2').value;
    const answer2 = document.getElementById('answer2').value;
    const question3 = document.getElementById('question3').value;
    const answer3 = document.getElementById('answer3').value;
    
    // Vérifier que toutes les questions sont remplies
    if (!question1 || !question2 || !question3) {
        return "Veuillez sélectionner 3 questions";
    }
    
    // Vérifier que les réponses sont remplies
    if (!answer1 || !answer2 || !answer3) {
        return "Veuillez répondre aux 3 questions";
    }
    
    // Vérifier que les questions sont différentes
    const questions = [question1, question2, question3];
    const uniqueQuestions = new Set(questions);
    if (uniqueQuestions.size !== 3) {
        return "Veuillez choisir des questions différentes";
    }
    
    // Vérifier la longueur des réponses
    if (answer1.length < 2 || answer2.length < 2 || answer3.length < 2) {
        return "Les réponses doivent faire au moins 2 caractères";
    }
    
    return null; // Tout est valide
}

// ==================== CODE DE RÉCUPÉRATION ====================

function copyRecoveryCode() {
    const recoveryCode = document.getElementById('recovery-code-value').textContent;
    navigator.clipboard.writeText(recoveryCode).then(() => {
        notify('✅ Code copié dans le presse-papier', 'success');
    }).catch(err => {
        console.error('Erreur copie:', err);
        notify('❌ Erreur lors de la copie', 'error');
    });
}

function finishSetup() {
    showScreen('login-screen');
    notify('✅ Configuration terminée ! Vous pouvez maintenant vous connecter', 'success');
    
    // Nettoyer les formulaires
    document.getElementById('register-password').value = '';
    document.getElementById('register-confirm').value = '';
    document.getElementById('question1').value = '';
    document.getElementById('answer1').value = '';
    document.getElementById('question2').value = '';
    document.getElementById('answer2').value = '';
    document.getElementById('question3').value = '';
    document.getElementById('answer3').value = '';
}

// ==================== CONNEXION ====================

function initializeLoginForm() {
    const loginForm = document.getElementById('login-form');
    if (loginForm) {
        loginForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const password = document.getElementById('login-password').value;
            
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
                        csrf_token: data.csrf_token
                    };
                    localStorage.setItem('session_token', data.session_token);
                    localStorage.setItem('csrf_token', data.csrf_token);
                    
                    notify('✅ Connecté', 'success');
                    showDashboard();
                } else {
                    notify(data.error || 'Connexion échouée', 'error');
                }
            } catch (err) {
                notify('Erreur réseau', 'error');
            }
        });
    }
}

// ==================== FONCTIONNALITÉ MOT DE PASSE OUBLIÉ ====================

function showForgotPassword() {
    showScreen('forgot-password-screen');
    loadSecurityQuestionsForRecovery();
}

function showLoginScreen() {
    showScreen('login-screen');
}

async function loadSecurityQuestionsForRecovery() {
    try {
        const response = await fetch(`${API_URL}/security-questions`);
        const data = await response.json();
        
        const container = document.getElementById('security-questions-container');
        
        if (response.ok && data.success) {
            container.innerHTML = `
                <div class="form-group">
                    <label>${escapeHtml(data.questions.question1)}</label>
                    <input type="text" id="recovery-answer1" placeholder="Votre réponse (optionnel)">
                </div>
                <div class="form-group">
                    <label>${escapeHtml(data.questions.question2)}</label>
                    <input type="text" id="recovery-answer2" placeholder="Votre réponse (optionnel)">
                </div>
                <div class="form-group">
                    <label>${escapeHtml(data.questions.question3)}</label>
                    <input type="text" id="recovery-answer3" placeholder="Votre réponse (optionnel)">
                </div>
                <div class="info" style="margin-bottom: 15px;">
                    <small>💡 Répondez aux 3 questions <strong>OU</strong> utilisez le code de récupération</small>
                </div>
            `;
        } else {
            container.innerHTML = '<div class="warning-box">Aucune question de sécurité configurée. Utilisez le code de récupération.</div>';
        }
    } catch (err) {
        console.error('Erreur chargement questions:', err);
        container.innerHTML = '<div class="warning-box">Erreur de chargement. Utilisez le code de récupération.</div>';
    }
}

function initializeForgotPasswordForm() {
    const form = document.getElementById('forgot-password-form');
    if (form) {
        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const answer1 = document.getElementById('recovery-answer1')?.value || '';
            const answer2 = document.getElementById('recovery-answer2')?.value || '';
            const answer3 = document.getElementById('recovery-answer3')?.value || '';
            const recoveryCode = document.getElementById('recovery-code-input').value || '';

            // Vérifier qu'au moins une méthode est utilisée
            const hasAnswers = answer1.trim() && answer2.trim() && answer3.trim();
            const hasCode = recoveryCode.trim();
            
            if (!hasAnswers && !hasCode) {
                notify('❌ Répondez aux 3 questions OU entrez le code de récupération', 'error');
                return;
            }

            if (hasAnswers && !hasCode) {
                // Vérification par questions seulement
                try {
                    const response = await fetch(`${API_URL}/recover-account`, {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({
                            answer1: answer1,
                            answer2: answer2, 
                            answer3: answer3
                        })
                    });
                    
                    const data = await response.json();
                    
                    if (response.ok) {
                        showScreen('reset-password-screen');
                        notify('✅ Questions validées ! Choisissez un nouveau mot de passe', 'success');
                    } else {
                        notify(data.error || '❌ Réponses incorrectes', 'error');
                    }
                } catch (err) {
                    notify('❌ Erreur réseau lors de la vérification', 'error');
                    console.error(err);
                }
            } else if (hasCode && !hasAnswers) {
                // Vérification par code seulement
                try {
                    const response = await fetch(`${API_URL}/recover-account`, {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({
                            recovery_code: recoveryCode
                        })
                    });
                    
                    const data = await response.json();
                    
                    if (response.ok) {
                        showScreen('reset-password-screen');
                        notify('✅ Code valide ! Choisissez un nouveau mot de passe', 'success');
                    } else {
                        notify(data.error || '❌ Code de récupération invalide', 'error');
                    }
                } catch (err) {
                    notify('❌ Erreur réseau lors de la vérification', 'error');
                    console.error(err);
                }
            } else {
                notify('❌ Utilisez soit les questions soit le code, pas les deux', 'error');
            }
        });
    }
}

function initializeResetPasswordForm() {
    const form = document.getElementById('reset-password-form');
    if (form) {
        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const password = document.getElementById('reset-password').value;
            const confirm = document.getElementById('reset-confirm').value;
            
            if (password !== confirm) {
                notify('❌ Les mots de passe ne correspondent pas', 'error');
                return;
            }
            
            const strength = checkPasswordStrength(password);
            if (!strength.isStrong) {
                notify('❌ Mot de passe trop faible. Utilisez majuscules, minuscules, chiffres et symboles.', 'error');
                return;
            }
            
            try {
                const response = await fetch(`${API_URL}/reset-password`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ password })
                });
                
                const data = await response.json();
                
                if (response.ok) {
                    notify('✅ Mot de passe réinitialisé avec succès !', 'success');
                    
                    // Nettoyer tous les formulaires
                    setTimeout(() => {
                        document.getElementById('reset-password').value = '';
                        document.getElementById('reset-confirm').value = '';
                        document.getElementById('recovery-answer1').value = '';
                        document.getElementById('recovery-answer2').value = '';
                        document.getElementById('recovery-answer3').value = '';
                        document.getElementById('recovery-code-input').value = '';
                        
                        showScreen('login-screen');
                    }, 2000);
                    
                } else {
                    notify(data.error || '❌ Erreur lors de la réinitialisation', 'error');
                }
            } catch (err) {
                notify('❌ Erreur réseau lors de la réinitialisation', 'error');
                console.error(err);
            }
        });
    }
}

// ==================== DÉVERROUILLAGE ====================

function initializeUnlockForm() {
    const unlockForm = document.getElementById('unlock-form');
    if (unlockForm) {
        unlockForm.addEventListener('submit', async (e) => {
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
    }
}

// ==================== CHARGEMENT DES MOTS DE PASSE ====================

async function loadPasswords() {
    const token = localStorage.getItem('session_token');
    if (!token) {
        showScreen('login-screen');
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
            <div class="password-card" data-id="${pwd.id}" data-website="${escapeHtml(pwd.website)}" data-username="${escapeHtml(pwd.username)}">
                <h3>${escapeHtml(pwd.website)}</h3>
                <div class="username">${escapeHtml(pwd.username)}</div>
                <div class="password-actions">
                    <button class="btn btn-secondary" onclick="copyPassword(${pwd.id})">📋 Copier</button>
                    <button class="btn btn-danger" onclick="deletePassword(${pwd.id})">🗑️ Supprimer</button>
                </div>
            </div>
        `).join('');
        
    } catch (err) {
        console.error('Erreur chargement:', err);
        notify('Erreur chargement', 'error');
    }
}

function filterPasswords() {
    const searchTerm = document.getElementById('search-input').value.toLowerCase();
    const cards = document.querySelectorAll('.password-card');
    
    cards.forEach(card => {
        const website = card.dataset.website.toLowerCase();
        const username = card.dataset.username.toLowerCase();
        
        if (website.includes(searchTerm) || username.includes(searchTerm)) {
            card.style.display = '';
        } else {
            card.style.display = 'none';
        }
    });
}

// ==================== COPIER MOT DE PASSE ====================

async function copyPassword(id) {
    const token = localStorage.getItem('session_token');
    const csrf = localStorage.getItem('csrf_token');
    
    if (!token || !csrf) {
        notify('Session invalide', 'error');
        return;
    }
    
    try {
        const response = await fetch(`${API_URL}/passwords/${id}/decrypt`, {
            headers: { 
                'Authorization': `Bearer ${token}`,
                'X-CSRF-Token': csrf
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
            notify('📋 Mot de passe copié (30s)', 'success');
            
            if (clipboardTimeout) clearTimeout(clipboardTimeout);
            clipboardTimeout = setTimeout(() => {
                navigator.clipboard.writeText('').catch(() => {});
                console.log('🧹 Clipboard nettoyé');
            }, CLIPBOARD_TIMEOUT);
        } else {
            notify(data.error || 'Erreur déchiffrement', 'error');
        }
    } catch (err) {
        notify('Erreur réseau', 'error');
        console.error(err);
    }
}

// ==================== SUPPRIMER MOT DE PASSE ====================

async function deletePassword(id) {
    if (!confirm('Supprimer ce mot de passe ?')) return;
    
    const token = localStorage.getItem('session_token');
    const csrf = localStorage.getItem('csrf_token');
    
    try {
        const response = await fetch(`${API_URL}/passwords/${id}`, {
            method: 'DELETE',
            headers: { 
                'Authorization': `Bearer ${token}`,
                'X-CSRF-Token': csrf
            }
        });
        
        if (response.ok) {
            notify('🗑️ Mot de passe supprimé', 'success');
            loadPasswords();
        } else {
            notify('Erreur suppression', 'error');
        }
    } catch (err) {
        notify('Erreur réseau', 'error');
    }
}

// ==================== AJOUTER MOT DE PASSE ====================

function showAddPassword() {
    document.getElementById('add-modal').classList.remove('hidden');
    document.getElementById('add-website').value = '';
    document.getElementById('add-username').value = '';
    document.getElementById('add-password').value = '';
    document.getElementById('add-notes').value = '';
}

function closeModal() {
    document.getElementById('add-modal').classList.add('hidden');
}

function initializeAddPasswordForm() {
    const addForm = document.getElementById('add-password-form');
    if (addForm) {
        addForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const website = document.getElementById('add-website').value;
            const username = document.getElementById('add-username').value;
            const password = document.getElementById('add-password').value;
            const notes = document.getElementById('add-notes').value;
            
            const token = localStorage.getItem('session_token');
            const csrf = localStorage.getItem('csrf_token');
            
            try {
                const response = await fetch(`${API_URL}/passwords`, {
                    method: 'POST',
                    headers: { 
                        'Authorization': `Bearer ${token}`,
                        'X-CSRF-Token': csrf,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({ website, username, password, notes })
                });
                
                if (response.ok) {
                    notify('💾 Mot de passe enregistré', 'success');
                    closeModal();
                    loadPasswords();
                } else {
                    const data = await response.json();
                    notify(data.error || 'Erreur enregistrement', 'error');
                }
            } catch (err) {
                notify('Erreur réseau', 'error');
            }
        });
    }
}

// ==================== GÉNÉRATEUR DE MOT DE PASSE ====================

function generatePasswordPopup() {
    document.getElementById('generate-modal').classList.remove('hidden');
    generateNewPassword();
}

function closeGenerateModal() {
    document.getElementById('generate-modal').classList.add('hidden');
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
    });
}

function autoGeneratePassword() {
    const password = generateSecurePassword(16);
    document.getElementById('add-password').value = password;
    document.getElementById('add-password').type = 'text';
    
    notify('🎲 Mot de passe généré', 'success');
}

// ==================== GÉNÉRATEUR DE PHRASE DE PASSE ====================

async function generatePassphrase() {
    try {
        const token = localStorage.getItem('session_token');
        const response = await fetch(`${API_URL}/generate-passphrase`, {
            headers: { 'Authorization': `Bearer ${token}` }
        });
        
        if (response.ok) {
            const data = await response.json();
            if (data.passphrase) {
                document.getElementById('add-password').value = data.passphrase;
                document.getElementById('add-password').type = 'text';
                notify('🎲 Phrase de passe générée!', 'success');
            }
        }
    } catch (err) {
        console.error('Erreur génération phrase:', err);
        const fallbackPassphrase = generateFallbackPassphrase();
        document.getElementById('add-password').value = fallbackPassphrase;
        document.getElementById('add-password').type = 'text';
        notify('🎲 Phrase de passe générée (fallback)', 'success');
    }
}

function generateFallbackPassphrase() {
    const words = ["secure", "vault", "password", "manager", "crypto", "protection", 
                   "privacy", "access", "control", "defense", "firewall", "gateway"];
    const separators = "-_=+!@#$";
    
    let passphrase = "";
    const wordCount = 3 + Math.floor(Math.random() * 2);
    
    for (let i = 0; i < wordCount; i++) {
        if (i > 0) {
            passphrase += separators[Math.floor(Math.random() * separators.length)];
        }
        passphrase += words[Math.floor(Math.random() * words.length)];
    }
    
    passphrase += separators[Math.floor(Math.random() * separators.length)];
    passphrase += Math.floor(Math.random() * 100).toString().padStart(2, '0');
    passphrase += separators[Math.floor(Math.random() * separators.length)];
    
    return passphrase;
}

// ==================== DÉCONNEXION ====================

async function logout() {
    console.log('🚪 Déconnexion');
    
    if (autoLockTimer) clearTimeout(autoLockTimer);
    if (clipboardTimeout) {
        clearTimeout(clipboardTimeout);
        navigator.clipboard.writeText('').catch(() => {});
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
    
    document.querySelectorAll('input, textarea').forEach(input => input.value = '');
    
    showScreen('login-screen');
    
    notify('🔒 Déconnecté', 'success');
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

function checkPasswordStrength(password) {
    if (!password || password.length < 12) {
        return { isStrong: false, score: 0, width: '0%', color: '#ef4444' };
    }
    
    let score = 0;
    if (/[a-z]/.test(password)) score++;
    if (/[A-Z]/.test(password)) score++;
    if (/[0-9]/.test(password)) score++;
    if (/[^a-zA-Z0-9]/.test(password)) score++;
    
    const colors = ['#ef4444', '#f59e0b', '#fbbf24', '#10b981', '#059669'];
    const widths = ['20%', '40%', '60%', '80%', '100%'];
    
    return {
        isStrong: score >= 3,
        score,
        width: widths[score] || '0%',
        color: colors[score] || '#ef4444'
    };
}

function updateStrengthBar(inputId, barId, textId) {
    const input = document.getElementById(inputId);
    const bar = document.getElementById(barId);
    const text = textId ? document.getElementById(textId) : null;
    
    if (input && bar) {
        input.addEventListener('input', () => {
            const result = checkPasswordStrength(input.value);
            bar.style.setProperty('--strength', result.width);
            bar.style.setProperty('--strength-color', result.color);
            
            if (text) {
                const messages = ['Très faible', 'Faible', 'Moyen', 'Fort', 'Très fort'];
                text.textContent = input.value ? messages[result.score] || '' : '';
            }
        });
    }
}

// ==================== NOTIFICATIONS ====================

function notify(message, type = 'success') {
    const notification = document.getElementById('notification');
    notification.textContent = message;
    notification.className = `notification ${type}`;
    notification.classList.remove('hidden');
    
    setTimeout(() => {
        notification.classList.add('hidden');
    }, 3000);
}


function initializeEventListeners() {
    // Formulaires existants
    initializeRegisterForm();
    initializeLoginForm();
    initializeUnlockForm();
    initializeAddPasswordForm();
    initializeSecurityQuestionsForm();
    
    // NOUVEAUX: Formulaires de récupération
    initializeForgotPasswordForm();
    initializeResetPasswordForm();
    
    // Indicateurs de force
    updateStrengthBar('register-password', 'strength-bar', 'strength-text');
    updateStrengthBar('add-password', 'add-strength-bar', null);
    updateStrengthBar('reset-password', 'reset-strength-bar', 'reset-strength-text');
    
    // Clavier
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            closeModal();
            closeGenerateModal();
        }
    });
    
    // Activité utilisateur
    ['mousemove', 'keydown', 'click', 'scroll', 'touchstart'].forEach(event => {
        document.addEventListener(event, resetActivityTimer, { passive: true });
    });
    
    // Slider longueur
    const lengthSlider = document.getElementById('password-length');
    if (lengthSlider) {
        lengthSlider.addEventListener('input', function() {
            document.getElementById('length-value').textContent = this.value;
        });
    }
    
    // Protection autofill
    const forms = document.querySelectorAll('form');
    forms.forEach(form => {
        form.setAttribute('autocomplete', 'off');
        form.setAttribute('novalidate', 'novalidate');
    });
}