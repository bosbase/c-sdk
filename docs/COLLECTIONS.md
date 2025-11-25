# Collections - C SDK Documentation

## Overview

**Collections** represent your application data. Under the hood they are backed by plain SQLite tables that are generated automatically with the collection **name** and **fields** (columns).

A single entry of a collection is called a **record** (a single row in the SQL table).

## Collection Types

### Base Collection

Default collection type for storing any application data.

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Authenticate as superuser
    char* auth_json = NULL;
    bosbase_error* auth_err = NULL;
    if (bosbase_collection_auth_with_password(pb, "_superusers", 
            "admin@example.com", "password", "{}", NULL, NULL, "{}", "{}", 
            &auth_json, &auth_err) != 0) {
        fprintf(stderr, "Auth failed: %s\n", auth_err->message);
        bosbase_free_error(auth_err);
        bosbase_client_free(pb);
        return 1;
    }
    bosbase_free_string(auth_json);
    
    // Create base collection using bosbase_send
    const char* collection_body = 
        "{\"type\":\"base\",\"name\":\"articles\",\"fields\":["
        "{\"name\":\"title\",\"type\":\"text\",\"required\":true},"
        "{\"name\":\"description\",\"type\":\"text\"}"
        "]}";
    
    char* result_json = NULL;
    bosbase_error* err = NULL;
    if (bosbase_send(pb, "/api/collections", "POST", collection_body, 
            "{}", "{}", 30000, NULL, 0, &result_json, &err) == 0) {
        printf("Collection created: %s\n", result_json);
        bosbase_free_string(result_json);
    } else {
        fprintf(stderr, "Error: %s\n", err->message);
        bosbase_free_error(err);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

### View Collection

Read-only collection populated from a SQL SELECT statement.

```c
const char* view_body = 
    "{\"type\":\"view\",\"name\":\"post_stats\","
    "\"options\":{\"query\":\"SELECT posts.id, posts.name, "
    "count(comments.id) as totalComments FROM posts "
    "LEFT JOIN comments on comments.postId = posts.id "
    "GROUP BY posts.id\"}}";

char* view_json = NULL;
if (bosbase_send(pb, "/api/collections", "POST", view_body, 
        "{}", "{}", 30000, NULL, 0, &view_json, NULL) == 0) {
    printf("View created: %s\n", view_json);
    bosbase_free_string(view_json);
}
```

### Auth Collection

Base collection with authentication fields (email, password, etc.).

```c
const char* auth_collection_body = 
    "{\"type\":\"auth\",\"name\":\"users\",\"fields\":["
    "{\"name\":\"name\",\"type\":\"text\",\"required\":true}"
    "]}";

char* auth_col_json = NULL;
if (bosbase_send(pb, "/api/collections", "POST", auth_collection_body, 
        "{}", "{}", 30000, NULL, 0, &auth_col_json, NULL) == 0) {
    printf("Auth collection created: %s\n", auth_col_json);
    bosbase_free_string(auth_col_json);
}
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

### Create Collection

```c
// Using scaffolds - create base collection
const char* base_collection = 
    "{\"type\":\"base\",\"name\":\"articles\",\"fields\":["
    "{\"name\":\"title\",\"type\":\"text\",\"required\":true},"
    "{\"name\":\"created\",\"type\":\"autodate\",\"required\":false,"
    "\"onCreate\":true,\"onUpdate\":false},"
    "{\"name\":\"updated\",\"type\":\"autodate\",\"required\":false,"
    "\"onCreate\":true,\"onUpdate\":true}"
    "]}";

char* created_json = NULL;
if (bosbase_send(pb, "/api/collections", "POST", base_collection, 
        "{}", "{}", 30000, NULL, 0, &created_json, NULL) == 0) {
    printf("Created: %s\n", created_json);
    bosbase_free_string(created_json);
}
```

### Update Collection

```c
// Update collection rules
const char* update_body = 
    "{\"listRule\":\"published = true\"}";

char* updated_json = NULL;
if (bosbase_send(pb, "/api/collections/articles", "PATCH", update_body, 
        "{}", "{}", 30000, NULL, 0, &updated_json, NULL) == 0) {
    printf("Updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

### Add Fields to Collection

To add a new field to an existing collection, fetch the collection, add the field to the fields array, and update:

```c
// Get existing collection
char* collection_json = NULL;
if (bosbase_send(pb, "/api/collections/articles", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &collection_json, NULL) == 0) {
    // Parse JSON, add field, then update
    // (In practice, use a JSON library like cJSON or jansson)
    // For example with cJSON:
    // cJSON* json = cJSON_Parse(collection_json);
    // cJSON* fields = cJSON_GetObjectItem(json, "fields");
    // cJSON* new_field = cJSON_CreateObject();
    // cJSON_AddStringToObject(new_field, "name", "views");
    // cJSON_AddStringToObject(new_field, "type", "number");
    // cJSON_AddNumberToObject(new_field, "min", 0);
    // cJSON_AddTrueToObject(new_field, "onlyInt");
    // cJSON_AddItemToArray(fields, new_field);
    // char* updated_str = cJSON_Print(json);
    
    // Update collection
    // bosbase_send(pb, "/api/collections/articles", "PATCH", updated_str, ...);
    
    bosbase_free_string(collection_json);
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
const char* query = "{\"filter\":\"published = true\","
                    "\"sort\":\"-created\","
                    "\"expand\":\"author\"}";

if (bosbase_collection_get_list(pb, "articles", 1, 20, 
        "published = true", "-created", "author", NULL, 
        query, "{}", &records_json, NULL) == 0) {
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
const char* update_body = "{\"title\":\"Updated\",\"views+\":1,\"tags+\":\"new-tag\"}";

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

### BoolField

```c
// Field definition in collection:
// {"name": "published", "type": "bool", "required": true}

// Create record with bool field
const char* body = "{\"published\":true}";
char* result = NULL;
bosbase_collection_create(pb, "articles", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result, NULL);
```

### NumberField

```c
// Field definition:
// {"name": "views", "type": "number", "min": 0}

// Update with field modifier
const char* body = "{\"views+\":1}";
char* result = NULL;
bosbase_collection_update(pb, "articles", "RECORD_ID", body, 
    NULL, 0, NULL, NULL, "{}", "{}", &result, NULL);
```

### TextField

```c
// Field definition:
// {"name": "title", "type": "text", "required": true, "min": 6, "max": 100}

// Create with autogenerate modifier
const char* body = "{\"slug:autogenerate\":\"article-\"}";
char* result = NULL;
bosbase_collection_create(pb, "articles", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result, NULL);
```

### EmailField

```c
// Field definition:
// {"name": "email", "type": "email", "required": true}
```

### URLField

```c
// Field definition:
// {"name": "website", "type": "url"}
```

### EditorField

```c
// Field definition:
// {"name": "content", "type": "editor", "required": true}

// Create record with HTML content
const char* body = "{\"content\":\"<p>HTML content</p>\"}";
char* result = NULL;
bosbase_collection_create(pb, "articles", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result, NULL);
```

### DateField

```c
// Field definition:
// {"name": "published_at", "type": "date"}

// Create with date
const char* body = "{\"published_at\":\"2024-11-10T18:45:27.123Z\"}";
char* result = NULL;
bosbase_collection_create(pb, "articles", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result, NULL);
```

### AutodateField

**Important Note:** Bosbase does not initialize `created` and `updated` fields by default. To use these fields, you must explicitly add them when initializing the collection. For autodate fields, `onCreate` and `onUpdate` must be direct properties of the field object, not nested in an `options` object:

```c
// Field definition with proper structure:
// {
//   "name": "created",
//   "type": "autodate",
//   "required": false,
//   "onCreate": true,   // Set on record creation (direct property)
//   "onUpdate": false   // Don't update on record update (direct property)
// }

// For updated field:
// {
//   "name": "updated",
//   "type": "autodate",
//   "required": false,
//   "onCreate": true,   // Set on record creation (direct property)
//   "onUpdate": true    // Update on record update (direct property)
// }

// The value is automatically set by the backend based on onCreate and onUpdate properties
```

### SelectField

```c
// Single select field definition:
// {"name": "status", "type": "select", 
//  "options": {"values": ["draft", "published"]}, "maxSelect": 1}

// Create record
const char* body = "{\"status\":\"published\"}";
char* result = NULL;
bosbase_collection_create(pb, "articles", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result, NULL);

// Multiple select field definition:
// {"name": "tags", "type": "select", 
//  "options": {"values": ["tech", "design"]}, "maxSelect": 5}

// Update with modifier
const char* update_body = "{\"tags+\":\"marketing\"}";
bosbase_collection_update(pb, "articles", "RECORD_ID", update_body, 
    NULL, 0, NULL, NULL, "{}", "{}", &result, NULL);
```

### FileField

```c
// Field definition:
// {"name": "cover", "type": "file", "maxSelect": 1, 
//  "mimeTypes": ["image/jpeg"]}

// Create with file upload
bosbase_file_attachment files[1] = {
    {
        .field = "cover",
        .filename = "photo.jpg",
        .content_type = "image/jpeg",
        .data = image_data,  // uint8_t* image_data
        .data_len = image_size
    }
};

const char* body = "{\"title\":\"My Article\"}";
char* result = NULL;
bosbase_collection_create(pb, "articles", body, files, 1, 
    NULL, NULL, "{}", "{}", &result, NULL);
```

### RelationField

```c
// Field definition:
// {"name": "author", "type": "relation", 
//  "options": {"collectionId": "users"}, "maxSelect": 1}

// Create record with relation
const char* body = "{\"title\":\"My Article\",\"author\":\"USER_ID\"}";
char* result = NULL;
bosbase_collection_create(pb, "articles", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result, NULL);

// Get record with expanded relation
char* record_json = NULL;
bosbase_collection_get_one(pb, "articles", "RECORD_ID", 
    "author", NULL, "{}", "{}", &record_json, NULL);
```

### JSONField

```c
// Field definition:
// {"name": "metadata", "type": "json"}

// Create record with JSON field
const char* body = "{\"title\":\"My Article\","
                   "\"metadata\":{\"seo\":{\"title\":\"SEO Title\"}}}";
char* result = NULL;
bosbase_collection_create(pb, "articles", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result, NULL);
```

### GeoPointField

```c
// Field definition:
// {"name": "location", "type": "geoPoint"}

// Create record with geo point
const char* body = "{\"name\":\"Tokyo\","
                   "\"location\":{\"lon\":139.6917,\"lat\":35.6586}}";
char* result = NULL;
bosbase_collection_create(pb, "places", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result, NULL);
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Authenticate as superuser
    char* auth_json = NULL;
    bosbase_error* auth_err = NULL;
    if (bosbase_collection_auth_with_password(pb, "_superusers", 
            "admin@example.com", "password", "{}", NULL, NULL, "{}", "{}", 
            &auth_json, &auth_err) != 0) {
        fprintf(stderr, "Auth failed: %s\n", auth_err->message);
        bosbase_free_error(auth_err);
        bosbase_client_free(pb);
        return 1;
    }
    bosbase_free_string(auth_json);
    
    // Create auth collection
    const char* users_collection = 
        "{\"type\":\"auth\",\"name\":\"users\"}";
    char* users_json = NULL;
    bosbase_send(pb, "/api/collections", "POST", users_collection, 
        "{}", "{}", 30000, NULL, 0, &users_json, NULL);
    bosbase_free_string(users_json);
    
    // Create base collection
    const char* articles_collection = 
        "{\"type\":\"base\",\"name\":\"articles\",\"fields\":["
        "{\"name\":\"title\",\"type\":\"text\",\"required\":true},"
        "{\"name\":\"author\",\"type\":\"relation\","
        "\"options\":{\"collectionId\":\"users\"},\"maxSelect\":1}"
        "]}";
    char* articles_json = NULL;
    bosbase_send(pb, "/api/collections", "POST", articles_collection, 
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
    char* user_auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "users", 
        "user@example.com", "password123", "{}", NULL, NULL, "{}", "{}", 
        &user_auth_json, NULL);
    bosbase_free_string(user_auth_json);
    
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

## Error Handling

Always check return values and handle errors:

```c
char* result = NULL;
bosbase_error* err = NULL;

if (bosbase_collection_create(pb, "articles", body, NULL, 0, 
        NULL, NULL, "{}", "{}", &result, &err) != 0) {
    if (err) {
        fprintf(stderr, "Error %d: %s\n", err->status, err->message);
        bosbase_free_error(err);
    }
    return 1;
}

// Use result...
bosbase_free_string(result);
```

## Memory Management

Remember to free all allocated strings:

- `bosbase_free_string()` - For strings returned by SDK functions
- `bosbase_free_error()` - For error objects
- `bosbase_client_free()` - For the client instance

## Related Documentation

- [API Records](./API_RECORDS.md) - Detailed record operations
- [Relations](./RELATIONS.md) - Working with relations
- [Authentication](./AUTHENTICATION.md) - Authentication guide
- [Files](./FILES.md) - File uploads and handling

