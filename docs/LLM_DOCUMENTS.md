# LLM Document API - C SDK Documentation

## Overview

The LLM Document API wraps the `/api/llm-documents` endpoints that are backed by the embedded chromem-go vector store (persisted in rqlite). Each document contains text content, optional metadata and an embedding vector that can be queried with semantic search.

## Getting Started

```c
#include "bosbase_c.h"

bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");

// Authenticate as superuser
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Create a logical namespace for your documents
const char* create_body = "{\"domain\":\"internal\"}";
char* created_json = NULL;
if (bosbase_send(pb, "/api/llm-documents/knowledge-base", "PUT", 
        create_body, "{}", "{}", 30000, NULL, 0, 
        &created_json, NULL) == 0) {
    printf("Collection created: %s\n", created_json);
    bosbase_free_string(created_json);
}
```

## Insert Documents

```c
const char* doc1 = 
    "{"
    "\"content\":\"Leaves are green because chlorophyll absorbs red and blue light.\","
    "\"metadata\":{\"topic\":\"biology\"}"
    "}";

char* inserted1 = NULL;
if (bosbase_send(pb, "/api/llm-documents/knowledge-base", "POST", 
        doc1, "{}", "{}", 30000, NULL, 0, &inserted1, NULL) == 0) {
    printf("Document inserted: %s\n", inserted1);
    bosbase_free_string(inserted1);
}

// Insert with custom ID
const char* doc2 = 
    "{"
    "\"id\":\"sky\","
    "\"content\":\"The sky is blue because of Rayleigh scattering.\","
    "\"metadata\":{\"topic\":\"physics\"}"
    "}";

char* inserted2 = NULL;
bosbase_send(pb, "/api/llm-documents/knowledge-base", "POST", 
    doc2, "{}", "{}", 30000, NULL, 0, &inserted2, NULL);
bosbase_free_string(inserted2);
```

## Query Documents

```c
const char* query_body = 
    "{"
    "\"queryText\":\"Why is the sky blue?\","
    "\"limit\":3,"
    "\"where\":{\"topic\":\"physics\"}"
    "}";

char* query_result = NULL;
if (bosbase_send(pb, "/api/llm-documents/knowledge-base/query", "POST", 
        query_body, "{}", "{}", 30000, NULL, 0, 
        &query_result, NULL) == 0) {
    // Parse query_result to get:
    // - results: array of matches with id and similarity scores
    printf("Query results: %s\n", query_result);
    bosbase_free_string(query_result);
}
```

## Manage Documents

### Update Document

```c
const char* update_body = 
    "{\"metadata\":{\"topic\":\"physics\",\"reviewed\":\"true\"}}";

char* updated_json = NULL;
if (bosbase_send(pb, "/api/llm-documents/knowledge-base/sky", "PATCH", 
        update_body, "{}", "{}", 30000, NULL, 0, 
        &updated_json, NULL) == 0) {
    printf("Updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

### List Documents

```c
// List documents with pagination
const char* query = "{\"page\":1,\"perPage\":25}";
char* list_json = NULL;
if (bosbase_send(pb, "/api/llm-documents/knowledge-base", "GET", 
        NULL, query, "{}", 30000, NULL, 0, &list_json, NULL) == 0) {
    printf("Documents: %s\n", list_json);
    bosbase_free_string(list_json);
}
```

### Delete Document

```c
bosbase_error* err = NULL;
if (bosbase_send(pb, "/api/llm-documents/knowledge-base/sky", "DELETE", 
        NULL, "{}", "{}", 30000, NULL, 0, NULL, &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
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
    
    // Insert document
    const char* doc = 
        "{\"content\":\"The sky is blue because of Rayleigh scattering.\","
        "\"metadata\":{\"topic\":\"physics\"}}";
    char* inserted = NULL;
    bosbase_send(pb, "/api/llm-documents/knowledge-base", "POST", 
        doc, "{}", "{}", 30000, NULL, 0, &inserted, NULL);
    bosbase_free_string(inserted);
    
    // Query documents
    const char* query = 
        "{\"queryText\":\"Why is the sky blue?\",\"limit\":3}";
    char* results = NULL;
    bosbase_send(pb, "/api/llm-documents/knowledge-base/query", "POST", 
        query, "{}", "{}", 30000, NULL, 0, &results, NULL);
    printf("Results: %s\n", results);
    bosbase_free_string(results);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Vector API](./VECTOR_API.md) - Vector search operations
- [LangChaingo API](./LANGCHAINGO_API.md) - RAG workflows

