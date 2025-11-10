# ============================================
# MAKEFILE - PASSWORD VAULT (HTTPS UNIQUEMENT)
# ============================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -O2 -Wno-unused-result
LDFLAGS = -lsqlite3 -lsodium -lssl -lcrypto -lpthread

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
SQL_DIR = sql
WEB_DIR = web
TEST_DIR = tests
TEST_BUILD_DIR = build/tests
TEST_EXEC = run_tests
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(TEST_BUILD_DIR)/%.o)
TEST_LDFLAGS = -lcriterion $(LDFLAGS)

SOURCES = $(SRC_DIR)/api_server.c \
          $(SRC_DIR)/database.c \
          $(SRC_DIR)/crypto.c \
          $(SRC_DIR)/session.c \
          $(SRC_DIR)/database_security.c \
          $(SRC_DIR)/security_questions.c \
		  $(SRC_DIR)/password_history.c

OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
EXECUTABLE = password_vault

# Fichiers objets pour les tests (exclut api_server.c qui a main())
TEST_OBJ_DEPS = $(BUILD_DIR)/database.o \
                $(BUILD_DIR)/crypto.o \
                $(BUILD_DIR)/session.o \
                $(BUILD_DIR)/database_security.o \
                $(BUILD_DIR)/security_questions.o \
				$(SRC_DIR)/password_history.o

RED = \033[0;31m
GREEN = \033[0;32m
BLUE = \033[0;34m
YELLOW = \033[0;33m
NC = \033[0m

.PHONY: all clean clean-all reset run setup install test ssl help kill status

all: setup $(EXECUTABLE)
	@echo "$(GREEN)✓ Compilation réussie!$(NC)"
	@echo "$(BLUE)→ Lancez avec: make run (HTTPS)$(NC)"

setup:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(WEB_DIR)
	@mkdir -p $(SQL_DIR)
	@mkdir -p $(TEST_BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | setup
	@echo "$(BLUE)→ Compilation de $<...$(NC)"
	@$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(EXECUTABLE): $(OBJECTS)
	@echo "$(BLUE)→ Linkage...$(NC)"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@

kill:
	@echo "$(YELLOW)🔫 Arrêt de tous les processus $(EXECUTABLE)...$(NC)"
	@if pgrep -x $(EXECUTABLE) > /dev/null; then \
		killall $(EXECUTABLE) 2>/dev/null; \
		sleep 0.5; \
		echo "$(GREEN)✓ Processus arrêtés$(NC)"; \
	else \
		echo "$(GREEN)✓ Aucun processus actif$(NC)"; \
	fi

clean:
	@$(MAKE) kill
	@echo "$(BLUE)🧹 Nettoyage des fichiers compilés...$(NC)"
	@rm -rf $(BUILD_DIR) $(EXECUTABLE) $(TEST_EXEC)
	@echo "$(GREEN)✓ Nettoyage terminé (BD préservée)$(NC)"

reset:
	@$(MAKE) kill
	@echo "$(RED)╔════════════════════════════════════════╗$(NC)"
	@echo "$(RED)║  ⚠️  RÉINITIALISATION COMPLÈTE  ⚠️   ║$(NC)"
	@echo "$(RED)╚════════════════════════════════════════╝$(NC)"
	@rm -rf $(BUILD_DIR) $(EXECUTABLE) $(TEST_EXEC)
	@rm -f *.db *.db-shm *.db-wal
	@rm -f /tmp/vault.db /tmp/vault.db-shm /tmp/vault.db-wal
	@rm -f cert.pem key.pem
	@echo "$(GREEN)✓ Réinitialisation terminée$(NC)"

clean-all: reset

run: kill all ssl-check
	@echo "$(GREEN)🚀 Démarrage du serveur HTTPS...$(NC)"
	@echo ""
	@./$(EXECUTABLE)

install:
	@echo "$(BLUE)→ Installation des dépendances...$(NC)"
	@command -v sqlite3 >/dev/null 2>&1 || { \
		echo "$(YELLOW)→ Installation de SQLite3...$(NC)"; \
		sudo apt-get update && sudo apt-get install -y libsqlite3-dev sqlite3; \
	}
	@command -v pkg-config >/dev/null 2>&1 || { \
		echo "$(YELLOW)→ Installation de pkg-config...$(NC)"; \
		sudo apt-get install -y pkg-config; \
	}
	@pkg-config --exists libsodium || { \
		echo "$(YELLOW)→ Installation de libsodium...$(NC)"; \
		sudo apt-get install -y libsodium-dev; \
	}
	@pkg-config --exists openssl || { \
		echo "$(YELLOW)→ Installation de OpenSSL...$(NC)"; \
		sudo apt-get install -y libssl-dev; \
	}
	@pkg-config --exists criterion || { \
		echo "$(YELLOW)→ Installation de Criterion...$(NC)"; \
		sudo apt-get install -y libcriterion-dev; \
	}
	@echo "$(GREEN)✓ Dépendances installées$(NC)"

ssl:
	@echo "$(BLUE)→ Génération des certificats SSL...$(NC)"
	@openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes \
		-subj "/C=FR/ST=IDF/L=Paris/O=PasswordVault/CN=localhost" 2>/dev/null
	@chmod 600 key.pem cert.pem
	@echo "$(GREEN)✓ Certificats générés$(NC)"

ssl-check:
	@if [ ! -f cert.pem ] || [ ! -f key.pem ]; then \
		echo "$(YELLOW)⚠️  Certificats SSL manquants. Génération...$(NC)"; \
		$(MAKE) ssl; \
	fi

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | setup
	@$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(TEST_EXEC): $(TEST_OBJ_DEPS) $(TEST_OBJECTS)
	@echo "$(BLUE)→ Compilation des tests...$(NC)"
	@$(CC) $(TEST_OBJ_DEPS) $(TEST_OBJECTS) $(TEST_LDFLAGS) -o $@

test: setup $(TEST_EXEC)
	@echo "$(BLUE)→ Nettoyage de la base de données de test...$(NC)"
	@rm -f vault.db vault.db-shm vault.db-wal
	@echo "$(BLUE)→ Arrêt des processus serveur existants...$(NC)"
	@pkill -x $(EXECUTABLE) 2>/dev/null || true
	@sleep 0.5
	@echo "$(BLUE)→ Exécution de la suite de tests...$(NC)"
	@./$(TEST_EXEC) --jobs=1 --verbose || true
	@echo "$(GREEN)✓ Tests terminés$(NC)"

status:
	@echo "$(BLUE)╔════════════════════════════════════════╗$(NC)"
	@echo "$(BLUE)║        📊 ÉTAT DU SYSTÈME 📊          ║$(NC)"
	@echo "$(BLUE)╠════════════════════════════════════════╣$(NC)"
	@ps aux | grep $(EXECUTABLE) | grep -v grep >/dev/null && \
		echo "$(BLUE)║$(NC) 🟢 Serveur: $(GREEN)ACTIF (HTTPS)$(NC)            $(BLUE)║$(NC)" || \
		echo "$(BLUE)║$(NC) ⚫ Serveur: $(RED)ARRÊTÉ$(NC)                     $(BLUE)║$(NC)"
	@if [ -f /tmp/vault.db ]; then \
		USERS=$$(sqlite3 /tmp/vault.db "SELECT COUNT(*) FROM users;" 2>/dev/null); \
		PASSWORDS=$$(sqlite3 /tmp/vault.db "SELECT COUNT(*) FROM passwords;" 2>/dev/null); \
		echo "$(BLUE)║$(NC) 📁 Base: /tmp/vault.db               $(BLUE)║$(NC)"; \
		echo "$(BLUE)║$(NC)    → $$USERS utilisateur(s)                  $(BLUE)║$(NC)"; \
		echo "$(BLUE)║$(NC)    → $$PASSWORDS mot(s) de passe enregistré(s)  $(BLUE)║$(NC)"; \
	else \
		echo "$(BLUE)║$(NC) ⚫ Base de données: $(YELLOW)NON CRÉÉE$(NC)        $(BLUE)║$(NC)"; \
	fi
	@netstat -tuln 2>/dev/null | grep 8443 >/dev/null && \
		echo "$(BLUE)║$(NC) 🟢 Port 8443: $(GREEN)ACTIF (HTTPS)$(NC)          $(BLUE)║$(NC)" || \
		echo "$(BLUE)║$(NC) ⚫ Port 8443: $(YELLOW)LIBRE$(NC)                   $(BLUE)║$(NC)"
	@echo "$(BLUE)╚════════════════════════════════════════╝$(NC)"

help:
	@echo "$(BLUE)╔════════════════════════════════════════════════╗$(NC)"
	@echo "$(BLUE)║    🔐 PASSWORD VAULT - Commandes 🔐           ║$(NC)"
	@echo "$(BLUE)╠════════════════════════════════════════════════╣$(NC)"
	@echo "$(BLUE)║$(NC) make run       - Lancer le serveur (HTTPS)     $(BLUE)║$(NC)"
	@echo "$(BLUE)║$(NC) make test      - Exécuter les tests            $(BLUE)║$(NC)"
	@echo "$(BLUE)║$(NC) make security-test - Tests de sécurité        $(BLUE)║$(NC)"
	@echo "$(BLUE)║$(NC) make clean     - Nettoyer (garde la BD)        $(BLUE)║$(NC)"
	@echo "$(BLUE)║$(NC) make reset     - TOUT réinitialiser 💥         $(BLUE)║$(NC)"
	@echo "$(BLUE)║$(NC) make kill      - Tuer les processus            $(BLUE)║$(NC)"
	@echo "$(BLUE)║$(NC) make status    - Afficher l'état               $(BLUE)║$(NC)"
	@echo "$(BLUE)║$(NC) make ssl       - Générer certificats SSL       $(BLUE)║$(NC)"
	@echo "$(BLUE)║$(NC) make install   - Installer dépendances         $(BLUE)║$(NC)"
	@echo "$(BLUE)╚════════════════════════════════════════════════╝$(NC)"