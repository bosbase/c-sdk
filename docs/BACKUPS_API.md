# Backups API - C SDK Documentation

## Overview

The Backups API provides endpoints for managing application data backups. You can create backups, upload existing backup files, download backups, delete backups, and restore the application from a backup.

**Key Features:**
- List all available backup files
- Create new backups with custom names or auto-generated names
- Upload existing backup ZIP files
- Download backup files (requires file token)
- Delete backup files
- Restore the application from a backup (restarts the app)

**Backend Endpoints:**
- `GET /api/backups` - List backups
- `POST /api/backups` - Create backup
- `POST /api/backups/upload` - Upload backup
- `GET /api/backups/{key}` - Download backup
- `DELETE /api/backups/{key}` - Delete backup
- `POST /api/backups/{key}/restore` - Restore backup

**Note**: All Backups API operations require superuser authentication (except download which requires a superuser file token).

## Authentication

All Backups API operations require superuser authentication:

```c
#include "bosbase_c.h"

bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");

// Authenticate as superuser
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);
```

**Downloading backups** requires a superuser file token (obtained via `bosbase_files_get_token()`), but does not require the Authorization header.

## List Backups

Returns a list of all available backup files with their metadata.

```c
// Get all backups
char* backups_json = NULL;
if (bosbase_send(pb, "/api/backups", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &backups_json, NULL) == 0) {
    // Parse backups_json to get array of backup objects:
    // Each backup contains:
    // - key: string - The filename/key of the backup
    // - size: number - File size in bytes
    // - modified: string - ISO 8601 timestamp
    printf("Backups: %s\n", backups_json);
    bosbase_free_string(backups_json);
}
```

## Create Backup

Creates a new backup of the application data.

```c
// Create backup with auto-generated name
const char* create_body = "{}";
char* backup_json = NULL;
if (bosbase_send(pb, "/api/backups", "POST", create_body, 
        "{}", "{}", 30000, NULL, 0, &backup_json, NULL) == 0) {
    printf("Backup created: %s\n", backup_json);
    bosbase_free_string(backup_json);
}

// Create backup with custom name
const char* named_body = "{\"name\":\"my-backup.zip\"}";
char* named_backup = NULL;
bosbase_send(pb, "/api/backups", "POST", named_body, 
    "{}", "{}", 30000, NULL, 0, &named_backup, NULL);
bosbase_free_string(named_backup);
```

## Upload Backup

Upload an existing backup ZIP file.

```c
// Read backup file into memory
FILE* file = fopen("backup.zip", "rb");
fseek(file, 0, SEEK_END);
long file_size = ftell(file);
fseek(file, 0, SEEK_SET);
uint8_t* file_data = malloc(file_size);
fread(file_data, 1, file_size, file);
fclose(file);

// Upload backup
bosbase_file_attachment files[1] = {
    {
        .field = "file",
        .filename = "backup.zip",
        .content_type = "application/zip",
        .data = file_data,
        .data_len = file_size
    }
};

char* upload_result = NULL;
if (bosbase_send(pb, "/api/backups/upload", "POST", "{}", 
        "{}", "{}", 30000, files, 1, &upload_result, NULL) == 0) {
    printf("Backup uploaded: %s\n", upload_result);
    bosbase_free_string(upload_result);
}

free(file_data);
```

## Download Backup

Download a backup file (requires file token).

```c
// Get file token first
char* token_json = NULL;
if (bosbase_files_get_token(pb, "{}", "{}", &token_json, NULL) == 0) {
    // Parse token_json to get token string
    const char* token = "FILE_TOKEN";  // From parsed JSON
    
    // Download backup
    char* backup_data = NULL;
    const char* query = "{\"token\":\"FILE_TOKEN\"}";
    if (bosbase_send(pb, "/api/backups/backup_key.zip", "GET", NULL, 
            query, "{}", 30000, NULL, 0, &backup_data, NULL) == 0) {
        // Save backup_data to file
        FILE* out = fopen("downloaded_backup.zip", "wb");
        // Write backup_data...
        fclose(out);
        bosbase_free_string(backup_data);
    }
    bosbase_free_string(token_json);
}
```

## Delete Backup

Delete a backup file.

```c
bosbase_error* err = NULL;
if (bosbase_send(pb, "/api/backups/backup_key.zip", "DELETE", NULL, 
        "{}", "{}", 30000, NULL, 0, NULL, &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Restore Backup

Restore the application from a backup (restarts the app).

```c
const char* restore_body = "{}";
char* restore_result = NULL;
if (bosbase_send(pb, "/api/backups/backup_key.zip/restore", "POST", 
        restore_body, "{}", "{}", 30000, NULL, 0, 
        &restore_result, NULL) == 0) {
    printf("Restore initiated: %s\n", restore_result);
    // Note: This will restart the application
    bosbase_free_string(restore_result);
}
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "_superusers", 
        "admin@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // List backups
    char* backups = NULL;
    bosbase_send(pb, "/api/backups", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &backups, NULL);
    printf("Backups: %s\n", backups);
    bosbase_free_string(backups);
    
    // Create backup
    char* created = NULL;
    bosbase_send(pb, "/api/backups", "POST", "{}", 
        "{}", "{}", 30000, NULL, 0, &created, NULL);
    printf("Created: %s\n", created);
    bosbase_free_string(created);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Health API](./HEALTH_API.md) - Check backup readiness with `canBackup`
- [Files](./FILES.md) - File token generation

