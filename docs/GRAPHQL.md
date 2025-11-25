# GraphQL API - C SDK Documentation

## Overview

Use `bosbase_graphql_query()` to call `/api/graphql` with your current auth token. It returns `{ data, errors, extensions }`.

> **Authentication**: the GraphQL endpoint is **superuser-only**. Authenticate as a superuser before calling GraphQL.

## Authentication

```c
#include "bosbase_c.h"

bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");

// Authenticate as superuser
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);
```

## Single-table Query

```c
const char* query = 
    "query ActiveUsers($limit: Int!) {"
    "  records(collection: \"users\", perPage: $limit, filter: \"status = true\") {"
    "    items { id data }"
    "  }"
    "}";

const char* variables = "{\"limit\":5}";

char* result_json = NULL;
if (bosbase_graphql_query(pb, query, variables, "{}", "{}", 
        &result_json, NULL) == 0) {
    // Parse result_json to get:
    // - data: query results
    // - errors: array of errors (if any)
    // - extensions: additional metadata
    printf("Result: %s\n", result_json);
    bosbase_free_string(result_json);
}
```

## Multi-table Join via Expands

```c
const char* query = 
    "query PostsWithAuthors {"
    "  records("
    "    collection: \"posts\","
    "    expand: [\"author\", \"author.profile\"],"
    "    sort: \"-created\""
    "  ) {"
    "    items {"
    "      id"
    "      data  # expanded relations live under data.expand"
    "    }"
    "  }"
    "}";

char* result_json = NULL;
bosbase_graphql_query(pb, query, "{}", "{}", "{}", 
    &result_json, NULL);
bosbase_free_string(result_json);
```

## Conditional Query with Variables

```c
const char* query = 
    "query FilteredOrders($minTotal: Float!, $state: String!) {"
    "  records("
    "    collection: \"orders\","
    "    filter: \"total >= $minTotal && status = $state\","
    "    sort: \"created\""
    "  ) {"
    "    items { id data }"
    "  }"
    "}";

const char* variables = "{\"minTotal\":100,\"state\":\"paid\"}";

char* result_json = NULL;
bosbase_graphql_query(pb, query, variables, "{}", "{}", 
    &result_json, NULL);
bosbase_free_string(result_json);
```

## Create a Record

```c
const char* mutation = 
    "mutation CreatePost($data: JSON!) {"
    "  createRecord(collection: \"posts\", data: $data, expand: [\"author\"]) {"
    "    id"
    "    data"
    "  }"
    "}";

const char* variables = 
    "{\"data\":{\"title\":\"Hello\",\"author\":\"USER_ID\"}}";

char* result_json = NULL;
bosbase_graphql_query(pb, mutation, variables, "{}", "{}", 
    &result_json, NULL);
bosbase_free_string(result_json);
```

## Update a Record

```c
const char* mutation = 
    "mutation UpdatePost($id: ID!, $data: JSON!) {"
    "  updateRecord(collection: \"posts\", id: $id, data: $data) {"
    "    id"
    "    data"
    "  }"
    "}";

const char* variables = 
    "{\"id\":\"POST_ID\",\"data\":{\"title\":\"Updated title\"}}";

char* result_json = NULL;
bosbase_graphql_query(pb, mutation, variables, "{}", "{}", 
    &result_json, NULL);
bosbase_free_string(result_json);
```

## Delete a Record

```c
const char* mutation = 
    "mutation DeletePost($id: ID!) {"
    "  deleteRecord(collection: \"posts\", id: $id)"
    "}";

const char* variables = "{\"id\":\"POST_ID\"}";

char* result_json = NULL;
bosbase_graphql_query(pb, mutation, variables, "{}", "{}", 
    &result_json, NULL);
bosbase_free_string(result_json);
```

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
    
    // GraphQL query
    const char* query = 
        "query {"
        "  records(collection: \"posts\", perPage: 10) {"
        "    items { id data }"
        "  }"
        "}";
    
    char* result = NULL;
    if (bosbase_graphql_query(pb, query, "{}", "{}", "{}", 
            &result, NULL) == 0) {
        printf("GraphQL result: %s\n", result);
        bosbase_free_string(result);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [API Records](./API_RECORDS.md) - REST API equivalent
- [Collections](./COLLECTIONS.md) - Collection management

