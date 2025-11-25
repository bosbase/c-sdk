# Collection API - C SDK Documentation

## Overview

The Collection API provides endpoints for managing collections (Base, Auth, and View types). All operations require superuser authentication and allow you to create, read, update, and delete collections along with their schemas and configurations.

**Key Features:**
- List and search collections
- View collection details
- Create collections (base, auth, view)
- Update collection schemas and rules
- Delete collections
- Truncate collections (delete all records)
- Import collections in bulk
- Get collection scaffolds (templates)

**Backend Endpoints:**
- `GET /api/collections` - List collections
- `GET /api/collections/{collection}` - View collection
- `POST /api/collections` - Create collection
- `PATCH /api/collections/{collection}` - Update collection
- `DELETE /api/collections/{collection}` - Delete collection
- `DELETE /api/collections/{collection}/truncate` - Truncate collection
- `PUT /api/collections/import` - Import collections
- `GET /api/collections/meta/scaffolds` - Get scaffolds

**Note**: All Collection API operations require superuser authentication.

## Authentication

All Collection API operations require superuser authentication:

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

## List Collections

Returns a paginated list of collections with support for filtering and sorting.

```c
// Basic list
char* collections_json = NULL;
if (bosbase_send(pb, "/api/collections", "GET", NULL, 
        "{\"page\":1,\"perPage\":30}", "{}", 30000, NULL, 0, 
        &collections_json, NULL) == 0) {
    printf("Collections: %s\n", collections_json);
    bosbase_free_string(collections_json);
}
```

### Advanced Filtering and Sorting

```c
// Filter by type
const char* query = "{\"filter\":\"type = \\\"auth\\\"\",\"page\":1,\"perPage\":100}";
char* auth_collections = NULL;
bosbase_send(pb, "/api/collections", "GET", NULL, query, "{}", 
    30000, NULL, 0, &auth_collections, NULL);
bosbase_free_string(auth_collections);

// Filter by name pattern
const char* query2 = "{\"filter\":\"name ~ \\\"user\\\"\",\"page\":1,\"perPage\":100}";
char* matching = NULL;
bosbase_send(pb, "/api/collections", "GET", NULL, query2, "{}", 
    30000, NULL, 0, &matching, NULL);
bosbase_free_string(matching);

// Sort by creation date
const char* query3 = "{\"sort\":\"-created\",\"page\":1,\"perPage\":100}";
char* sorted = NULL;
bosbase_send(pb, "/api/collections", "GET", NULL, query3, "{}", 
    30000, NULL, 0, &sorted, NULL);
bosbase_free_string(sorted);
```

## View Collection

Retrieve a single collection by ID or name:

```c
// By name
char* collection_json = NULL;
if (bosbase_send(pb, "/api/collections/posts", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &collection_json, NULL) == 0) {
    printf("Collection: %s\n", collection_json);
    bosbase_free_string(collection_json);
}

// By ID
char* collection_by_id = NULL;
bosbase_send(pb, "/api/collections/_pbc_2287844090", "GET", NULL, 
    "{}", "{}", 30000, NULL, 0, &collection_by_id, NULL);
bosbase_free_string(collection_by_id);
```

## Create Collection

Create a new collection with schema fields and configuration.

### Create Base Collection

```c
const char* base_collection = 
    "{"
    "\"name\":\"posts\","
    "\"type\":\"base\","
    "\"fields\":["
    "{\"name\":\"title\",\"type\":\"text\",\"required\":true,\"min\":10,\"max\":255},"
    "{\"name\":\"content\",\"type\":\"editor\",\"required\":false},"
    "{\"name\":\"published\",\"type\":\"bool\",\"required\":false},"
    "{\"name\":\"author\",\"type\":\"relation\",\"required\":true,"
    "\"options\":{\"collectionId\":\"users\"},\"maxSelect\":1}"
    "],"
    "\"listRule\":\"@request.auth.id != \\\"\\\"\","
    "\"viewRule\":\"@request.auth.id != \\\"\\\" || published = true\""
    "}";

char* created_json = NULL;
if (bosbase_send(pb, "/api/collections", "POST", base_collection, 
        "{}", "{}", 30000, NULL, 0, &created_json, NULL) == 0) {
    printf("Base collection created: %s\n", created_json);
    bosbase_free_string(created_json);
}
```

### Create Auth Collection

```c
const char* auth_collection = 
    "{"
    "\"name\":\"users\","
    "\"type\":\"auth\","
    "\"fields\":["
    "{\"name\":\"name\",\"type\":\"text\",\"required\":true}"
    "],"
    "\"listRule\":\"@request.auth.id != \\\"\\\"\","
    "\"viewRule\":\"@request.auth.id = id || @request.auth.id != \\\"\\\"\""
    "}";

char* auth_created = NULL;
bosbase_send(pb, "/api/collections", "POST", auth_collection, 
    "{}", "{}", 30000, NULL, 0, &auth_created, NULL);
bosbase_free_string(auth_created);
```

### Create View Collection

```c
const char* view_collection = 
    "{"
    "\"name\":\"post_stats\","
    "\"type\":\"view\","
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

## Update Collection

Update collection schema, rules, or other properties:

```c
// Update collection rules
const char* update_body = 
    "{\"listRule\":\"@request.auth.id != \\\"\\\" && status = \\\"published\\\"\"}";

char* updated_json = NULL;
if (bosbase_send(pb, "/api/collections/posts", "PATCH", update_body, 
        "{}", "{}", 30000, NULL, 0, &updated_json, NULL) == 0) {
    printf("Collection updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}

// Update collection name
const char* rename_body = "{\"name\":\"articles\"}";
char* renamed_json = NULL;
bosbase_send(pb, "/api/collections/posts", "PATCH", rename_body, 
    "{}", "{}", 30000, NULL, 0, &renamed_json, NULL);
bosbase_free_string(renamed_json);
```

## Delete Collection

Delete a collection and all its records:

```c
bosbase_error* err = NULL;
if (bosbase_send(pb, "/api/collections/posts", "DELETE", NULL, 
        "{}", "{}", 30000, NULL, 0, NULL, &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Truncate Collection

Delete all records in a collection without deleting the collection itself:

```c
bosbase_error* err = NULL;
if (bosbase_send(pb, "/api/collections/posts/truncate", "DELETE", NULL, 
        "{}", "{}", 30000, NULL, 0, NULL, &err) != 0) {
    fprintf(stderr, "Truncate failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Get Collection Scaffolds

Get predefined collection templates:

```c
char* scaffolds_json = NULL;
if (bosbase_send(pb, "/api/collections/meta/scaffolds", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &scaffolds_json, NULL) == 0) {
    // Parse scaffolds_json to see available templates
    printf("Available scaffolds: %s\n", scaffolds_json);
    bosbase_free_string(scaffolds_json);
}
```

## Import Collections

Import multiple collections at once:

```c
const char* import_body = 
    "{"
    "\"collections\":["
    "{"
    "\"name\":\"posts\","
    "\"type\":\"base\","
    "\"fields\":[{\"name\":\"title\",\"type\":\"text\"}]"
    "},"
    "{"
    "\"name\":\"users\","
    "\"type\":\"auth\","
    "\"fields\":[{\"name\":\"email\",\"type\":\"email\"}]"
    "}"
    "]"
    "}";

char* import_result = NULL;
if (bosbase_send(pb, "/api/collections/import", "PUT", import_body, 
        "{}", "{}", 30000, NULL, 0, &import_result, NULL) == 0) {
    printf("Collections imported: %s\n", import_result);
    bosbase_free_string(import_result);
}
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate as superuser
    char* auth_json = NULL;
    if (bosbase_collection_auth_with_password(pb, "_superusers", 
            "admin@example.com", "password", "{}", NULL, NULL, 
            "{}", "{}", &auth_json, NULL) != 0) {
        fprintf(stderr, "Authentication failed\n");
        bosbase_client_free(pb);
        return 1;
    }
    bosbase_free_string(auth_json);
    
    // List collections
    char* collections = NULL;
    bosbase_send(pb, "/api/collections", "GET", NULL, 
        "{\"page\":1,\"perPage\":50}", "{}", 30000, NULL, 0, 
        &collections, NULL);
    printf("Collections: %s\n", collections);
    bosbase_free_string(collections);
    
    // Create a new collection
    const char* new_collection = 
        "{\"name\":\"articles\",\"type\":\"base\","
        "\"fields\":[{\"name\":\"title\",\"type\":\"text\",\"required\":true}]}";
    
    char* created = NULL;
    if (bosbase_send(pb, "/api/collections", "POST", new_collection, 
            "{}", "{}", 30000, NULL, 0, &created, NULL) == 0) {
        printf("Created: %s\n", created);
        bosbase_free_string(created);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection concepts and field types
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Setting access rules

