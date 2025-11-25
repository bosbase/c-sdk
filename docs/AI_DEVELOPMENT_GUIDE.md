# AI Development Guide - C SDK

This guide provides a comprehensive, fast reference for AI systems to quickly develop applications using the BosBase C SDK. All examples are production-ready and follow best practices.

## Table of Contents

1. [Authentication](#authentication)
2. [Initialize Collections](#initialize-collections)
3. [Define Collection Fields](#define-collection-fields)
4. [Add Data to Collections](#add-data-to-collections)
5. [Modify Collection Data](#modify-collection-data)
6. [Delete Data from Collections](#delete-data-from-collections)
7. [Query Collection Contents](#query-collection-contents)
8. [Add and Delete Fields from Collections](#add-and-delete-fields-from-collections)
9. [Query Collection Field Information](#query-collection-field-information)
10. [Upload Files](#upload-files)
11. [Query Logs](#query-logs)

---

## Authentication

### Initialize Client

```c
#include "bosbase_c.h"

bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
```

### Password Authentication

```c
// Authenticate with email/username and password
char* auth_json = NULL;
if (bosbase_collection_auth_with_password(pb, "users", 
        "user@example.com", "password123", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL) == 0) {
    // Auth data is automatically stored
    const char* token = bosbase_auth_token(pb);
    char* record = bosbase_auth_record(pb);
    printf("Token: %s\n", token);
    printf("Record: %s\n", record);
    bosbase_free_string(record);
    bosbase_free_string(auth_json);
}
```

### Check Authentication Status

```c
const char* token = bosbase_auth_token(pb);
if (token) {
    char* record = bosbase_auth_record(pb);
    printf("Authenticated as: %s\n", record);
    bosbase_free_string(record);
} else {
    printf("Not authenticated\n");
}
```

### Logout

```c
bosbase_auth_clear(pb);
```

---

## Initialize Collections

### Create Base Collection

```c
const char* collection_body = 
    "{"
    "\"type\":\"base\","
    "\"name\":\"articles\","
    "\"fields\":["
    "{\"name\":\"title\",\"type\":\"text\",\"required\":true,\"min\":6,\"max\":100},"
    "{\"name\":\"description\",\"type\":\"text\"}"
    "],"
    "\"listRule\":\"@request.auth.id != \\\"\\\" || status = \\\"public\\\"\","
    "\"viewRule\":\"@request.auth.id != \\\"\\\" || status = \\\"public\\\"\""
    "}";

char* created_json = NULL;
if (bosbase_send(pb, "/api/collections", "POST", collection_body, 
        "{}", "{}", 30000, NULL, 0, &created_json, NULL) == 0) {
    printf("Collection created: %s\n", created_json);
    bosbase_free_string(created_json);
}
```

### Create Auth Collection

```c
const char* auth_collection = 
    "{"
    "\"type\":\"auth\","
    "\"name\":\"users\","
    "\"fields\":["
    "{\"name\":\"name\",\"type\":\"text\",\"required\":true}"
    "]}";

char* auth_created = NULL;
bosbase_send(pb, "/api/collections", "POST", auth_collection, 
    "{}", "{}", 30000, NULL, 0, &auth_created, NULL);
bosbase_free_string(auth_created);
```

---

## Define Collection Fields

### Add Fields to Collection

```c
// Get existing collection
char* collection_json = NULL;
bosbase_send(pb, "/api/collections/articles", "GET", NULL, 
    "{}", "{}", 30000, NULL, 0, &collection_json, NULL);

// Parse JSON, add field, then update
// Field definition example:
// {
//   "name": "views",
//   "type": "number",
//   "min": 0,
//   "onlyInt": true
// }

// Update collection with new field
// bosbase_send(pb, "/api/collections/articles", "PATCH", updated_json, ...);

bosbase_free_string(collection_json);
```

---

## Add Data to Collections

### Create Record

```c
const char* body = "{\"title\":\"My Article\",\"description\":\"Content\"}";
char* created_json = NULL;
if (bosbase_collection_create(pb, "articles", body, NULL, 0, 
        NULL, NULL, "{}", "{}", &created_json, NULL) == 0) {
    printf("Record created: %s\n", created_json);
    bosbase_free_string(created_json);
}
```

---

## Modify Collection Data

### Update Record

```c
const char* update_body = "{\"title\":\"Updated Title\"}";
char* updated_json = NULL;
if (bosbase_collection_update(pb, "articles", "RECORD_ID", update_body, 
        NULL, 0, NULL, NULL, "{}", "{}", &updated_json, NULL) == 0) {
    printf("Record updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

---

## Delete Data from Collections

### Delete Record

```c
bosbase_error* err = NULL;
if (bosbase_collection_delete(pb, "articles", "RECORD_ID", 
        NULL, "{}", "{}", &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

---

## Query Collection Contents

### List Records

```c
char* records_json = NULL;
if (bosbase_collection_get_list(pb, "articles", 1, 20, 
        NULL, NULL, NULL, NULL, "{}", "{}", 
        &records_json, NULL) == 0) {
    printf("Records: %s\n", records_json);
    bosbase_free_string(records_json);
}
```

### Get Single Record

```c
char* record_json = NULL;
if (bosbase_collection_get_one(pb, "articles", "RECORD_ID", 
        NULL, NULL, "{}", "{}", &record_json, NULL) == 0) {
    printf("Record: %s\n", record_json);
    bosbase_free_string(record_json);
}
```

---

## Add and Delete Fields from Collections

See [Collections Documentation](./COLLECTIONS.md) for detailed field management examples.

---

## Query Collection Field Information

### Get Schema

```c
char* schema_json = NULL;
if (bosbase_send(pb, "/api/collections/articles/schema", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &schema_json, NULL) == 0) {
    // Parse schema_json to get field information
    printf("Schema: %s\n", schema_json);
    bosbase_free_string(schema_json);
}
```

---

## Upload Files

### Upload File with Record

```c
bosbase_file_attachment files[1] = {
    {
        .field = "cover",
        .filename = "photo.jpg",
        .content_type = "image/jpeg",
        .data = image_data,
        .data_len = image_size
    }
};

const char* body = "{\"title\":\"My Article\"}";
char* result_json = NULL;
if (bosbase_collection_create(pb, "articles", body, files, 1, 
        NULL, NULL, "{}", "{}", &result_json, NULL) == 0) {
    printf("Created with file: %s\n", result_json);
    bosbase_free_string(result_json);
}
```

---

## Query Logs

```c
// Authenticate as superuser first
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Query logs
char* logs_json = NULL;
if (bosbase_send(pb, "/api/logs", "GET", NULL, 
        "{\"page\":1,\"perPage\":50}", "{}", 30000, NULL, 0, 
        &logs_json, NULL) == 0) {
    printf("Logs: %s\n", logs_json);
    bosbase_free_string(logs_json);
}
```

---

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Authenticate
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "_superusers", 
        "admin@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // Create collection
    const char* collection = 
        "{\"type\":\"base\",\"name\":\"articles\","
        "\"fields\":[{\"name\":\"title\",\"type\":\"text\",\"required\":true}]}";
    char* created = NULL;
    bosbase_send(pb, "/api/collections", "POST", collection, 
        "{}", "{}", 30000, NULL, 0, &created, NULL);
    bosbase_free_string(created);
    
    // Create record
    const char* body = "{\"title\":\"My Article\"}";
    char* record = NULL;
    bosbase_collection_create(pb, "articles", body, NULL, 0, 
        NULL, NULL, "{}", "{}", &record, NULL);
    printf("Record: %s\n", record);
    bosbase_free_string(record);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Collections](./COLLECTIONS.md) - Detailed collection operations
- [API Records](./API_RECORDS.md) - Record operations
- [Authentication](./AUTHENTICATION.md) - Authentication guide

