# Crons API - C SDK Documentation

## Overview

The Crons API provides endpoints for viewing and manually triggering scheduled cron jobs. All operations require superuser authentication and allow you to list registered cron jobs and execute them on-demand.

**Key Features:**
- List all registered cron jobs
- View cron job schedules (cron expressions)
- Manually trigger cron jobs
- Built-in system jobs for maintenance tasks

**Backend Endpoints:**
- `GET /api/crons` - List cron jobs
- `POST /api/crons/{jobId}` - Run cron job

**Note**: All Crons API operations require superuser authentication.

## Authentication

All Crons API operations require superuser authentication:

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

## List Cron Jobs

Returns a list of all registered cron jobs with their IDs and schedule expressions.

```c
// Get all cron jobs
char* crons_json = NULL;
if (bosbase_send(pb, "/api/crons", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &crons_json, NULL) == 0) {
    // Parse crons_json to get array of cron job objects:
    // Each job contains:
    // - id: string - Unique identifier for the job
    // - expression: string - Cron expression defining the schedule
    printf("Cron jobs: %s\n", crons_json);
    bosbase_free_string(crons_json);
}
```

### Built-in System Jobs

The following cron jobs are typically registered by default:

| Job ID | Expression | Description | Schedule |
|--------|-----------|-------------|----------|
| `__pbLogsCleanup__` | `0 */6 * * *` | Cleans up old log entries | Every 6 hours |
| `__pbDBOptimize__` | `0 0 * * *` | Optimizes database | Daily at midnight |
| `__pbMFACleanup__` | `0 * * * *` | Cleans up expired MFA records | Every hour |
| `__pbOTPCleanup__` | `0 * * * *` | Cleans up expired OTP codes | Every hour |

## Run Cron Job

Manually trigger a cron job to execute immediately.

```c
// Run a specific cron job
char* result_json = NULL;
if (bosbase_send(pb, "/api/crons/__pbLogsCleanup__", "POST", 
        "{}", "{}", "{}", 30000, NULL, 0, &result_json, NULL) == 0) {
    printf("Cron job executed: %s\n", result_json);
    bosbase_free_string(result_json);
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
    
    // List cron jobs
    char* crons = NULL;
    bosbase_send(pb, "/api/crons", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &crons, NULL);
    printf("Cron jobs: %s\n", crons);
    bosbase_free_string(crons);
    
    // Run a cron job
    char* result = NULL;
    bosbase_send(pb, "/api/crons/__pbLogsCleanup__", "POST", 
        "{}", "{}", "{}", 30000, NULL, 0, &result, NULL);
    printf("Result: %s\n", result);
    bosbase_free_string(result);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Logs API](./LOGS_API.md) - Viewing application logs
- [Health API](./HEALTH_API.md) - Server health checks

