#ifndef DATABASE_SECURITY_H
#define DATABASE_SECURITY_H

#include "database.h"

// ==================== STRUCTURES ====================

typedef struct {
    int critical_events_7d;
    int high_events_7d;
    int expired_passwords;
    int failed_logins_24h;
    int total_passwords;
    int active_sessions;
} SecurityDashboard;

// ==================== HISTORIQUE DES MOTS DE PASSE ====================

/**
 * Ajoute un mot de passe à l'historique
 * @param db Base de données
 * @param password_id ID du mot de passe
 * @param encrypted_password Mot de passe chiffré à archiver
 * @return 1 si succès, 0 sinon
 */
int db_add_password_to_history(Database *db, int password_id, const char *encrypted_password);

/**
 * Récupère l'historique des mots de passe
 * @param db Base de données
 * @param password_id ID du mot de passe
 * @param history Tableau pour stocker l'historique
 * @param count Nombre d'entrées récupérées
 * @param max_count Nombre maximum d'entrées
 * @return 1 si succès, 0 sinon
 */
int db_get_password_history(Database *db, int password_id, char history[][512], int *count, int max_count);

// ==================== ÉVÉNEMENTS DE SÉCURITÉ ====================

/**
 * Enregistre un événement de sécurité
 * @param db Base de données
 * @param action Action effectuée (ex: "login_failed", "password_changed")
 * @param severity Sévérité: "low", "medium", "high", "critical"
 * @param description Description détaillée
 * @param ip Adresse IP source
 * @return 1 si succès, 0 sinon
 */
int db_log_security_event(Database *db, const char *action, const char *severity, 
                          const char *description, const char *ip);

/**
 * Récupère les événements de sécurité récents
 * @param db Base de données
 * @param days Nombre de jours à récupérer
 * @param events Tableau pour stocker les événements
 * @param count Nombre d'événements récupérés
 * @param max_count Nombre maximum d'événements
 * @return 1 si succès, 0 sinon
 */
int db_get_security_events(Database *db, int days, char events[][256], int *count, int max_count);

// ==================== POLITIQUES DE SÉCURITÉ ====================

/**
 * Récupère la valeur d'une politique de sécurité
 * @param db Base de données
 * @param policy_name Nom de la politique
 * @param value Pointeur pour stocker la valeur
 * @return 1 si succès, 0 sinon
 */
int db_get_security_policy(Database *db, const char *policy_name, int *value);

/**
 * Met à jour une politique de sécurité
 * @param db Base de données
 * @param policy_name Nom de la politique
 * @param value Nouvelle valeur
 * @return 1 si succès, 0 sinon
 */
int db_update_security_policy(Database *db, const char *policy_name, int value);

// ==================== EXPIRATION DES MOTS DE PASSE ====================

/**
 * Compte les mots de passe expirés
 * @param db Base de données
 * @param max_age_days Âge maximum en jours
 * @return Nombre de mots de passe expirés
 */
int db_get_expired_passwords_count(Database *db, int max_age_days);

/**
 * Récupère les IDs des mots de passe expirés
 * @param db Base de données
 * @param max_age_days Âge maximum en jours
 * @param ids Tableau pour stocker les IDs
 * @param count Nombre d'IDs récupérés
 * @param max_count Nombre maximum d'IDs
 * @return 1 si succès, 0 sinon
 */
int db_get_expired_passwords(Database *db, int max_age_days, int ids[], int *count, int max_count);

// ==================== STATISTIQUES DE SÉCURITÉ ====================

/**
 * Récupère le tableau de bord de sécurité
 * @param db Base de données
 * @param dashboard Structure à remplir
 * @return 1 si succès, 0 sinon
 */
int db_get_security_dashboard(Database *db, SecurityDashboard *dashboard);

// ==================== NETTOYAGE ET MAINTENANCE ====================

/**
 * Nettoie les anciens logs et événements
 * @param db Base de données
 * @param days_to_keep Nombre de jours à conserver
 * @return Nombre d'entrées supprimées
 */
int db_cleanup_old_logs(Database *db, int days_to_keep);

// ==================== DÉTECTION D'ANOMALIES ====================

/**
 * Détecte une activité suspecte pour une IP
 * @param db Base de données
 * @param ip Adresse IP à vérifier
 * @return 1 si activité suspecte détectée, 0 sinon
 */
int db_detect_suspicious_activity(Database *db, const char *ip);

#endif // DATABASE_SECURITY_H