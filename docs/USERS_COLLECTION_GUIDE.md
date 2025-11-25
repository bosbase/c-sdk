# Built-in Users Collection Guide - C SDK

This guide explains how to use the built-in `users` collection for authentication, registration, and API rules. **The `users` collection is automatically created when BosBase is initialized and does not need to be created manually.**

## Table of Contents

1. [Overview](#overview)
2. [Users Collection Structure](#users-collection-structure)
3. [User Registration](#user-registration)
4. [User Login/Authentication](#user-loginauthentication)
5. [API Rules and Filters with Users](#api-rules-and-filters-with-users)
6. [Using Users with Other Collections](#using-users-with-other-collections)
7. [Complete Examples](#complete-examples)

---

## Overview

The `users` collection is a **built-in auth collection** that is automatically created when BosBase starts. It has:

- **Collection ID**: `_pb_users_auth_`
- **Collection Name**: `users`
- **Type**: `auth` (authentication collection)
- **Purpose**: User accounts, authentication, and authorization

**Important**: 
- ✅ **DO NOT** create a new `users` collection manually
- ✅ **DO** use the existing built-in `users` collection
- ✅ The collection already has proper API rules configured
- ✅ It supports password, OAuth2, and OTP authentication

### Getting Users Collection Information

```c
// Get the users collection details
char* users_collection = NULL;
if (bosbase_send(pb, "/api/collections/users", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &users_collection, NULL) == 0) {
    // Parse users_collection to get:
    // - id: collection ID
    // - name: "users"
    // - type: "auth"
    // - fields: array of fields
    // - API rules
    printf("Users collection: %s\n", users_collection);
    bosbase_free_string(users_collection);
}
```

---

## Users Collection Structure

### System Fields (Automatically Created)

These fields are automatically added to all auth collections (including `users`):

| Field Name | Type | Description | Required | Hidden |
|------------|------|-------------|----------|--------|
| `id` | text | Unique record identifier | Yes | No |
| `email` | email | User email address | Yes* | No |
| `username` | text | Username (optional, if enabled) | No* | No |
| `password` | password | Hashed password | Yes* | Yes |
| `tokenKey` | text | Token key for auth tokens | Yes | Yes |
| `emailVisibility` | bool | Whether email is visible to others | No | No |
| `verified` | bool | Whether email is verified | No | No |
| `created` | date | Record creation timestamp | Yes | No |
| `updated` | date | Last update timestamp | Yes | No |

*Required based on authentication method configuration

### Default API Rules

The `users` collection comes with these default API rules:

```c
// Default rules (conceptual):
// listRule: "id = @request.auth.id"    // Users can only list themselves
// viewRule: "id = @request.auth.id"   // Users can only view themselves
// createRule: ""                       // Anyone can register (public)
// updateRule: "id = @request.auth.id" // Users can only update themselves
// deleteRule: "id = @request.auth.id" // Users can only delete themselves
```

---

## User Registration

### Register New User

```c
const char* user_body = 
    "{"
    "\"email\":\"user@example.com\","
    "\"password\":\"password123\","
    "\"passwordConfirm\":\"password123\","
    "\"name\":\"John Doe\""
    "}";

char* user_json = NULL;
if (bosbase_collection_create(pb, "users", user_body, NULL, 0, 
        NULL, NULL, "{}", "{}", &user_json, NULL) == 0) {
    printf("User registered: %s\n", user_json);
    bosbase_free_string(user_json);
}
```

---

## User Login/Authentication

### Password Authentication

```c
char* auth_json = NULL;
if (bosbase_collection_auth_with_password(pb, "users", 
        "user@example.com", "password123", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL) == 0) {
    // Auth data is automatically stored
    const char* token = bosbase_auth_token(pb);
    char* record = bosbase_auth_record(pb);
    printf("Authenticated: %s\n", record);
    bosbase_free_string(record);
    bosbase_free_string(auth_json);
}
```

### OAuth2 Authentication

```c
// Get auth methods
char* methods_json = NULL;
bosbase_send(pb, "/api/collections/users/auth-methods", "GET", NULL, 
    "{}", "{}", 30000, NULL, 0, &methods_json, NULL);

// Parse to get OAuth2 providers, then authenticate
// (See OAuth2_CONFIGURATION.md for details)

bosbase_free_string(methods_json);
```

### OTP Authentication

```c
// Request OTP
const char* otp_request = "{\"email\":\"user@example.com\"}";
char* otp_result = NULL;
bosbase_send(pb, "/api/collections/users/request-otp", "POST", 
    otp_request, "{}", "{}", 30000, NULL, 0, &otp_result, NULL);

// Parse to get otpId, then authenticate with OTP
const char* otp_auth = 
    "{\"otpId\":\"OTP_ID\",\"otp\":\"123456\"}";
char* auth_result = NULL;
bosbase_send(pb, "/api/collections/users/auth-with-otp", "POST", 
    otp_auth, "{}", "{}", 30000, NULL, 0, &auth_result, NULL);

bosbase_free_string(otp_result);
bosbase_free_string(auth_result);
```

---

## API Rules and Filters with Users

### Using Users in API Rules

```c
// Create collection with user-based rules
const char* collection = 
    "{"
    "\"type\":\"base\","
    "\"name\":\"posts\","
    "\"fields\":["
    "{\"name\":\"title\",\"type\":\"text\",\"required\":true},"
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
bosbase_send(pb, "/api/collections", "POST", collection, 
    "{}", "{}", 30000, NULL, 0, &created, NULL);
bosbase_free_string(created);
```

---

## Using Users with Other Collections

### Create Record with User Relation

```c
// Authenticate first
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "users", 
    "user@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Get current user ID
char* user_record = bosbase_auth_record(pb);
// Parse to get user ID

// Create post with author relation
const char* post_body = 
    "{\"title\":\"My Post\",\"author\":\"USER_ID\"}";
char* post_json = NULL;
bosbase_collection_create(pb, "posts", post_body, NULL, 0, 
    NULL, NULL, "{}", "{}", &post_json, NULL);
bosbase_free_string(post_json);

bosbase_free_string(user_record);
```

---

## Complete Examples

### Example 1: User Registration and Login

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Register user
    const char* register_body = 
        "{\"email\":\"user@example.com\","
        "\"password\":\"password123\","
        "\"passwordConfirm\":\"password123\"}";
    char* registered = NULL;
    bosbase_collection_create(pb, "users", register_body, NULL, 0, 
        NULL, NULL, "{}", "{}", &registered, NULL);
    bosbase_free_string(registered);
    
    // Login
    char* auth = NULL;
    if (bosbase_collection_auth_with_password(pb, "users", 
            "user@example.com", "password123", "{}", NULL, NULL, 
            "{}", "{}", &auth, NULL) == 0) {
        printf("Login successful\n");
        bosbase_free_string(auth);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

### Example 2: User Profile Update

```c
// Authenticate first
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "users", 
    "user@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Get current user ID
char* user_record = bosbase_auth_record(pb);
// Parse to get user ID

// Update profile
const char* update_body = "{\"name\":\"Updated Name\"}";
char* updated = NULL;
bosbase_collection_update(pb, "users", "USER_ID", update_body, 
    NULL, 0, NULL, NULL, "{}", "{}", &updated, NULL);
bosbase_free_string(updated);

bosbase_free_string(user_record);
```

## Related Documentation

- [Authentication](./AUTHENTICATION.md) - Detailed authentication guide
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Access control
- [Collections](./COLLECTIONS.md) - Collection management

