# Collections - C SDK Documentation (Extended)

This document provides comprehensive documentation for working with Collections and Fields in the BosBase C SDK. This documentation is designed to be AI-readable and includes practical examples for all operations.

## Table of Contents

- [Overview](#overview)
- [Collection Types](#collection-types)
- [Collections API](#collections-api)
- [Records API](#records-api)
- [Field Types](#field-types)
- [Examples](#examples)

## Overview

**Collections** represent your application data. Under the hood they are backed by plain SQLite tables that are generated automatically with the collection **name** and **fields** (columns).

A single entry of a collection is called a **record** (a single row in the SQL table).

You can manage your **collections** from the Dashboard, or with the C SDK using `bosbase_send` with the `/api/collections` endpoint.

Similarly, you can manage your **records** from the Dashboard, or with the C SDK using collection-specific functions like `bosbase_collection_create`, `bosbase_collection_get_list`, etc.

## Collection Types

Currently there are 3 collection types: **Base**, **View** and **Auth**.

### Base Collection

**Base collection** is the default collection type and it could be used to store any application data (articles, products, posts, etc.).

```c
#include "bosbase_c.h"

bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");

// Authenticate as superuser
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Create a base collection
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

### View Collection

**View collection** is a read-only collection type where the data is populated from a plain SQL `SELECT` statement.

```c
// Create a view collection
const char* view_collection = 
    "{"
    "\"type\":\"view\","
    "\"name\":\"post_stats\","
    "\"options\":{"
    "\"query\":\"SELECT posts.id, posts.name, count(comments.id) as totalComments "
    "FROM posts LEFT JOIN comments on comments.postId = posts.id "
    "GROUP BY posts.id\""
    "}"
    "}";

char* view_created = NULL;
bosbase_send(pb, "/api/collections", "POST", view_collection, 
    "{}", "{}", 30000, NULL, 0, &view_created, NULL);
bosbase_free_string(view_created);
```

**Note**: View collections don't receive realtime events because they don't have create/update/delete operations.

### Auth Collection

**Auth collection** has everything from the **Base collection** but with some additional special fields to help you manage your app users and also provide various authentication options.

```c
// Create an auth collection
const char* users_collection = 
    "{"
    "\"type\":\"auth\","
    "\"name\":\"users\","
    "\"fields\":["
    "{\"name\":\"name\",\"type\":\"text\",\"required\":true},"
    "{\"name\":\"role\",\"type\":\"select\","
    "\"options\":{\"values\":[\"employee\",\"staff\",\"admin\"]}}"
    "]}";

char* users_created = NULL;
bosbase_send(pb, "/api/collections", "POST", users_collection, 
    "{}", "{}", 30000, NULL, 0, &users_created, NULL);
bosbase_free_string(users_created);
```

## Collections API

### List Collections

```c
char* collections_json = NULL;
if (bosbase_send(pb, "/api/collections", "GET", NULL, 
        "{\"page\":1,\"perPage\":50}", "{}", 30000, NULL, 0, 
        &collections_json, NULL) == 0) {
    printf("Collections: %s\n", collections_json);
    bosbase_free_string(collections_json);
}
```

### Get Collection

```c
char* collection_json = NULL;
if (bosbase_send(pb, "/api/collections/articles", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &collection_json, NULL) == 0) {
    printf("Collection: %s\n", collection_json);
    bosbase_free_string(collection_json);
}
```

### Update Collection

```c
const char* update_body = "{\"listRule\":\"published = true\"}";
char* updated_json = NULL;
if (bosbase_send(pb, "/api/collections/articles", "PATCH", update_body, 
        "{}", "{}", 30000, NULL, 0, &updated_json, NULL) == 0) {
    printf("Updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

### Delete Collection

```c
bosbase_error* err = NULL;
if (bosbase_send(pb, "/api/collections/articles", "DELETE", NULL, 
        "{}", "{}", 30000, NULL, 0, NULL, &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Records API

### List Records

```c
char* records_json = NULL;
if (bosbase_collection_get_list(pb, "articles", 1, 20, 
        "published = true", "-created", "author", NULL, 
        "{}", "{}", &records_json, NULL) == 0) {
    printf("Records: %s\n", records_json);
    bosbase_free_string(records_json);
}
```

### Get Record

```c
char* record_json = NULL;
if (bosbase_collection_get_one(pb, "articles", "RECORD_ID", 
        "author,category", NULL, "{}", "{}", &record_json, NULL) == 0) {
    printf("Record: %s\n", record_json);
    bosbase_free_string(record_json);
}
```

### Create Record

```c
const char* body = "{\"title\":\"My Article\",\"views+\":1}";
char* created_json = NULL;
if (bosbase_collection_create(pb, "articles", body, NULL, 0, 
        NULL, NULL, "{}", "{}", &created_json, NULL) == 0) {
    printf("Created: %s\n", created_json);
    bosbase_free_string(created_json);
}
```

### Update Record

```c
const char* update_body = "{\"title\":\"Updated\",\"views+\":1}";
char* updated_json = NULL;
if (bosbase_collection_update(pb, "articles", "RECORD_ID", update_body, 
        NULL, 0, NULL, NULL, "{}", "{}", &updated_json, NULL) == 0) {
    printf("Updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

### Delete Record

```c
bosbase_error* err = NULL;
if (bosbase_collection_delete(pb, "articles", "RECORD_ID", 
        NULL, "{}", "{}", &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Field Types

See [Collections Documentation](./COLLECTIONS.md) for detailed field type examples.

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Authenticate
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "_superusers", 
        "admin@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // Create collections
    const char* users_col = "{\"type\":\"auth\",\"name\":\"users\"}";
    char* users_json = NULL;
    bosbase_send(pb, "/api/collections", "POST", users_col, 
        "{}", "{}", 30000, NULL, 0, &users_json, NULL);
    bosbase_free_string(users_json);
    
    const char* articles_col = 
        "{\"type\":\"base\",\"name\":\"articles\","
        "\"fields\":["
        "{\"name\":\"title\",\"type\":\"text\",\"required\":true},"
        "{\"name\":\"author\",\"type\":\"relation\","
        "\"options\":{\"collectionId\":\"users\"},\"maxSelect\":1}"
        "]}";
    char* articles_json = NULL;
    bosbase_send(pb, "/api/collections", "POST", articles_col, 
        "{}", "{}", 30000, NULL, 0, &articles_json, NULL);
    bosbase_free_string(articles_json);
    
    // Create and authenticate user
    const char* user_body = 
        "{\"email\":\"user@example.com\","
        "\"password\":\"password123\","
        "\"passwordConfirm\":\"password123\"}";
    char* user_json = NULL;
    bosbase_collection_create(pb, "users", user_body, NULL, 0, 
        NULL, NULL, "{}", "{}", &user_json, NULL);
    bosbase_free_string(user_json);
    
    // Authenticate user
    char* user_auth = NULL;
    bosbase_collection_auth_with_password(pb, "users", 
        "user@example.com", "password123", "{}", NULL, NULL, 
        "{}", "{}", &user_auth, NULL);
    bosbase_free_string(user_auth);
    
    // Create article
    const char* article_body = "{\"title\":\"My Article\",\"author\":\"USER_ID\"}";
    char* article_json = NULL;
    bosbase_collection_create(pb, "articles", article_body, NULL, 0, 
        NULL, NULL, "{}", "{}", &article_json, NULL);
    printf("Article created: %s\n", article_json);
    bosbase_free_string(article_json);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Collections](./COLLECTIONS.md) - Main collections documentation
- [API Records](./API_RECORDS.md) - Record operations
- [Authentication](./AUTHENTICATION.md) - Authentication guide

