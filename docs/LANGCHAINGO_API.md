# LangChaingo API - C SDK Documentation

## Overview

BosBase exposes the `/api/langchaingo` endpoints so you can run LangChainGo powered workflows without leaving the platform. The C SDK accesses these endpoints via `bosbase_send`.

The service exposes four high-level methods:

| Method | HTTP Endpoint | Description |
| --- | --- | --- |
| Completions | `POST /api/langchaingo/completions` | Runs a chat/completion call using the configured LLM provider. |
| RAG | `POST /api/langchaingo/rag` | Runs a retrieval-augmented generation pass over an `llmDocuments` collection. |
| Query Documents | `POST /api/langchaingo/documents/query` | Asks an OpenAI-backed chain to answer questions over `llmDocuments` and optionally return matched sources. |
| SQL | `POST /api/langchaingo/sql` | Lets OpenAI draft and execute SQL against your BosBase database, then returns the results. |

## Authentication

All LangChaingo API operations require superuser authentication:

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

## Text + Chat Completions

```c
const char* completion_body = 
    "{"
    "\"model\":{"
    "\"provider\":\"openai\","
    "\"model\":\"gpt-4o-mini\""
    "},"
    "\"messages\":["
    "{\"role\":\"system\",\"content\":\"Answer in one sentence.\"},"
    "{\"role\":\"user\",\"content\":\"Explain Rayleigh scattering.\"}"
    "],"
    "\"temperature\":0.2"
    "}";

char* completion_json = NULL;
if (bosbase_send(pb, "/api/langchaingo/completions", "POST", 
        completion_body, "{}", "{}", 30000, NULL, 0, 
        &completion_json, NULL) == 0) {
    // Parse completion_json to get:
    // - content: string - The completion text
    // - functionCall: object (optional)
    // - toolCalls: array (optional)
    // - generationInfo: object (optional)
    printf("Completion: %s\n", completion_json);
    bosbase_free_string(completion_json);
}
```

## Retrieval-Augmented Generation (RAG)

Pair the LangChaingo endpoints with the `/api/llm-documents` store to build RAG workflows.

```c
const char* rag_body = 
    "{"
    "\"collection\":\"knowledge-base\","
    "\"question\":\"Why is the sky blue?\","
    "\"topK\":4,"
    "\"returnSources\":true,"
    "\"filters\":{"
    "\"where\":{\"topic\":\"physics\"}"
    "}"
    "}";

char* rag_json = NULL;
if (bosbase_send(pb, "/api/langchaingo/rag", "POST", 
        rag_body, "{}", "{}", 30000, NULL, 0, &rag_json, NULL) == 0) {
    // Parse rag_json to get:
    // - answer: string - The generated answer
    // - sources: array - Array of source documents with scores
    printf("RAG answer: %s\n", rag_json);
    bosbase_free_string(rag_json);
}
```

### Custom Prompt Template

```c
const char* rag_custom = 
    "{"
    "\"collection\":\"knowledge-base\","
    "\"question\":\"Summarize the explanation below in 2 sentences.\","
    "\"promptTemplate\":\"Context:\\n{{.context}}\\n\\nQuestion: {{.question}}\\nSummary:\""
    "}";

char* custom_rag = NULL;
bosbase_send(pb, "/api/langchaingo/rag", "POST", rag_custom, 
    "{}", "{}", 30000, NULL, 0, &custom_rag, NULL);
bosbase_free_string(custom_rag);
```

## LLM Document Queries

> **Note**: This interface is only available to superusers.

```c
const char* query_body = 
    "{"
    "\"collection\":\"knowledge-base\","
    "\"query\":\"List three bullet points about Rayleigh scattering.\","
    "\"topK\":3,"
    "\"returnSources\":true"
    "}";

char* query_result = NULL;
if (bosbase_send(pb, "/api/langchaingo/documents/query", "POST", 
        query_body, "{}", "{}", 30000, NULL, 0, 
        &query_result, NULL) == 0) {
    // Parse query_result to get:
    // - answer: string
    // - sources: array
    printf("Query result: %s\n", query_result);
    bosbase_free_string(query_result);
}
```

## SQL Generation + Execution

> **Important Notes**:
> - This interface is only available to superusers
> - SQL queries are executed against the BosBase database
> - Use with caution as this allows direct database access

```c
const char* sql_body = 
    "{"
    "\"query\":\"How many users signed up this month?\","
    "\"collection\":\"users\""
    "}";

char* sql_result = NULL;
if (bosbase_send(pb, "/api/langchaingo/sql", "POST", 
        sql_body, "{}", "{}", 30000, NULL, 0, 
        &sql_result, NULL) == 0) {
    // Parse sql_result to get:
    // - sql: string - The generated SQL query
    // - result: array - Query results
    printf("SQL result: %s\n", sql_result);
    bosbase_free_string(sql_result);
}
```

## Model Configuration

You can override the default model configuration:

```c
const char* custom_model = 
    "{"
    "\"model\":{"
    "\"provider\":\"ollama\","
    "\"model\":\"llama2\","
    "\"baseUrl\":\"http://localhost:11434\""
    "},"
    "\"messages\":[{\"role\":\"user\",\"content\":\"Hello\"}]"
    "}";

char* custom_result = NULL;
bosbase_send(pb, "/api/langchaingo/completions", "POST", 
    custom_model, "{}", "{}", 30000, NULL, 0, &custom_result, NULL);
bosbase_free_string(custom_result);
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
    
    // Completion
    const char* completion = 
        "{\"messages\":[{\"role\":\"user\",\"content\":\"Hello\"}]}";
    char* comp_result = NULL;
    bosbase_send(pb, "/api/langchaingo/completions", "POST", 
        completion, "{}", "{}", 30000, NULL, 0, &comp_result, NULL);
    printf("Completion: %s\n", comp_result);
    bosbase_free_string(comp_result);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [LLM Documents](./LLM_DOCUMENTS.md) - LLM document management
- [Vector API](./VECTOR_API.md) - Vector search operations

