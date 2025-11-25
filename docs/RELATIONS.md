# Working with Relations - C SDK Documentation

## Overview

Relations allow you to link records between collections. BosBase supports both single and multiple relations, and provides powerful features for expanding related records and working with back-relations.

**Key Features:**
- Single and multiple relations
- Expand related records without additional requests
- Nested relation expansion (up to 6 levels)
- Back-relations for reverse lookups
- Field modifiers for append/prepend/remove operations

**Relation Field Types:**
- **Single Relation**: Links to one record (MaxSelect <= 1)
- **Multiple Relation**: Links to multiple records (MaxSelect > 1)

## Setting Up Relations

### Creating a Relation Field

```c
// Get existing collection
char* collection_json = NULL;
if (bosbase_send(pb, "/api/collections/posts", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &collection_json, NULL) == 0) {
    // Parse JSON, add relation field, then update
    // Field definition:
    // {
    //   "name": "user",
    //   "type": "relation",
    //   "options": {"collectionId": "users"},
    //   "maxSelect": 1,
    //   "required": true
    // }
    
    // For multiple relation:
    // {
    //   "name": "tags",
    //   "type": "relation",
    //   "options": {"collectionId": "tags"},
    //   "maxSelect": 10,
    //   "minSelect": 1
    // }
    
    bosbase_free_string(collection_json);
}
```

## Creating Records with Relations

### Single Relation

```c
// Create a post with a single user relation
const char* body = "{\"title\":\"My Post\",\"user\":\"USER_ID\"}";
char* result_json = NULL;
if (bosbase_collection_create(pb, "posts", body, NULL, 0, 
        NULL, NULL, "{}", "{}", &result_json, NULL) == 0) {
    printf("Created with relation: %s\n", result_json);
    bosbase_free_string(result_json);
}
```

### Multiple Relations

```c
// Create a post with multiple tags
const char* body = 
    "{\"title\":\"My Post\","
    "\"tags\":[\"TAG_ID1\",\"TAG_ID2\",\"TAG_ID3\"]}";

char* result_json = NULL;
if (bosbase_collection_create(pb, "posts", body, NULL, 0, 
        NULL, NULL, "{}", "{}", &result_json, NULL) == 0) {
    printf("Created with multiple relations: %s\n", result_json);
    bosbase_free_string(result_json);
}
```

### Mixed Relations

```c
// Create a comment with both single and multiple relations
const char* body = 
    "{\"message\":\"Great post!\","
    "\"post\":\"POST_ID\","
    "\"user\":\"USER_ID\","
    "\"tags\":[\"TAG1\",\"TAG2\"]}";

char* result_json = NULL;
bosbase_collection_create(pb, "comments", body, NULL, 0, 
    NULL, NULL, "{}", "{}", &result_json, NULL);
bosbase_free_string(result_json);
```

## Updating Relations

### Replace All Relations

```c
// Replace all tags
const char* body = "{\"tags\":[\"NEW_TAG1\",\"NEW_TAG2\"]}";
char* result_json = NULL;
bosbase_collection_update(pb, "posts", "POST_ID", body, 
    NULL, 0, NULL, NULL, "{}", "{}", &result_json, NULL);
bosbase_free_string(result_json);
```

### Append Relations (Using + Modifier)

```c
// Append tags to existing ones
const char* body = "{\"tags+\":\"NEW_TAG_ID\"}";  // Append single tag
char* result_json = NULL;
bosbase_collection_update(pb, "posts", "POST_ID", body, 
    NULL, 0, NULL, NULL, "{}", "{}", &result_json, NULL);
bosbase_free_string(result_json);

// Append multiple tags
const char* body2 = "{\"tags+\":[\"TAG_ID1\",\"TAG_ID2\"]}";
char* result2_json = NULL;
bosbase_collection_update(pb, "posts", "POST_ID", body2, 
    NULL, 0, NULL, NULL, "{}", "{}", &result2_json, NULL);
bosbase_free_string(result2_json);
```

### Prepend Relations (Using + Prefix)

```c
// Prepend tags (tags will appear first)
const char* body = "{\"+tags\":\"PRIORITY_TAG\"}";
char* result_json = NULL;
bosbase_collection_update(pb, "posts", "POST_ID", body, 
    NULL, 0, NULL, NULL, "{}", "{}", &result_json, NULL);
bosbase_free_string(result_json);
```

### Remove Relations (Using - Modifier)

```c
// Remove single tag
const char* body = "{\"tags-\":\"TAG_ID_TO_REMOVE\"}";
char* result_json = NULL;
bosbase_collection_update(pb, "posts", "POST_ID", body, 
    NULL, 0, NULL, NULL, "{}", "{}", &result_json, NULL);
bosbase_free_string(result_json);

// Remove multiple tags
const char* body2 = "{\"tags-\":[\"TAG1\",\"TAG2\"]}";
char* result2_json = NULL;
bosbase_collection_update(pb, "posts", "POST_ID", body2, 
    NULL, 0, NULL, NULL, "{}", "{}", &result2_json, NULL);
bosbase_free_string(result2_json);
```

## Expanding Relations

The `expand` parameter allows you to fetch related records in a single request, eliminating the need for multiple API calls.

### Basic Expand

```c
// Get comment with expanded user
char* comment_json = NULL;
if (bosbase_collection_get_one(pb, "comments", "COMMENT_ID", 
        "user", NULL, "{}", "{}", &comment_json, NULL) == 0) {
    // Parse comment_json to access:
    // - comment.expand.user.name
    // - comment.user (still the ID: "USER_ID")
    printf("Comment with user: %s\n", comment_json);
    bosbase_free_string(comment_json);
}
```

### Expand Multiple Relations

```c
// Expand multiple relations (comma-separated)
char* comment_json = NULL;
if (bosbase_collection_get_one(pb, "comments", "COMMENT_ID", 
        "user,post", NULL, "{}", "{}", &comment_json, NULL) == 0) {
    // Parse to access:
    // - comment.expand.user.name
    // - comment.expand.post.title
    printf("Comment with relations: %s\n", comment_json);
    bosbase_free_string(comment_json);
}
```

### Nested Expand (Dot Notation)

You can expand nested relations up to 6 levels deep using dot notation:

```c
// Expand post and its tags, and user
char* comment_json = NULL;
if (bosbase_collection_get_one(pb, "comments", "COMMENT_ID", 
        "user,post.tags", NULL, "{}", "{}", &comment_json, NULL) == 0) {
    // Parse to access nested expands:
    // - comment.expand.post.expand.tags
    printf("Nested relations: %s\n", comment_json);
    bosbase_free_string(comment_json);
}

// Expand even deeper
char* post_json = NULL;
if (bosbase_collection_get_one(pb, "posts", "POST_ID", 
        "user,comments.user", NULL, "{}", "{}", &post_json, NULL) == 0) {
    // Access: post.expand.comments[].expand.user
    printf("Deep nested: %s\n", post_json);
    bosbase_free_string(post_json);
}
```

## Back-Relations

Back-relations allow you to find records that reference the current record through a relation field.

### Understanding Back-Relations

If you have:
- Collection `posts` with field `author` (relation to `users`)
- Collection `comments` with field `post` (relation to `posts`)

You can expand back-relations:
- From a `post`: `comments_via_post` - all comments that reference this post
- From a `user`: `posts_via_author` - all posts authored by this user

### Using Back-Relations

```c
// Get post with back-relation to comments
char* post_json = NULL;
if (bosbase_collection_get_one(pb, "posts", "POST_ID", 
        "comments_via_post", NULL, "{}", "{}", &post_json, NULL) == 0) {
    // Parse to access:
    // - post.expand.comments_via_post (array of comment records)
    printf("Post with comments: %s\n", post_json);
    bosbase_free_string(post_json);
}

// Get user with back-relation to posts
char* user_json = NULL;
if (bosbase_collection_get_one(pb, "users", "USER_ID", 
        "posts_via_author", NULL, "{}", "{}", &user_json, NULL) == 0) {
    // Access: user.expand.posts_via_author
    printf("User with posts: %s\n", user_json);
    bosbase_free_string(user_json);
}
```

### Nested Back-Relations

```c
// Expand back-relation and its nested relations
char* post_json = NULL;
if (bosbase_collection_get_one(pb, "posts", "POST_ID", 
        "comments_via_post.user", NULL, "{}", "{}", &post_json, NULL) == 0) {
    // Access: post.expand.comments_via_post[].expand.user
    printf("Post with comments and users: %s\n", post_json);
    bosbase_free_string(post_json);
}
```

## Filtering by Relations

You can filter records based on related record properties:

```c
// Filter posts by author's email
const char* filter = "@collection.users.email = \"admin@example.com\"";
char* posts_json = NULL;
if (bosbase_collection_get_list(pb, "posts", 1, 20, filter, 
        NULL, NULL, NULL, "{}", "{}", &posts_json, NULL) == 0) {
    printf("Filtered posts: %s\n", posts_json);
    bosbase_free_string(posts_json);
}

// Filter by relation ID
const char* filter2 = "author = \"USER_ID\"";
char* posts2_json = NULL;
bosbase_collection_get_list(pb, "posts", 1, 20, filter2, 
    NULL, NULL, NULL, "{}", "{}", &posts2_json, NULL);
bosbase_free_string(posts2_json);
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Create post with author relation
    const char* post_body = 
        "{\"title\":\"My Post\",\"author\":\"USER_ID\","
        "\"tags\":[\"TAG1\",\"TAG2\"]}";
    
    char* post_json = NULL;
    if (bosbase_collection_create(pb, "posts", post_body, NULL, 0, 
            NULL, NULL, "{}", "{}", &post_json, NULL) == 0) {
        // Parse to get post ID
        
        // Get post with expanded relations
        char* expanded_json = NULL;
        if (bosbase_collection_get_one(pb, "posts", "POST_ID", 
                "author,tags,comments_via_post.user", NULL, 
                "{}", "{}", &expanded_json, NULL) == 0) {
            // Access:
            // - expanded.expand.author (user record)
            // - expanded.expand.tags (array of tag records)
            // - expanded.expand.comments_via_post[].expand.user
            printf("Post with all relations: %s\n", expanded_json);
            bosbase_free_string(expanded_json);
        }
        bosbase_free_string(post_json);
    }
    
    // Update relations
    const char* update_body = 
        "{\"tags+\":\"NEW_TAG\",\"tags-\":\"OLD_TAG\"}";
    char* updated_json = NULL;
    bosbase_collection_update(pb, "posts", "POST_ID", update_body, 
        NULL, 0, NULL, NULL, "{}", "{}", &updated_json, NULL);
    bosbase_free_string(updated_json);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Best Practices

1. **Use Expand Wisely**: Only expand relations you actually need to reduce payload size
2. **Nested Expands**: Limit nested expand depth to avoid large responses
3. **Back-Relations**: Use back-relations for reverse lookups instead of multiple queries
4. **Field Modifiers**: Use `+`, `-` modifiers for efficient relation updates
5. **Filter Before Expand**: Apply filters before expanding to reduce data transfer

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection configuration
- [API Records](./API_RECORDS.md) - Record operations
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Filter syntax

