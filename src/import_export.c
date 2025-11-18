#define _POSIX_C_SOURCE 200809L

#include "../include/import_export.h"
#include "../include/crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sodium.h>

static void generate_checksum(const char *data, size_t data_len, char *checksum_out) {
    unsigned char hash[crypto_generichash_BYTES];
    crypto_generichash(hash, sizeof(hash), (unsigned char*)data, data_len, NULL, 0);
    
    for (int i = 0; i < 32 && i < (int)crypto_generichash_BYTES; i++) {
        snprintf(checksum_out + (i * 2), 3, "%02x", hash[i]);
    }
    checksum_out[64] = '\0';
}

static char* escape_json_string(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    char *escaped = malloc(len * 2 + 1);
    size_t j = 0;
    
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            escaped[j++] = '\\';
        }
        escaped[j++] = str[i];
    }
    escaped[j] = '\0';
    return escaped;
}

static void compute_entries_checksum(PasswordEntry *entries, int count, char *checksum_out) {
    size_t buffer_size = count * 2048;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        checksum_out[0] = '\0';
        return;
    }
    
    size_t offset = 0;
    for (int i = 0; i < count; i++) {
        int written = snprintf(buffer + offset, buffer_size - offset,
            "%s|%s|%s|%s|",
            entries[i].website,
            entries[i].username,
            entries[i].encrypted_password,
            entries[i].notes);
        if (written > 0 && (size_t)written < buffer_size - offset) {
            offset += written;
        }
    }
    
    generate_checksum(buffer, offset, checksum_out);
    free(buffer);
}

int export_passwords_encrypted(Database *db, const char *export_password, char **output_json, size_t *output_len) {
    if (!db || !export_password || !output_json) return 0;
    
    PasswordEntry *entries = NULL;
    int count = 0;
    
    if (!db_get_all_passwords(db, &entries, &count)) {
        return 0;
    }
    
    char checksum[65];
    compute_entries_checksum(entries, count, checksum);
    
    size_t json_size = 2048 + (count * 4096);
    char *json = malloc(json_size);
    if (!json) {
        free(entries);
        return 0;
    }
    memset(json, 0, json_size);
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    size_t offset = 0;
    int written = snprintf(json + offset, json_size - offset,
        "{\"version\":\"1.0\",\"export_date\":\"%s\",\"password_count\":%d,\"checksum\":\"%s\",\"passwords\":[",
        date_str, count, checksum);
    if (written < 0 || (size_t)written >= json_size - offset) {
        free(json);
        free(entries);
        return 0;
    }
    offset += written;
    
    for (int i = 0; i < count; i++) {
        char *website = escape_json_string(entries[i].website);
        char *username = escape_json_string(entries[i].username);
        char *password = escape_json_string(entries[i].encrypted_password);
        char *notes = escape_json_string(entries[i].notes);
        
        written = snprintf(json + offset, json_size - offset,
            "%s{\"id\":%d,\"website\":\"%s\",\"username\":\"%s\",\"password\":\"%s\",\"notes\":\"%s\",\"created_at\":\"%s\",\"updated_at\":\"%s\"}",
            i > 0 ? "," : "",
            entries[i].id,
            website,
            username,
            password,
            notes,
            entries[i].created_at,
            entries[i].updated_at);
        
        free(website);
        free(username);
        free(password);
        free(notes);
        
        if (written < 0 || (size_t)written >= json_size - offset) {
            free(json);
            free(entries);
            return 0;
        }
        offset += written;
    }
    
    written = snprintf(json + offset, json_size - offset, "]}");
    if (written < 0 || (size_t)written >= json_size - offset) {
        free(json);
        free(entries);
        return 0;
    }
    offset += written;
    
    free(entries);
    
    size_t encrypted_buf_size = ((offset * 4) / 3 + 4) * 2 + 4096;
    char *encrypted = malloc(encrypted_buf_size);
    size_t encrypted_len = encrypted_buf_size;
    
    if (!encrypted || !encrypt_data(json, export_password, encrypted, &encrypted_len)) {
        free(json);
        if (encrypted) free(encrypted);
        return 0;
    }
    
    secure_zero_memory(json, json_size);
    free(json);
    
    *output_json = encrypted;
    *output_len = encrypted_len;
    
    return 1;
}

int export_vault_to_file(Database *db, const char *filepath, const char *export_password) {
    if (!db || !filepath || !export_password) return 0;
    
    char *encrypted_data = NULL;
    size_t encrypted_len = 0;
    
    if (!export_passwords_encrypted(db, export_password, &encrypted_data, &encrypted_len)) {
        return 0;
    }
    
    FILE *file = fopen(filepath, "wb");
    if (!file) {
        secure_zero_memory(encrypted_data, encrypted_len);
        free(encrypted_data);
        return 0;
    }
    
    size_t written = fwrite(encrypted_data, 1, encrypted_len, file);
    fclose(file);
    
    secure_zero_memory(encrypted_data, encrypted_len);
    free(encrypted_data);
    
    return written == encrypted_len;
}

static int parse_and_import_json(Database *db, const char *json) {
    const char *ptr = strstr(json, "\"password_count\":");
    if (!ptr) return 0;
    
    int count = 0;
    sscanf(ptr + 17, "%d", &count);
    
    ptr = strstr(json, "\"passwords\":[");
    if (!ptr) return 0;
    ptr += 13;
    
    int imported = 0;
    const char *current = ptr;
    
    while (*current && imported < count) {
        current = strchr(current, '{');
        if (!current) break;
        
        const char *next_brace = strchr(current + 1, '}');
        if (!next_brace) break;
        
        char website[256] = {0};
        char username[256] = {0};
        char password[1024] = {0};
        char notes[1024] = {0};
        
        const char *field_start;
        const char *field_end;
        
        field_start = strstr(current, "\"website\":\"");
        if (field_start && field_start < next_brace) {
            field_start += 11;
            field_end = strchr(field_start, '"');
            if (field_end && field_end < next_brace) {
                size_t len = field_end - field_start;
                if (len < sizeof(website)) {
                    strncpy(website, field_start, len);
                    website[len] = '\0';
                }
            }
        }
        
        field_start = strstr(current, "\"username\":\"");
        if (field_start && field_start < next_brace) {
            field_start += 12;
            field_end = strchr(field_start, '"');
            if (field_end && field_end < next_brace) {
                size_t len = field_end - field_start;
                if (len < sizeof(username)) {
                    strncpy(username, field_start, len);
                    username[len] = '\0';
                }
            }
        }
        
        field_start = strstr(current, "\"password\":\"");
        if (field_start && field_start < next_brace) {
            field_start += 12;
            field_end = strchr(field_start, '"');
            if (field_end && field_end < next_brace) {
                size_t len = field_end - field_start;
                if (len < sizeof(password)) {
                    strncpy(password, field_start, len);
                    password[len] = '\0';
                }
            }
        }
        
        field_start = strstr(current, "\"notes\":\"");
        if (field_start && field_start < next_brace) {
            field_start += 9;
            field_end = strchr(field_start, '"');
            if (field_end && field_end < next_brace) {
                size_t len = field_end - field_start;
                if (len < sizeof(notes)) {
                    strncpy(notes, field_start, len);
                    notes[len] = '\0';
                }
            }
        }
        
        if (strlen(website) > 0 && strlen(username) > 0 && strlen(password) > 0) {
            if (db_add_password(db, website, username, password, notes)) {
                imported++;
            }
        }
        
        current = next_brace + 1;
    }
    
    return imported;
}

int import_passwords_encrypted(Database *db, const char *input_json, size_t input_len, const char *export_password) {
    if (!db || !input_json || !export_password) return 0;
    
    char *decrypted = malloc(input_len * 4);
    if (!decrypted) return 0;
    
    if (!decrypt_data(input_json, input_len, export_password, decrypted)) {
        free(decrypted);
        return 0;
    }
    
    const char *checksum_ptr = strstr(decrypted, "\"checksum\":\"");
    if (!checksum_ptr) {
        secure_zero_memory(decrypted, input_len * 4);
        free(decrypted);
        return 0;
    }
    
    char stored_checksum[65];
    memset(stored_checksum, 0, sizeof(stored_checksum));
    const char *checksum_value = checksum_ptr + 12;
    int i = 0;
    while (i < 64 && checksum_value[i] && checksum_value[i] != '"') {
        stored_checksum[i] = checksum_value[i];
        i++;
    }
    stored_checksum[i] = '\0';
    
    int imported = parse_and_import_json(db, decrypted);
    
    if (imported > 0) {
        PasswordEntry *entries = NULL;
        int count = 0;
        
        if (db_get_all_passwords(db, &entries, &count) && count == imported) {
            char computed_checksum[65];
            compute_entries_checksum(entries, count, computed_checksum);
            free(entries);
            
            if (strncmp(stored_checksum, computed_checksum, 64) != 0) {
                secure_zero_memory(decrypted, input_len * 4);
                free(decrypted);
                return 0;
            }
        }
    }
    
    secure_zero_memory(decrypted, input_len * 4);
    free(decrypted);
    
    return imported;
}

int import_vault_from_file(Database *db, const char *filepath, const char *export_password) {
    if (!db || !filepath || !export_password) return 0;
    
    FILE *file = fopen(filepath, "rb");
    if (!file) return 0;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *encrypted_data = malloc(file_size + 1);
    if (!encrypted_data) {
        fclose(file);
        return 0;
    }
    
    size_t read = fread(encrypted_data, 1, file_size, file);
    fclose(file);
    
    if (read != (size_t)file_size) {
        free(encrypted_data);
        return 0;
    }
    
    encrypted_data[file_size] = '\0';
    
    int imported = import_passwords_encrypted(db, encrypted_data, file_size, export_password);
    
    secure_zero_memory(encrypted_data, file_size);
    free(encrypted_data);
    
    return imported;
}