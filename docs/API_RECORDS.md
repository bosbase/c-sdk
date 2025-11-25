# API Records - C SDK Documentation

## Overview

The Records API provides comprehensive CRUD (Create, Read, Update, Delete) operations for collection records, along with powerful search, filtering, and authentication capabilities.

**Key Features:**
- Paginated list and search with filtering and sorting
- Single record retrieval with expand support
- Create, update, and delete operations
- Batch operations for multiple records
- Authentication methods (password, OAuth2, OTP)
- Email verification and password reset
- Relation expansion up to 6 levels deep
- Field selection and excerpt modifiers

**Backend Endpoints:**
- `GET /api/collections/{collection}/records` - List records
- `GET /api/collections/{collection}/records/{id}` - View record
- `POST /api/collections/{collection}/records` - Create record
- `PATCH /api/collections/{collection}/records/{id}` - Update record
- `DELETE /api/collections/{collection}/records/{id}` - Delete record
- `POST /api/batch` - Batch operations

## CRUD Operations

### List/Search Records

Returns a paginated records list with support for sorting, filtering, and expansion.

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Basic list with pagination
    char* records_json = NULL;
    if (bosbase_collection_get_list(pb, "posts", 1, 50, NULL, NULL, NULL, 
            NULL, "{}", "{}", &records_json, NULL) == 0) {
        printf("Records: %s\n", records_json);
        bosbase_free_string(records_json);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

#### Advanced List with Filtering and Sorting

```c
// Filter and sort
char* result_json = NULL;
if (bosbase_collection_get_list(pb, "posts", 1, 50, 
        "created >= \"2022-01-01 00:00:00\" && status = \"published\"",
        "-created,title",  // DESC by created, ASC by title
        "author,categories", NULL, "{}", "{}", &result_json, NULL) == 0) {
    printf("Filtered records: %s\n", result_json);
    bosbase_free_string(result_json);
}

// Filter with operators
char* result2_json = NULL;
if (bosbase_collection_get_list(pb, "posts", 1, 50, 
        "title ~ \"javascript\" && views > 100",
        "-views", NULL, NULL, "{}", "{}", &result2_json, NULL) == 0) {
    printf("Search results: %s\n", result2_json);
    bosbase_free_string(result2_json);
}
```

#### Get Full List

Fetch all records at once (useful for small collections):

```c
// Get all records with batch size
char* all_posts_json = NULL;
if (bosbase_collection_get_full_list(pb, "posts", 200, 
        "status = \"published\"", "-created", NULL, NULL, 
        "{}", "{}", &all_posts_json, NULL) == 0) {
    printf("All posts: %s\n", all_posts_json);
    bosbase_free_string(all_posts_json);
}
```

### View Record

Retrieve a single record by ID:

```c
// Basic retrieval
char* record_json = NULL;
if (bosbase_collection_get_one(pb, "posts", "RECORD_ID", 
        NULL, NULL, "{}", "{}", &record_json, NULL) == 0) {
    printf("Record: %s\n", record_json);
    bosbase_free_string(record_json);
}

// With expanded relations
char* record_expanded_json = NULL;
if (bosbase_collection_get_one(pb, "posts", "RECORD_ID", 
        "author,categories,tags", NULL, "{}", "{}", 
        &record_expanded_json, NULL) == 0) {
    printf("Record with relations: %s\n", record_expanded_json);
    bosbase_free_string(record_expanded_json);
}

// Nested expand
char* nested_json = NULL;
if (bosbase_collection_get_one(pb, "comments", "COMMENT_ID", 
        "post.author,user", NULL, "{}", "{}", &nested_json, NULL) == 0) {
    printf("Nested relations: %s\n", nested_json);
    bosbase_free_string(nested_json);
}

// Field selection (via query_json)
const char* query = "{\"fields\":\"id,title,content,author.name\"}";
char* selected_json = NULL;
if (bosbase_collection_get_one(pb, "posts", "RECORD_ID", 
        NULL, NULL, query, "{}", &selected_json, NULL) == 0) {
    printf("Selected fields: %s\n", selected_json);
    bosbase_free_string(selected_json);
}
```

### Create Record

Create a new record:

```c
// Simple create
const char* body = "{\"title\":\"My First Post\","
                   "\"content\":\"Lorem ipsum...\","
                   "\"status\":\"draft\"}";

char* created_json = NULL;
if (bosbase_collection_create(pb, "posts", body, NULL, 0, 
        NULL, NULL, "{}", "{}", &created_json, NULL) == 0) {
    printf("Created: %s\n", created_json);
    bosbase_free_string(created_json);
}

// Create with relations
const char* body_with_relations = 
    "{\"title\":\"My Post\","
    "\"author\":\"AUTHOR_ID\","
    "\"categories\":[\"cat1\",\"cat2\"]}";

char* relation_json = NULL;
if (bosbase_collection_create(pb, "posts", body_with_relations, NULL, 0, 
        NULL, NULL, "{}", "{}", &relation_json, NULL) == 0) {
    printf("Created with relations: %s\n", relation_json);
    bosbase_free_string(relation_json);
}

// Create with file upload
bosbase_file_attachment files[1] = {
    {
        .field = "image",
        .filename = "photo.jpg",
        .content_type = "image/jpeg",
        .data = image_data,
        .data_len = image_size
    }
};

const char* body_with_file = "{\"title\":\"My Post\"}";
char* file_json = NULL;
if (bosbase_collection_create(pb, "posts", body_with_file, files, 1, 
        NULL, NULL, "{}", "{}", &file_json, NULL) == 0) {
    printf("Created with file: %s\n", file_json);
    bosbase_free_string(file_json);
}

// Create with expand to get related data immediately
const char* query_expand = "{\"expand\":\"author\"}";
char* expanded_json = NULL;
if (bosbase_collection_create(pb, "posts", body_with_relations, NULL, 0, 
        "author", NULL, query_expand, "{}", &expanded_json, NULL) == 0) {
    printf("Created with expand: %s\n", expanded_json);
    bosbase_free_string(expanded_json);
}
```

### Update Record

Update an existing record:

```c
// Simple update
const char* update_body = 
    "{\"title\":\"Updated Title\",\"status\":\"published\"}";

char* updated_json = NULL;
if (bosbase_collection_update(pb, "posts", "RECORD_ID", update_body, 
        NULL, 0, NULL, NULL, "{}", "{}", &updated_json, NULL) == 0) {
    printf("Updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}

// Update with relations using modifiers
const char* relation_update = 
    "{\"categories+\":\"NEW_CATEGORY_ID\",\"tags-\":\"OLD_TAG_ID\"}";

char* mod_json = NULL;
if (bosbase_collection_update(pb, "posts", "RECORD_ID", relation_update, 
        NULL, 0, NULL, NULL, "{}", "{}", &mod_json, NULL) == 0) {
    printf("Updated relations: %s\n", mod_json);
    bosbase_free_string(mod_json);
}

// Update with file upload
bosbase_file_attachment new_file[1] = {
    {
        .field = "image",
        .filename = "new_photo.jpg",
        .content_type = "image/jpeg",
        .data = new_image_data,
        .data_len = new_image_size
    }
};

const char* file_update = "{\"title\":\"Updated Title\"}";
char* file_update_json = NULL;
if (bosbase_collection_update(pb, "posts", "RECORD_ID", file_update, 
        new_file, 1, NULL, NULL, "{}", "{}", &file_update_json, NULL) == 0) {
    printf("Updated with file: %s\n", file_update_json);
    bosbase_free_string(file_update_json);
}
```

### Delete Record

Delete a record:

```c
// Simple delete
bosbase_error* err = NULL;
if (bosbase_collection_delete(pb, "posts", "RECORD_ID", 
        NULL, "{}", "{}", &err) != 0) {
    fprintf(stderr, "Delete failed: %s\n", err->message);
    bosbase_free_error(err);
} else {
    printf("Record deleted successfully\n");
}
```

## Filter Syntax

The filter parameter supports a powerful query syntax:

### Comparison Operators

```c
// Equal
const char* filter = "status = \"published\"";

// Not equal
const char* filter2 = "status != \"draft\"";

// Greater than / Less than
const char* filter3 = "views > 100";
const char* filter4 = "created < \"2023-01-01\"";

// Greater/Less than or equal
const char* filter5 = "age >= 18";
const char* filter6 = "price <= 99.99";
```

### String Operators

```c
// Contains (like)
const char* filter = "title ~ \"javascript\"";
// Equivalent to: title LIKE "%javascript%"

// Not contains
const char* filter2 = "title !~ \"deprecated\"";

// Exact match (case-sensitive)
const char* filter3 = "email = \"user@example.com\"";
```

### Array Operators (for multiple relations/files)

```c
// Any of / At least one
const char* filter = "tags.id ?= \"TAG_ID\"";         // Any tag matches
const char* filter2 = "tags.name ?~ \"important\"";    // Any tag name contains "important"

// All must match
const char* filter3 = "tags.id = \"TAG_ID\" && tags.id = \"TAG_ID2\"";
```

### Logical Operators

```c
// AND
const char* filter = "status = \"published\" && views > 100";

// OR
const char* filter2 = "status = \"published\" || status = \"featured\"";

// Parentheses for grouping
const char* filter3 = "(status = \"published\" || featured = true) && views > 50";
```

### Special Identifiers

```c
// Request context (only in API rules, not client filters)
// @request.auth.id, @request.query.*, etc.

// Collection joins
const char* filter = "@collection.users.email = \"test@example.com\"";

// Record fields
const char* filter2 = "author.id = @request.auth.id";
```

## Sorting

Sort records using the `sort` parameter:

```c
// Single field (ASC)
const char* sort = "created";

// Single field (DESC)
const char* sort2 = "-created";

// Multiple fields
const char* sort3 = "-created,title";  // DESC by created, then ASC by title

// Supported fields
const char* sort4 = "@random";         // Random order
const char* sort5 = "@rowid";          // Internal row ID
const char* sort6 = "id";              // Record ID
const char* sort7 = "fieldName";       // Any collection field

// Relation field sorting
const char* sort8 = "author.name";     // Sort by related author's name
```

## Field Selection

Control which fields are returned via query_json:

```c
// Specific fields
const char* query = "{\"fields\":\"id,title,content\"}";

// All fields at level
const char* query2 = "{\"fields\":\"*\"}";

// Nested field selection
const char* query3 = "{\"fields\":\"*,author.name,author.email\"}";

// Excerpt modifier for text fields
const char* query4 = "{\"fields\":\"*,content:excerpt(200,true)\"}";
// Returns first 200 characters with ellipsis if truncated
```

## Expanding Relations

Expand related records without additional API calls:

```c
// Single relation
const char* expand = "author";

// Multiple relations
const char* expand2 = "author,categories,tags";

// Nested relations (up to 6 levels)
const char* expand3 = "author.profile,categories.tags";

// Back-relations
const char* expand4 = "comments_via_post.user";
```

See [Relations Documentation](./RELATIONS.md) for detailed information.

## Pagination Options

```c
// Skip total count (faster queries) - via query_json
const char* query = "{\"skipTotal\":true}";
char* result_json = NULL;
if (bosbase_collection_get_list(pb, "posts", 1, 50, 
        "status = \"published\"", NULL, NULL, NULL, 
        query, "{}", &result_json, NULL) == 0) {
    // totalItems and totalPages will be -1
    printf("Results: %s\n", result_json);
    bosbase_free_string(result_json);
}

// Get Full List with batch processing
char* all_json = NULL;
if (bosbase_collection_get_full_list(pb, "posts", 200, 
        NULL, "-created", NULL, NULL, "{}", "{}", &all_json, NULL) == 0) {
    // Processes in batches of 200 to avoid memory issues
    printf("All records: %s\n", all_json);
    bosbase_free_string(all_json);
}
```

## Batch Operations

Execute multiple operations in a single transaction using `bosbase_send`:

```c
// Create batch request body
const char* batch_body = 
    "{"
    "\"requests\":["
    "{\"method\":\"POST\",\"url\":\"/api/collections/posts/records\","
    "\"body\":{\"title\":\"Post 1\",\"author\":\"AUTHOR_ID\"}},"
    "{\"method\":\"POST\",\"url\":\"/api/collections/posts/records\","
    "\"body\":{\"title\":\"Post 2\",\"author\":\"AUTHOR_ID\"}},"
    "{\"method\":\"PATCH\",\"url\":\"/api/collections/tags/records/TAG_ID\","
    "\"body\":{\"name\":\"Updated Tag\"}},"
    "{\"method\":\"DELETE\",\"url\":\"/api/collections/categories/records/CAT_ID\"}"
    "]"
    "}";

char* batch_result = NULL;
bosbase_error* err = NULL;
if (bosbase_send(pb, "/api/batch", "POST", batch_body, 
        "{}", "{}", 30000, NULL, 0, &batch_result, &err) == 0) {
    printf("Batch results: %s\n", batch_result);
    bosbase_free_string(batch_result);
} else {
    fprintf(stderr, "Batch failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

**Note**: Batch operations must be enabled in Dashboard > Settings > Application.

## Complete Examples

### Example 1: Blog Post Search with Filters

```c
void search_posts(bosbase_client* pb, const char* query, 
                  const char* category_id, int min_views) {
    char filter[512];
    snprintf(filter, sizeof(filter), 
             "title ~ \"%s\" || content ~ \"%s\"", query, query);
    
    if (category_id) {
        char temp[256];
        snprintf(temp, sizeof(temp), " && categories.id ?= \"%s\"", category_id);
        strncat(filter, temp, sizeof(filter) - strlen(filter) - 1);
    }
    
    if (min_views > 0) {
        char temp[128];
        snprintf(temp, sizeof(temp), " && views >= %d", min_views);
        strncat(filter, temp, sizeof(filter) - strlen(filter) - 1);
    }
    
    char* result_json = NULL;
    if (bosbase_collection_get_list(pb, "posts", 1, 20, 
            filter, "-created", "author,categories", NULL, 
            "{}", "{}", &result_json, NULL) == 0) {
        printf("Search results: %s\n", result_json);
        bosbase_free_string(result_json);
    }
}
```

### Example 2: Advanced Filtering

```c
// Complex filter example
const char* complex_filter = 
    "(status = \"published\" || featured = true) && "
    "created >= \"2023-01-01\" && "
    "(tags.id ?= \"important\" || categories.id = \"news\") && "
    "views > 100 && "
    "author.email != \"\"";

const char* query = "{\"fields\":\"*,content:excerpt(300),author.name,author.email\"}";

char* result_json = NULL;
if (bosbase_collection_get_list(pb, "posts", 1, 50, 
        complex_filter, "-views,created", 
        "author.profile,tags,categories", NULL, 
        query, "{}", &result_json, NULL) == 0) {
    printf("Filtered results: %s\n", result_json);
    bosbase_free_string(result_json);
}
```

### Example 3: Pagination Helper

```c
void get_all_records_paginated(bosbase_client* pb, 
                                const char* collection_name,
                                const char* filter,
                                const char* sort) {
    int page = 1;
    int has_more = 1;
    
    while (has_more) {
        const char* query = "{\"skipTotal\":true}";
        char* result_json = NULL;
        
        if (bosbase_collection_get_list(pb, collection_name, page, 500, 
                filter, sort, NULL, NULL, query, "{}", 
                &result_json, NULL) == 0) {
            // Parse JSON to check if there are more records
            // (In practice, use a JSON library)
            printf("Page %d: %s\n", page, result_json);
            
            // Check if we got 500 records (likely more available)
            // has_more = (number_of_items == 500);
            
            bosbase_free_string(result_json);
            page++;
        } else {
            has_more = 0;
        }
    }
}
```

## Error Handling

```c
char* result = NULL;
bosbase_error* err = NULL;

if (bosbase_collection_create(pb, "posts", body, NULL, 0, 
        NULL, NULL, "{}", "{}", &result, &err) != 0) {
    if (err) {
        if (err->status == 400) {
            fprintf(stderr, "Validation error: %s\n", err->message);
        } else if (err->status == 403) {
            fprintf(stderr, "Permission denied\n");
        } else if (err->status == 404) {
            fprintf(stderr, "Collection or record not found\n");
        } else {
            fprintf(stderr, "Error %d: %s\n", err->status, err->message);
        }
        bosbase_free_error(err);
    }
    return 1;
}

// Use result...
bosbase_free_string(result);
```

## Best Practices

1. **Use Pagination**: Always use pagination for large datasets
2. **Skip Total When Possible**: Use `skipTotal: true` in query_json for better performance
3. **Batch Operations**: Use batch for multiple operations to reduce round trips
4. **Field Selection**: Only request fields you need to reduce payload size
5. **Expand Wisely**: Only expand relations you actually use
6. **Filter Before Sort**: Apply filters before sorting for better performance
7. **Memory Management**: Always free strings returned by SDK functions
8. **Error Handling**: Always check return values and handle errors gracefully

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection configuration
- [Relations](./RELATIONS.md) - Working with relations
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Filter syntax details
- [Authentication](./AUTHENTICATION.md) - Detailed authentication guide
- [Files](./FILES.md) - File uploads and handling

