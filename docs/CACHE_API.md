# Cache API - C SDK Documentation

## Overview

BosBase caches combine in-memory FreeCache storage with persistent database copies. Each cache instance is safe to use in single-node or multi-node (cluster) mode: nodes read from FreeCache first, fall back to the database if an item is missing or expired, and then reload FreeCache automatically.

The C SDK exposes the cache endpoints through `bosbase_send`. Typical use cases include:

- Caching AI prompts/responses that must survive restarts
- Quickly sharing feature flags and configuration between workers
- Preloading expensive vector search results for short periods

> **Timeouts & TTLs:** Each cache defines a default TTL (in seconds). Individual entries may provide their own `ttlSeconds`. A value of `0` keeps the entry until it is manually deleted.

## List Available Caches

Query and retrieve all currently available caches, including their names and capacities:

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate as superuser
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "_superusers", 
        "root@example.com", "hunter2", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // Query all available caches
    char* caches_json = NULL;
    if (bosbase_cache_list(pb, "{}", "{}", &caches_json, NULL) == 0) {
        // Parse caches_json to get array of cache objects:
        // Each cache object contains:
        // - name: string - The cache identifier
        // - sizeBytes: number - The cache capacity in bytes
        // - defaultTTLSeconds: number - Default expiration time
        // - readTimeoutMs: number - Read timeout in milliseconds
        // - created: string - Creation timestamp (RFC3339)
        // - updated: string - Last update timestamp (RFC3339)
        printf("Available caches: %s\n", caches_json);
        bosbase_free_string(caches_json);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

## Manage Cache Configurations

### Create Cache

```c
const char* cache_body = 
    "{"
    "\"name\":\"ai-session\","
    "\"sizeBytes\":67108864,"
    "\"defaultTTLSeconds\":300,"
    "\"readTimeoutMs\":25"
    "}";

char* created_json = NULL;
if (bosbase_cache_create(pb, "ai-session", cache_body, 
        "{}", "{}", &created_json, NULL) == 0) {
    printf("Cache created: %s\n", created_json);
    bosbase_free_string(created_json);
}
```

### Update Cache

```c
// Update limits later (e.g., shrink TTL to 2 minutes)
const char* update_body = "{\"defaultTTLSeconds\":120}";
char* updated_json = NULL;
if (bosbase_send(pb, "/api/caches/ai-session", "PATCH", update_body, 
        "{}", "{}", 30000, NULL, 0, &updated_json, NULL) == 0) {
    printf("Cache updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

### Delete Cache

```c
bosbase_error* err = NULL;
if (bosbase_send(pb, "/api/caches/ai-session", "DELETE", NULL, 
        "{}", "{}", 30000, NULL, 0, NULL, &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Work with Cache Entries

### Set Entry

Store an object in cache. The same payload is serialized into the DB.

```c
const char* entry_body = 
    "{"
    "\"value\":{"
    "\"prompt\":\"describe Saturn\","
    "\"embedding\":[0.1,0.2,0.3]"
    "},"
    "\"ttlSeconds\":90"
    "}";

char* set_result = NULL;
if (bosbase_cache_set_entry(pb, "ai-session", "dialog:42", 
        entry_body, 90, "{}", "{}", &set_result, NULL) == 0) {
    printf("Entry set: %s\n", set_result);
    bosbase_free_string(set_result);
}
```

### Get Entry

Read from cache. The response indicates where the hit came from.

```c
char* entry_json = NULL;
if (bosbase_cache_get_entry(pb, "ai-session", "dialog:42", 
        "{}", "{}", &entry_json, NULL) == 0) {
    // Parse entry_json to get:
    // - value: the cached object
    // - source: "cache" or "database"
    // - expiresAt: RFC3339 timestamp or undefined
    printf("Entry: %s\n", entry_json);
    bosbase_free_string(entry_json);
}
```

### Delete Entry

```c
bosbase_error* err = NULL;
if (bosbase_cache_delete_entry(pb, "ai-session", "dialog:42", 
        "{}", "{}", &err) != 0) {
    fprintf(stderr, "Delete entry failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Field Reference

| Field | Description |
|-------|-------------|
| `sizeBytes` | Approximate FreeCache size. Values too small (<512KB) or too large (>512MB) are clamped. |
| `defaultTTLSeconds` | Default expiration for entries. `0` means no expiration. |
| `readTimeoutMs` | Optional lock timeout while reading FreeCache. When exceeded, the value is fetched from the database instead. |

## Cluster-Aware Behavior

1. **Write-through persistence** – every `setEntry` writes to FreeCache and the `_cache_entries` table so other nodes (or a restarted node) can immediately reload values.
2. **Read path** – FreeCache is consulted first. If a lock cannot be acquired within `readTimeoutMs` or if the entry is missing/expired, BosBase queries the database copy and repopulates FreeCache in the background.
3. **Automatic cleanup** – expired entries are ignored and removed from the database when fetched, preventing stale data across nodes.

Use caches whenever you need fast, transient data that must still be recoverable or shareable across BosBase nodes.

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
    
    // List caches
    char* caches = NULL;
    bosbase_cache_list(pb, "{}", "{}", &caches, NULL);
    printf("Caches: %s\n", caches);
    bosbase_free_string(caches);
    
    // Set cache entry
    const char* value = "{\"data\":\"cached value\"}";
    char* set_result = NULL;
    bosbase_cache_set_entry(pb, "my-cache", "key1", value, 300, 
        "{}", "{}", &set_result, NULL);
    bosbase_free_string(set_result);
    
    // Get cache entry
    char* get_result = NULL;
    bosbase_cache_get_entry(pb, "my-cache", "key1", 
        "{}", "{}", &get_result, NULL);
    printf("Retrieved: %s\n", get_result);
    bosbase_free_string(get_result);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection management
- [Health API](./HEALTH_API.md) - Server health checks

