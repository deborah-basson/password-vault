BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS security_questions_new (
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

DROP TABLE IF EXISTS security_questions;

ALTER TABLE security_questions_new RENAME TO security_questions;

COMMIT;
