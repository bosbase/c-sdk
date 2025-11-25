# Vector Database API - C SDK Documentation

## Overview

Vector database operations for semantic search, RAG (Retrieval-Augmented Generation), and AI applications.

> **Note**: Vector operations are currently implemented using sqlite-vec but are designed with abstraction in mind to support future vector database providers.

The Vector API provides a unified interface for working with vector embeddings, enabling you to:
- Store and search vector embeddings
- Perform similarity search
- Build RAG applications
- Create recommendation systems
- Enable semantic search capabilities

## Getting Started

```c
#include "bosbase_c.h"

bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");

// Authenticate as superuser (vectors require superuser auth)
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);
```

## Collection Management

### Create Collection

Create a new vector collection with specified dimension and distance metric.

```c
const char* config = 
    "{"
    "\"dimension\":384,"
    "\"distance\":\"cosine\""
    "}";

char* created_json = NULL;
if (bosbase_vector_create_collection(pb, "documents", config, 
        "{}", "{}", &created_json, NULL) == 0) {
    printf("Collection created: %s\n", created_json);
    bosbase_free_string(created_json);
}

// Minimal example (uses defaults)
char* minimal_json = NULL;
bosbase_vector_create_collection(pb, "documents", "{}", 
    "{}", "{}", &minimal_json, NULL);
bosbase_free_string(minimal_json);
```

**Parameters:**
- `name` (string): Collection name
- `config` (JSON object, optional):
  - `dimension` (number, optional): Vector dimension. Default: 384
  - `distance` (string, optional): Distance metric. Default: 'cosine'
  - Options: 'cosine', 'l2', 'dot'

### List Collections

Get all available vector collections.

```c
char* collections_json = NULL;
if (bosbase_vector_list_collections(pb, "{}", "{}", 
        &collections_json, NULL) == 0) {
    printf("Collections: %s\n", collections_json);
    bosbase_free_string(collections_json);
}
```

### Delete Collection

```c
bosbase_error* err = NULL;
if (bosbase_vector_delete_collection(pb, "documents", 
        "{}", "{}", &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Document Operations

### Insert Document

Insert a vector document with embedding and optional metadata.

```c
const char* document = 
    "{"
    "\"collection\":\"documents\","
    "\"vector\":[0.1,0.2,0.3,0.4],"
    "\"metadata\":{\"title\":\"Example\",\"category\":\"tech\"},"
    "\"content\":\"Example content text\""
    "}";

char* inserted_json = NULL;
if (bosbase_vector_insert(pb, document, "{}", "{}", 
        &inserted_json, NULL) == 0) {
    printf("Document inserted: %s\n", inserted_json);
    bosbase_free_string(inserted_json);
}
```

### Batch Insert

Insert multiple documents at once.

```c
const char* batch_options = 
    "{"
    "\"collection\":\"documents\","
    "\"documents\":["
    "{\"vector\":[0.1,0.2,0.3],\"content\":\"Doc 1\"},"
    "{\"vector\":[0.4,0.5,0.6],\"content\":\"Doc 2\"}"
    "]"
    "}";

char* batch_result = NULL;
if (bosbase_vector_batch_insert(pb, batch_options, "{}", "{}", 
        &batch_result, NULL) == 0) {
    printf("Batch inserted: %s\n", batch_result);
    bosbase_free_string(batch_result);
}
```

### Get Document

Retrieve a document by ID.

```c
char* doc_json = NULL;
if (bosbase_vector_get(pb, "DOCUMENT_ID", "{}", "{}", 
        &doc_json, NULL) == 0) {
    printf("Document: %s\n", doc_json);
    bosbase_free_string(doc_json);
}
```

### Update Document

```c
const char* update_doc = 
    "{"
    "\"vector\":[0.7,0.8,0.9],"
    "\"content\":\"Updated content\""
    "}";

char* updated_json = NULL;
if (bosbase_vector_update(pb, "DOCUMENT_ID", update_doc, 
        "{}", "{}", &updated_json, NULL) == 0) {
    printf("Updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

### Delete Document

```c
bosbase_error* err = NULL;
if (bosbase_vector_delete(pb, "DOCUMENT_ID", "{}", "{}", &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Search Operations

### Vector Search

Perform similarity search.

```c
const char* search_options = 
    "{"
    "\"collection\":\"documents\","
    "\"queryVector\":[0.1,0.2,0.3,0.4],"
    "\"limit\":10,"
    "\"minScore\":0.5,"
    "\"includeDistance\":true,"
    "\"includeContent\":true"
    "}";

char* results_json = NULL;
if (bosbase_vector_search(pb, search_options, "{}", "{}", 
        &results_json, NULL) == 0) {
    // Parse results_json to get array of search results:
    // Each result contains:
    // - document: VectorDocument
    // - score: number (similarity score 0-1)
    // - distance: number (optional)
    printf("Search results: %s\n", results_json);
    bosbase_free_string(results_json);
}
```

### List Documents

List all documents in a collection.

```c
const char* query = "{\"collection\":\"documents\",\"limit\":100}";
char* list_json = NULL;
if (bosbase_vector_list(pb, query, "{}", &list_json, NULL) == 0) {
    printf("Documents: %s\n", list_json);
    bosbase_free_string(list_json);
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
    
    // Create collection
    char* created = NULL;
    bosbase_vector_create_collection(pb, "documents", "{}", 
        "{}", "{}", &created, NULL);
    bosbase_free_string(created);
    
    // Insert document
    const char* doc = 
        "{\"collection\":\"documents\","
        "\"vector\":[0.1,0.2,0.3],"
        "\"content\":\"Example\"}";
    char* inserted = NULL;
    bosbase_vector_insert(pb, doc, "{}", "{}", &inserted, NULL);
    bosbase_free_string(inserted);
    
    // Search
    const char* search = 
        "{\"collection\":\"documents\","
        "\"queryVector\":[0.1,0.2,0.3],"
        "\"limit\":10}";
    char* results = NULL;
    bosbase_vector_search(pb, search, "{}", "{}", &results, NULL);
    printf("Results: %s\n", results);
    bosbase_free_string(results);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [LLM Documents](./LLM_DOCUMENTS.md) - LLM document management
- [LangChaingo API](./LANGCHAINGO_API.md) - RAG workflows

