#include "../include/file_operations.h"
#include <stdio.h>
#include <string.h>

int save_to_file(const PasswordManager *manager) {
    if (!manager) return 0;
    
    FILE *file = fopen(manager->data_file, "wb");
    if (!file) return 0;
    
    int result = fwrite(manager, sizeof(PasswordManager), 1, file);
    fclose(file);
    
    return result == 1;
}

int load_from_file(PasswordManager *manager) {
    if (!manager) return 0;
    
    FILE *file = fopen(manager->data_file, "rb");
    if (!file) return 0;
    
    size_t result = fread(manager, sizeof(PasswordManager), 1, file);
    fclose(file);
    
    return result == 1;
}