#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "password_manager.h"

int save_to_file(const PasswordManager *manager);
int load_from_file(PasswordManager *manager);
int backup_data(const PasswordManager *manager);
int export_to_csv(const PasswordManager *manager, const char *filename);

#endif