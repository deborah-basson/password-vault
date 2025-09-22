#ifndef UI_H
#define UI_H

#include "password_manager.h"

void clear_screen();
void wait_key();
void show_menu();
void start_ui(PasswordManager *manager);
void show_passwords(const PasswordManager *manager);
void add_password_ui(PasswordManager *manager);
void edit_password_ui(PasswordManager *manager);
void delete_password_ui(PasswordManager *manager);
void settings_ui(PasswordManager *manager);

#endif