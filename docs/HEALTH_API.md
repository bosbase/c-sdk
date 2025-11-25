# Health API - C SDK Documentation

## Overview

The Health API provides a simple endpoint to check the health status of the server. It returns basic health information and, when authenticated as a superuser, provides additional diagnostic information about the server state.

**Key Features:**
- No authentication required for basic health check
- Superuser authentication provides additional diagnostic data
- Lightweight endpoint for monitoring and health checks
- Supports both GET and HEAD methods

**Backend Endpoints:**
- `GET /api/health` - Check health status
- `HEAD /api/health` - Check health status (HEAD method)

**Note**: The health endpoint is publicly accessible, but superuser authentication provides additional information.

## Authentication

Basic health checks do not require authentication:

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Basic health check (no auth required)
    char* health_json = NULL;
    if (bosbase_health_check(pb, "{}", "{}", &health_json, NULL) == 0) {
        printf("Health status: %s\n", health_json);
        bosbase_free_string(health_json);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

For additional diagnostic information, authenticate as a superuser:

```c
// Authenticate as superuser for extended health data
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

char* health_json = NULL;
bosbase_health_check(pb, "{}", "{}", &health_json, NULL);
// Parse health_json to access extended diagnostic data
bosbase_free_string(health_json);
```

## Health Check Response Structure

### Basic Response (Guest/Regular User)

```json
{
  "code": 200,
  "message": "API is healthy.",
  "data": {}
}
```

### Superuser Response

```json
{
  "code": 200,
  "message": "API is healthy.",
  "data": {
    "canBackup": true,
    "realIP": "192.168.1.100",
    "requireS3": false,
    "possibleProxyHeader": "X-Forwarded-For"
  }
}
```

## Check Health Status

Returns the health status of the API server.

### Basic Usage

```c
// Simple health check
char* health_json = NULL;
if (bosbase_health_check(pb, "{}", "{}", &health_json, NULL) == 0) {
    // Parse health_json to get:
    // - code: 200
    // - message: "API is healthy."
    printf("Health: %s\n", health_json);
    bosbase_free_string(health_json);
}
```

### With Superuser Authentication

```c
// Authenticate as superuser first
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Get extended health information
char* health_json = NULL;
if (bosbase_health_check(pb, "{}", "{}", &health_json, NULL) == 0) {
    // Parse health_json to access:
    // - data.canBackup: true/false
    // - data.realIP: "192.168.1.100"
    // - data.requireS3: false
    // - data.possibleProxyHeader: "" or header name
    printf("Extended health: %s\n", health_json);
    bosbase_free_string(health_json);
}
```

## Response Fields

### Common Fields (All Users)

| Field | Type | Description |
|-------|------|-------------|
| `code` | number | HTTP status code (always 200 for healthy server) |
| `message` | string | Health status message ("API is healthy.") |
| `data` | object | Health data (empty for non-superusers, populated for superusers) |

### Superuser-Only Fields (in `data`)

| Field | Type | Description |
|-------|------|-------------|
| `canBackup` | boolean | `true` if backup/restore operations can be performed, `false` if a backup/restore is currently in progress |
| `realIP` | string | The real IP address of the client (useful when behind proxies) |
| `requireS3` | boolean | `true` if S3 storage is required (local fallback disabled), `false` otherwise |
| `possibleProxyHeader` | string | Detected proxy header name (e.g., "X-Forwarded-For", "CF-Connecting-IP") if the server appears to be behind a reverse proxy, empty string otherwise |

## Use Cases

### 1. Basic Health Monitoring

```c
int check_server_health(bosbase_client* pb) {
    char* health_json = NULL;
    bosbase_error* err = NULL;
    
    if (bosbase_health_check(pb, "{}", "{}", &health_json, &err) == 0) {
        // Parse health_json to check code and message
        // If code == 200 and message == "API is healthy.", return 1
        bosbase_free_string(health_json);
        return 1;
    } else {
        if (err) {
            fprintf(stderr, "Health check error: %s\n", err->message);
            bosbase_free_error(err);
        }
        return 0;
    }
}
```

### 2. Backup Readiness Check

```c
int can_perform_backup(bosbase_client* pb) {
    // Authenticate as superuser
    char* auth_json = NULL;
    if (bosbase_collection_auth_with_password(pb, "_superusers", 
            "admin@example.com", "password", "{}", NULL, NULL, 
            "{}", "{}", &auth_json, NULL) != 0) {
        return 0;
    }
    bosbase_free_string(auth_json);
    
    // Check health
    char* health_json = NULL;
    if (bosbase_health_check(pb, "{}", "{}", &health_json, NULL) == 0) {
        // Parse health_json to check data.canBackup
        // Return 1 if canBackup == true, 0 otherwise
        bosbase_free_string(health_json);
        return 1;
    }
    return 0;
}
```

## Error Handling

```c
char* health_json = NULL;
bosbase_error* err = NULL;

if (bosbase_health_check(pb, "{}", "{}", &health_json, &err) != 0) {
    if (err) {
        fprintf(stderr, "Health check failed: %s\n", err->message);
        bosbase_free_error(err);
    }
    return 1;
}

// Use health_json...
bosbase_free_string(health_json);
```

## Best Practices

1. **Monitoring**: Use health checks for regular monitoring (e.g., every 30-60 seconds)
2. **Load Balancers**: Configure load balancers to use the health endpoint for health checks
3. **Pre-flight Checks**: Check `canBackup` before initiating backup operations
4. **Error Handling**: Always handle errors gracefully as the server may be down
5. **Rate Limiting**: Don't poll the health endpoint too frequently

## Related Documentation

- [Backups API](./BACKUPS_API.md) - Using `canBackup` to check backup readiness
- [Authentication](./AUTHENTICATION.md) - Superuser authentication

