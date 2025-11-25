# Logs API - C SDK Documentation

## Overview

The Logs API provides endpoints for viewing and analyzing application logs. All operations require superuser authentication and allow you to query request logs, filter by various criteria, and get aggregated statistics.

**Key Features:**
- List and paginate logs
- View individual log entries
- Filter logs by status, URL, method, IP, etc.
- Sort logs by various fields
- Get hourly aggregated statistics
- Filter statistics by criteria

**Backend Endpoints:**
- `GET /api/logs` - List logs
- `GET /api/logs/{id}` - View log
- `GET /api/logs/stats` - Get statistics

**Note**: All Logs API operations require superuser authentication.

## Authentication

All Logs API operations require superuser authentication:

```c
#include "bosbase_c.h"

bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");

// Authenticate as superuser
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);
```

## List Logs

Returns a paginated list of logs with support for filtering and sorting.

```c
// Basic list
char* logs_json = NULL;
if (bosbase_send(pb, "/api/logs", "GET", NULL, 
        "{\"page\":1,\"perPage\":30}", "{}", 30000, NULL, 0, 
        &logs_json, NULL) == 0) {
    // Parse logs_json to get:
    // - page: number
    // - perPage: number
    // - totalItems: number
    // - items: array of log entries
    printf("Logs: %s\n", logs_json);
    bosbase_free_string(logs_json);
}
```

### Log Entry Structure

Each log entry contains:

```json
{
  "id": "ai5z3aoed6809au",
  "created": "2024-10-27T09:28:19.524Z",
  "level": 0,
  "message": "GET /api/collections/posts/records",
  "data": {
    "auth": "_superusers",
    "execTime": 2.392327,
    "method": "GET",
    "referer": "http://localhost:8090/_/",
    "remoteIP": "127.0.0.1",
    "status": 200,
    "type": "request",
    "url": "/api/collections/posts/records?page=1",
    "userAgent": "Mozilla/5.0...",
    "userIP": "127.0.0.1"
  }
}
```

### Filtering Logs

```c
// Filter by HTTP status code
const char* query = "{\"filter\":\"data.status >= 400\",\"page\":1,\"perPage\":50}";
char* error_logs = NULL;
bosbase_send(pb, "/api/logs", "GET", NULL, query, "{}", 
    30000, NULL, 0, &error_logs, NULL);
bosbase_free_string(error_logs);

// Filter by method
const char* query2 = "{\"filter\":\"data.method = \\\"GET\\\"\",\"page\":1,\"perPage\":50}";
char* get_logs = NULL;
bosbase_send(pb, "/api/logs", "GET", NULL, query2, "{}", 
    30000, NULL, 0, &get_logs, NULL);
bosbase_free_string(get_logs);

// Filter by URL pattern
const char* query3 = "{\"filter\":\"data.url ~ \\\"/api/\\\"\",\"page\":1,\"perPage\":50}";
char* api_logs = NULL;
bosbase_send(pb, "/api/logs", "GET", NULL, query3, "{}", 
    30000, NULL, 0, &api_logs, NULL);
bosbase_free_string(api_logs);
```

## View Log

Retrieve a single log entry by ID.

```c
char* log_json = NULL;
if (bosbase_send(pb, "/api/logs/LOG_ID", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &log_json, NULL) == 0) {
    printf("Log entry: %s\n", log_json);
    bosbase_free_string(log_json);
}
```

## Get Statistics

Get hourly aggregated statistics.

```c
char* stats_json = NULL;
if (bosbase_send(pb, "/api/logs/stats", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &stats_json, NULL) == 0) {
    // Parse stats_json to get hourly statistics
    printf("Statistics: %s\n", stats_json);
    bosbase_free_string(stats_json);
}
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "_superusers", 
        "admin@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // List logs
    char* logs = NULL;
    bosbase_send(pb, "/api/logs", "GET", NULL, 
        "{\"page\":1,\"perPage\":50}", "{}", 30000, NULL, 0, &logs, NULL);
    printf("Logs: %s\n", logs);
    bosbase_free_string(logs);
    
    // Get statistics
    char* stats = NULL;
    bosbase_send(pb, "/api/logs/stats", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &stats, NULL);
    printf("Stats: %s\n", stats);
    bosbase_free_string(stats);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Crons API](./CRONS_API.md) - Cron job management
- [Health API](./HEALTH_API.md) - Server health checks

