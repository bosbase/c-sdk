# Schema Query API - C SDK Documentation

## Overview

The Schema Query API provides lightweight interfaces to retrieve collection field information without fetching full collection definitions. This is particularly useful for AI systems that need to understand the structure of collections and the overall system architecture.

**Key Features:**
- Get schema for a single collection by name or ID
- Get schemas for all collections in the system
- Lightweight response with only essential field information
- Support for all collection types (base, auth, view)
- Fast and efficient queries

**Backend Endpoints:**
- `GET /api/collections/{collection}/schema` - Get single collection schema
- `GET /api/collections/schemas` - Get all collection schemas

**Note**: All Schema Query API operations require superuser authentication.

## Authentication

All Schema Query API operations require superuser authentication:

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

## Get Single Collection Schema

Retrieves the schema (fields and types) for a single collection by name or ID.

```c
// Get schema for a collection by name
char* schema_json = NULL;
if (bosbase_send(pb, "/api/collections/demo1/schema", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &schema_json, NULL) == 0) {
    // Parse schema_json to get:
    // - name: string - Collection name
    // - type: string - Collection type ("base", "auth", "view")
    // - fields: array - Array of field information
    //   Each field contains:
    //   - name: string
    //   - type: string
    //   - required: boolean (optional)
    //   - system: boolean (optional)
    //   - hidden: boolean (optional)
    printf("Schema: %s\n", schema_json);
    bosbase_free_string(schema_json);
}

// Get schema for a collection by ID
char* schema_by_id = NULL;
bosbase_send(pb, "/api/collections/_pbc_base_123/schema", "GET", NULL, 
    "{}", "{}", 30000, NULL, 0, &schema_by_id, NULL);
bosbase_free_string(schema_by_id);
```

## Get All Collection Schemas

Retrieves schemas for all collections in the system.

```c
char* all_schemas_json = NULL;
if (bosbase_send(pb, "/api/collections/schemas", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &all_schemas_json, NULL) == 0) {
    // Parse all_schemas_json to get:
    // - collections: array of CollectionSchemaInfo objects
    printf("All schemas: %s\n", all_schemas_json);
    bosbase_free_string(all_schemas_json);
}
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "_superusers", 
        "admin@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // Get single collection schema
    char* schema = NULL;
    bosbase_send(pb, "/api/collections/posts/schema", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &schema, NULL);
    printf("Schema: %s\n", schema);
    bosbase_free_string(schema);
    
    // Get all schemas
    char* all_schemas = NULL;
    bosbase_send(pb, "/api/collections/schemas", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &all_schemas, NULL);
    printf("All schemas: %s\n", all_schemas);
    bosbase_free_string(all_schemas);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection management
- [Collection API](./COLLECTION_API.md) - Full collection operations

