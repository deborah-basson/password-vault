#include <criterion/criterion.h>
#include "../include/database.h"
#include "../include/crypto.h"
#include <unistd.h>
#include <stdio.h>

static Database* create_test_db(const char* db_name) {
    Database *db = malloc(sizeof(Database));
    
    unlink(db_name);
    
    if (sqlite3_open(db_name, &db->db) != SQLITE_OK) {
        fprintf(stderr, "Failed to open test database\n");
        free(db);
        return NULL;
    }
    
    sqlite3_exec(db->db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    sqlite3_exec(db->db, "PRAGMA locking_mode = EXCLUSIVE;", NULL, NULL, NULL);
    
    const char *schema = 
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  master_hash TEXT NOT NULL,"
        "  salt TEXT NOT NULL,"
        "  totp_secret TEXT,"
        "  is_2fa_enabled INTEGER DEFAULT 1,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS security_questions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id INTEGER DEFAULT 1,"
        "  question1 TEXT NOT NULL,"
        "  answer1_hash TEXT NOT NULL,"
        "  answer1_salt TEXT NOT NULL,"
        "  question2 TEXT NOT NULL,"
        "  answer2_hash TEXT NOT NULL,"
        "  answer2_salt TEXT NOT NULL,"
        "  question3 TEXT NOT NULL,"
        "  answer3_hash TEXT NOT NULL,"
        "  answer3_salt TEXT NOT NULL,"
        "  recovery_code TEXT NOT NULL,"
        "  recovery_attempts INTEGER DEFAULT 0,"
        "  locked_until DATETIME,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";
    
    char *err_msg = NULL;
    if (sqlite3_exec(db->db, schema, NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Schema error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }
    
    return db;
}

static void close_test_db(Database *db, const char* db_name) {
    if (db) {
        sqlite3_close(db->db);
        free(db);
    }
    unlink(db_name);
}

Test(security_questions, verify_correct_answers) {
    crypto_init();
    
    Database *db = create_test_db("test_correct.db");
    cr_assert_not_null(db, "Failed to create test database");
    
    int created = db_create_user_with_questions(db, "hash", "salt",
        "Ville de naissance?", "Paris",
        "Nom de votre animal?", "Rex",
        "Couleur préférée?", "Bleu",
        "RECOVERY123");
    
    cr_assert_eq(created, 1, "Failed to create user with questions");
    
    int result = db_verify_security_answers(db, "Paris", "Rex", "Bleu");
    cr_assert_eq(result, 1, "Les 3 réponses correctes doivent retourner 1");
    
    close_test_db(db, "test_correct.db");
}

Test(security_questions, verify_one_wrong_answer) {
    crypto_init();
    
    Database *db = create_test_db("test_one_wrong.db");
    cr_assert_not_null(db, "Failed to create test database");
    
    db_create_user_with_questions(db, "hash", "salt",
        "Ville de naissance?", "Paris",
        "Nom de votre animal?", "Rex",
        "Couleur préférée?", "Bleu",
        "RECOVERY123");
    
    int result = db_verify_security_answers(db, "Paris", "MAUVAIS", "Bleu");
    cr_assert_eq(result, 0, "Une réponse incorrecte doit retourner 0");
    
    close_test_db(db, "test_one_wrong.db");
}

Test(security_questions, verify_all_wrong_answers) {
    crypto_init();
    
    Database *db = create_test_db("test_all_wrong.db");
    cr_assert_not_null(db, "Failed to create test database");
    
    db_create_user_with_questions(db, "hash", "salt",
        "Ville de naissance?", "Paris",
        "Nom de votre animal?", "Rex",
        "Couleur préférée?", "Bleu",
        "RECOVERY123");
    
    int result = db_verify_security_answers(db, "Londres", "Medor", "Rouge");
    cr_assert_eq(result, 0, "Toutes les réponses incorrectes doivent retourner 0");
    
    close_test_db(db, "test_all_wrong.db");
}

Test(security_questions, account_lockout_after_three_attempts) {
    crypto_init();
    
    Database *db = create_test_db("test_lockout.db");
    cr_assert_not_null(db, "Failed to create test database");
    
    db_create_user_with_questions(db, "hash", "salt",
        "Ville de naissance?", "Paris",
        "Nom de votre animal?", "Rex",
        "Couleur préférée?", "Bleu",
        "RECOVERY123");
    
    int result1 = db_verify_security_answers(db, "WRONG", "WRONG", "WRONG");
    cr_assert_eq(result1, 0, "Première tentative devrait retourner 0");
    
    int result2 = db_verify_security_answers(db, "WRONG", "WRONG", "WRONG");
    cr_assert_eq(result2, 0, "Deuxième tentative devrait retourner 0");
    
    int result3 = db_verify_security_answers(db, "WRONG", "WRONG", "WRONG");
    cr_assert_eq(result3, -1, "Troisième tentative devrait verrouiller (retour -1)");
    
    close_test_db(db, "test_lockout.db");
}

Test(security_questions, case_sensitive_answers) {
    crypto_init();
    
    Database *db = create_test_db("test_case.db");
    cr_assert_not_null(db, "Failed to create test database");
    
    db_create_user_with_questions(db, "hash", "salt",
        "Ville de naissance?", "Paris",
        "Nom de votre animal?", "Rex",
        "Couleur préférée?", "Bleu",
        "RECOVERY123");
    
    int result = db_verify_security_answers(db, "paris", "rex", "bleu");
    cr_assert_eq(result, 0, "La casse doit être respectée");
    
    close_test_db(db, "test_case.db");
}
