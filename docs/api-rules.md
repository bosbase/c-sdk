# API Rules Documentation - C SDK

API Rules are collection access controls and data filters that determine who can perform actions on your collections and what data they can access.

## Overview

Each collection has 5 standard API rules, corresponding to specific API actions:

- **`listRule`** - Controls read/list access
- **`viewRule`** - Controls read/view access  
- **`createRule`** - Controls create access
- **`updateRule`** - Controls update access
- **`deleteRule`** - Controls delete access

Auth collections have two additional rules:

- **`manageRule`** - Admin-like permissions for managing auth records
- **`authRule`** - Additional constraints applied during authentication

## Rule Values

Each rule can be set to one of three values:

### 1. `null` (Locked)
Only authorized superusers can perform the action.

```c
const char* lock_body = "{\"listRule\":null}";
char* locked_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", lock_body, 
    "{}", "{}", 30000, NULL, 0, &locked_json, NULL);
bosbase_free_string(locked_json);
```

### 2. `""` (Empty String - Public)
Anyone (superusers, authorized users, and guests) can perform the action.

```c
const char* public_body = "{\"listRule\":\"\"}";
char* public_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", public_body, 
    "{}", "{}", 30000, NULL, 0, &public_json, NULL);
bosbase_free_string(public_json);
```

### 3. Non-empty String (Filter Expression)
Only users satisfying the filter expression can perform the action.

```c
const char* rule_body = "{\"listRule\":\"@request.auth.id != \\\"\\\"\"}";
char* rule_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", rule_body, 
    "{}", "{}", 30000, NULL, 0, &rule_json, NULL);
bosbase_free_string(rule_json);
```

## Default Permissions

When you create a base collection without specifying rules, BosBase applies opinionated defaults:

- `listRule` and `viewRule` default to an empty string (`""`), so guests and authenticated users can query records.
- `createRule` defaults to `@request.auth.id != ""`, restricting writes to authenticated users or superusers.
- `updateRule` and `deleteRule` default to `@request.auth.id != "" && createdBy = @request.auth.id`, which limits mutations to the record creator (superusers still bypass rules).

## Setting Rules

### Individual Rules

Set individual rules using collection update:

```c
// Set list rule
const char* list_rule = "{\"listRule\":\"@request.auth.id != \\\"\\\"\"}";
char* list_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", list_rule, 
    "{}", "{}", 30000, NULL, 0, &list_json, NULL);
bosbase_free_string(list_json);

// Set view rule
const char* view_rule = "{\"viewRule\":\"@request.auth.id != \\\"\\\"\"}";
char* view_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", view_rule, 
    "{}", "{}", 30000, NULL, 0, &view_json, NULL);
bosbase_free_string(view_json);

// Set create rule
const char* create_rule = "{\"createRule\":\"@request.auth.id != \\\"\\\"\"}";
char* create_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", create_rule, 
    "{}", "{}", 30000, NULL, 0, &create_json, NULL);
bosbase_free_string(create_json);

// Set update rule
const char* update_rule = 
    "{\"updateRule\":\"@request.auth.id != \\\"\\\" && author.id ?= @request.auth.id\"}";
char* update_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", update_rule, 
    "{}", "{}", 30000, NULL, 0, &update_json, NULL);
bosbase_free_string(update_json);

// Set delete rule
const char* delete_rule = "{\"deleteRule\":null}";  // Only superusers
char* delete_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", delete_rule, 
    "{}", "{}", 30000, NULL, 0, &delete_json, NULL);
bosbase_free_string(delete_json);
```

### Bulk Rule Updates

Set multiple rules at once:

```c
const char* all_rules = 
    "{"
    "\"listRule\":\"@request.auth.id != \\\"\\\"\","
    "\"viewRule\":\"@request.auth.id != \\\"\\\"\","
    "\"createRule\":\"@request.auth.id != \\\"\\\"\","
    "\"updateRule\":\"author = @request.auth.id\","
    "\"deleteRule\":\"author = @request.auth.id\""
    "}";

char* rules_json = NULL;
bosbase_send(pb, "/api/collections/products", "PATCH", all_rules, 
    "{}", "{}", 30000, NULL, 0, &rules_json, NULL);
bosbase_free_string(rules_json);
```

## Filter Syntax

See [API Rules and Filters](./API_RULES_AND_FILTERS.md) for detailed filter syntax documentation.

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

int setup_collection_rules(bosbase_client* pb) {
    // Authenticate as superuser
    char* auth_json = NULL;
    if (bosbase_collection_auth_with_password(pb, "_superusers", 
            "admin@example.com", "password", "{}", NULL, NULL, 
            "{}", "{}", &auth_json, NULL) != 0) {
        return 1;
    }
    bosbase_free_string(auth_json);
    
    // Create collection with rules
    const char* collection = 
        "{"
        "\"type\":\"base\","
        "\"name\":\"products\","
        "\"fields\":["
        "{\"name\":\"name\",\"type\":\"text\",\"required\":true},"
        "{\"name\":\"author\",\"type\":\"relation\","
        "\"options\":{\"collectionId\":\"users\"},\"maxSelect\":1}"
        "],"
        "\"listRule\":\"@request.auth.id != \\\"\\\"\","
        "\"viewRule\":\"@request.auth.id != \\\"\\\" || status = \\\"published\\\"\","
        "\"createRule\":\"@request.auth.id != \\\"\\\"\","
        "\"updateRule\":\"author = @request.auth.id\","
        "\"deleteRule\":\"author = @request.auth.id\""
        "}";
    
    char* created = NULL;
    if (bosbase_send(pb, "/api/collections", "POST", collection, 
            "{}", "{}", 30000, NULL, 0, &created, NULL) == 0) {
        printf("Collection with rules created\n");
        bosbase_free_string(created);
        return 0;
    }
    return 1;
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    if (setup_collection_rules(pb) == 0) {
        printf("Rules configured successfully\n");
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Detailed filter syntax
- [Collections](./COLLECTIONS.md) - Collection management
- [Authentication](./AUTHENTICATION.md) - User authentication

