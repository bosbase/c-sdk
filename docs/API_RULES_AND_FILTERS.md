# API Rules and Filters - C SDK Documentation

## Overview

API Rules are your collection access controls and data filters. They control who can perform actions on your collections and what data they can access.

Each collection has 5 rules, corresponding to specific API actions:
- `listRule` - Controls who can list records
- `viewRule` - Controls who can view individual records
- `createRule` - Controls who can create records
- `updateRule` - Controls who can update records
- `deleteRule` - Controls who can delete records

Auth collections have an additional `manageRule` that allows one user to fully manage another user's data.

## Rule Values

Each rule can be set to:

- **`null` (locked)** - Only authorized superusers can perform the action (default)
- **Empty string `""`** - Anyone can perform the action (superusers, authenticated users, and guests)
- **Non-empty string** - Only users that satisfy the filter expression can perform the action

## Important Notes

1. **Rules act as filters**: API Rules also act as record filters. For example, setting `listRule` to `status = "active"` will only return active records.
2. **HTTP Status Codes**: 
   - `200` with empty items for unsatisfied `listRule`
   - `400` for unsatisfied `createRule`
   - `404` for unsatisfied `viewRule`, `updateRule`, `deleteRule`
   - `403` for locked rules when not a superuser
3. **Superuser bypass**: API Rules are ignored when the action is performed by an authorized superuser.

## Setting Rules via SDK

### Create Collection with Rules

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Authenticate as superuser
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "_superusers", 
        "admin@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // Create collection with rules
    const char* collection_body = 
        "{"
        "\"type\":\"base\","
        "\"name\":\"articles\","
        "\"fields\":["
        "{\"name\":\"title\",\"type\":\"text\",\"required\":true},"
        "{\"name\":\"status\",\"type\":\"select\","
        "\"options\":{\"values\":[\"draft\",\"published\"]},\"maxSelect\":1},"
        "{\"name\":\"author\",\"type\":\"relation\","
        "\"options\":{\"collectionId\":\"users\"},\"maxSelect\":1}"
        "],"
        "\"listRule\":\"@request.auth.id != \\\"\\\" || status = \\\"published\\\"\","
        "\"viewRule\":\"@request.auth.id != \\\"\\\" || status = \\\"published\\\"\","
        "\"createRule\":\"@request.auth.id != \\\"\\\"\","
        "\"updateRule\":\"author = @request.auth.id || @request.auth.role = \\\"admin\\\"\","
        "\"deleteRule\":\"author = @request.auth.id || @request.auth.role = \\\"admin\\\"\""
        "}";
    
    char* result_json = NULL;
    if (bosbase_send(pb, "/api/collections", "POST", collection_body, 
            "{}", "{}", 30000, NULL, 0, &result_json, NULL) == 0) {
        printf("Collection created: %s\n", result_json);
        bosbase_free_string(result_json);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

### Update Rules

```c
// Update collection rules
const char* update_body = 
    "{\"listRule\":\"@request.auth.id != \\\"\\\" && "
    "(status = \\\"published\\\" || status = \\\"draft\\\")\"}";

char* updated_json = NULL;
if (bosbase_send(pb, "/api/collections/articles", "PATCH", update_body, 
        "{}", "{}", 30000, NULL, 0, &updated_json, NULL) == 0) {
    printf("Rules updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

### Remove Rule (Public Access)

```c
// Remove rule (set to empty string for public access)
const char* public_body = "{\"listRule\":\"\"}";
char* public_json = NULL;
bosbase_send(pb, "/api/collections/articles", "PATCH", public_body, 
    "{}", "{}", 30000, NULL, 0, &public_json, NULL);
bosbase_free_string(public_json);
```

### Lock Rule (Superuser Only)

```c
// Lock rule (set to null for superuser only)
const char* lock_body = "{\"deleteRule\":null}";
char* lock_json = NULL;
bosbase_send(pb, "/api/collections/articles", "PATCH", lock_body, 
    "{}", "{}", 30000, NULL, 0, &lock_json, NULL);
bosbase_free_string(lock_json);
```

## Filter Syntax

The syntax follows: `OPERAND OPERATOR OPERAND`

### Operators

**Comparison Operators:**
- `=` - Equal
- `!=` - NOT equal
- `>` - Greater than
- `>=` - Greater than or equal
- `<` - Less than
- `<=` - Less than or equal

**String Operators:**
- `~` - Like/Contains (auto-wraps right operand in `%` for wildcard match)
- `!~` - NOT Like/Contains

**Array Operators (Any/At least one of):**
- `?=` - Any Equal
- `?!=` - Any NOT equal
- `?>` - Any Greater than
- `?>=` - Any Greater than or equal
- `?<` - Any Less than
- `?<=` - Any Less than or equal
- `?~` - Any Like/Contains
- `?!~` - Any NOT Like/Contains

**Logical Operators:**
- `&&` - AND
- `||` - OR
- `()` - Grouping
- `//` - Single line comments

## Special Identifiers

### @request.*

Access current request data:

**@request.context** - The context where the rule is used
```c
const char* rule = "@request.context != \"oauth2\"";
```

**@request.method** - HTTP request method
```c
const char* rule = "@request.method = \"PATCH\"";
```

**@request.headers.*** - Request headers (normalized to lowercase, `-` replaced with `_`)
```c
const char* rule = "@request.headers.x_token = \"test\"";
```

**@request.query.*** - Query parameters
```c
const char* rule = "@request.query.page = \"1\"";
```

**@request.auth.*** - Current authenticated user
```c
const char* list_rule = "@request.auth.id != \"\"";
const char* view_rule = "@request.auth.email = \"admin@example.com\"";
const char* update_rule = "@request.auth.role = \"admin\"";
```

**@request.body.*** - Submitted body parameters
```c
const char* create_rule = "@request.body.title != \"\"";
const char* update_rule = "@request.body.status:isset = false";  // Prevent changing status
```

### @collection.*

Target other collections that aren't directly related:

```c
// Check if user has access to related collection
const char* rule = 
    "@request.auth.id != \"\" && "
    "@collection.news.categoryId ?= categoryId && "
    "@collection.news.author ?= @request.auth.id";
```

### @ Macros (Datetime)

All macros are UTC-based:

- `@now` - Current datetime as string
- `@second` - Current second (0-59)
- `@minute` - Current minute (0-59)
- `@hour` - Current hour (0-23)
- `@weekday` - Current weekday (0-6)
- `@day` - Current day
- `@month` - Current month
- `@year` - Current year
- `@yesterday` - Yesterday datetime
- `@tomorrow` - Tomorrow datetime
- `@todayStart` - Beginning of current day
- `@todayEnd` - End of current day
- `@monthStart` - Beginning of current month
- `@monthEnd` - End of current month
- `@yearStart` - Beginning of current year
- `@yearEnd` - End of current year

**Example:**
```c
const char* rule = "@request.body.publicDate >= @now";
const char* rule2 = "created >= @todayStart && created <= @todayEnd";
```

## Field Modifiers

### :isset

Check if a field was submitted in the request (only for `@request.*` fields):

```c
// Prevent changing role field
const char* rule = "@request.body.role:isset = false";
```

## Using Filters in Queries

Filters can also be used in client-side queries (not just in API rules):

```c
// Filter records
const char* filter = "status = \"published\" && views > 100";
char* records_json = NULL;
if (bosbase_collection_get_list(pb, "posts", 1, 50, filter, 
        NULL, NULL, NULL, "{}", "{}", &records_json, NULL) == 0) {
    printf("Filtered records: %s\n", records_json);
    bosbase_free_string(records_json);
}

// Complex filter
const char* complex_filter = 
    "(status = \"published\" || featured = true) && "
    "created >= \"2023-01-01\" && "
    "views > 100";
```

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
    
    // Create collection with comprehensive rules
    const char* collection = 
        "{"
        "\"type\":\"base\","
        "\"name\":\"articles\","
        "\"fields\":["
        "{\"name\":\"title\",\"type\":\"text\",\"required\":true},"
        "{\"name\":\"status\",\"type\":\"select\","
        "\"options\":{\"values\":[\"draft\",\"published\"]},\"maxSelect\":1},"
        "{\"name\":\"author\",\"type\":\"relation\","
        "\"options\":{\"collectionId\":\"users\"},\"maxSelect\":1}"
        "],"
        "\"listRule\":\"@request.auth.id != \\\"\\\" || status = \\\"published\\\"\","
        "\"viewRule\":\"@request.auth.id != \\\"\\\" || status = \\\"published\\\"\","
        "\"createRule\":\"@request.auth.id != \\\"\\\"\","
        "\"updateRule\":\"author = @request.auth.id || @request.auth.role = \\\"admin\\\"\","
        "\"deleteRule\":\"author = @request.auth.id || @request.auth.role = \\\"admin\\\"\""
        "}";
    
    char* result_json = NULL;
    if (bosbase_send(pb, "/api/collections", "POST", collection, 
            "{}", "{}", 30000, NULL, 0, &result_json, NULL) == 0) {
        printf("Collection with rules created\n");
        bosbase_free_string(result_json);
        return 0;
    }
    return 1;
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    if (setup_collection_rules(pb) == 0) {
        printf("Collection rules configured successfully\n");
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

## Best Practices

1. **Start Restrictive**: Begin with locked rules and gradually open access
2. **Use Special Identifiers**: Leverage `@request.auth.*` for user-based access control
3. **Test Rules**: Test rules with different user roles and authentication states
4. **Document Rules**: Document complex rules for future maintenance
5. **Security**: Never expose sensitive data in rules; use proper authentication

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection configuration
- [API Records](./API_RECORDS.md) - Using filters in queries
- [Authentication](./AUTHENTICATION.md) - User authentication

