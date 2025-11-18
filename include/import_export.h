#ifndef IMPORT_EXPORT_H
#define IMPORT_EXPORT_H

#include <stddef.h>
#include "database.h"

typedef struct {
    char version[16];
    char export_date[64];
    int password_count;
    char checksum[64];
} ExportMetadata;

int export_vault_to_file(Database *db, const char *filepath, const char *export_password);
int import_vault_from_file(Database *db, const char *filepath, const char *export_password);
int export_passwords_encrypted(Database *db, const char *export_password, char **output_json, size_t *output_len);
int import_passwords_encrypted(Database *db, const char *input_json, size_t input_len, const char *export_password);

#endif