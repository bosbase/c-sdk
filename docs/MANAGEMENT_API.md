# Management API - C SDK Documentation

This document covers the management API capabilities available in the C SDK, which correspond to the features available in the backend management UI.

> **Note**: All management API operations require superuser authentication.

## Table of Contents

- [Settings Service](#settings-service)
- [Backup Service](#backup-service)
- [Log Service](#log-service)
- [Cron Service](#cron-service)
- [Health Service](#health-service)
- [Collection Service](#collection-service)

---

## Settings Service

The Settings Service provides comprehensive management of application settings.

### Get Application Settings

```c
char* settings_json = NULL;
if (bosbase_send(pb, "/api/settings", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &settings_json, NULL) == 0) {
    // Parse settings_json to get:
    // - meta: application metadata
    // - trustedProxy: proxy configuration
    // - rateLimits: rate limit rules
    // - batch: batch operation settings
    printf("Settings: %s\n", settings_json);
    bosbase_free_string(settings_json);
}
```

### Update Application Settings

```c
const char* update_settings = 
    "{"
    "\"meta\":{"
    "\"appName\":\"My App\","
    "\"appURL\":\"https://example.com\","
    "\"hideControls\":false"
    "},"
    "\"trustedProxy\":{"
    "\"headers\":[\"X-Forwarded-For\"],"
    "\"useLeftmostIP\":true"
    "},"
    "\"rateLimits\":{"
    "\"enabled\":true,"
    "\"rules\":["
    "{"
    "\"label\":\"api/users\","
    "\"duration\":3600,"
    "\"maxRequests\":100"
    "}"
    "]"
    "},"
    "\"batch\":{"
    "\"enabled\":true,"
    "\"maxRequests\":100,"
    "\"interval\":200"
    "}"
    "}";

char* updated_json = NULL;
if (bosbase_send(pb, "/api/settings", "PATCH", update_settings, 
        "{}", "{}", 30000, NULL, 0, &updated_json, NULL) == 0) {
    printf("Settings updated: %s\n", updated_json);
    bosbase_free_string(updated_json);
}
```

### Update Mail Configuration

```c
const char* mail_config = 
    "{"
    "\"enabled\":true,"
    "\"host\":\"smtp.example.com\","
    "\"port\":587,"
    "\"username\":\"user@example.com\","
    "\"password\":\"password\","
    "\"tls\":true"
    "}";

char* mail_json = NULL;
bosbase_send(pb, "/api/settings/mail", "PATCH", mail_config, 
    "{}", "{}", 30000, NULL, 0, &mail_json, NULL);
bosbase_free_string(mail_json);
```

### Update Storage Configuration

```c
const char* storage_config = 
    "{"
    "\"type\":\"s3\","
    "\"bucket\":\"my-bucket\","
    "\"region\":\"us-east-1\","
    "\"endpoint\":\"https://s3.amazonaws.com\","
    "\"accessKey\":\"ACCESS_KEY\","
    "\"secretKey\":\"SECRET_KEY\""
    "}";

char* storage_json = NULL;
bosbase_send(pb, "/api/settings/storage", "PATCH", storage_config, 
    "{}", "{}", 30000, NULL, 0, &storage_json, NULL);
bosbase_free_string(storage_json);
```

## Backup Service

See [Backups API](./BACKUPS_API.md) for detailed backup operations.

## Log Service

See [Logs API](./LOGS_API.md) for detailed log operations.

## Cron Service

See [Crons API](./CRONS_API.md) for detailed cron operations.

## Health Service

See [Health API](./HEALTH_API.md) for detailed health check operations.

## Collection Service

See [Collection API](./COLLECTION_API.md) for detailed collection management.

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate as superuser
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "_superusers", 
        "admin@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // Get settings
    char* settings = NULL;
    bosbase_send(pb, "/api/settings", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &settings, NULL);
    printf("Settings: %s\n", settings);
    bosbase_free_string(settings);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Backups API](./BACKUPS_API.md) - Backup management
- [Logs API](./LOGS_API.md) - Log viewing
- [Crons API](./CRONS_API.md) - Cron job management
- [Health API](./HEALTH_API.md) - Health checks

