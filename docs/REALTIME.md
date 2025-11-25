# Realtime API - C SDK Documentation

## Overview

The Realtime API enables real-time updates for collection records using **Server-Sent Events (SSE)**. It allows you to subscribe to changes in collections or specific records and receive instant notifications when records are created, updated, or deleted.

**Key Features:**
- Real-time notifications for record changes
- Collection-level and record-level subscriptions
- Automatic connection management and reconnection
- Authorization support
- Subscription options (expand, custom headers, query params)
- Event-driven architecture

**Backend Endpoints:**
- `GET /api/realtime` - Establish SSE connection
- `POST /api/realtime` - Set subscriptions

## How It Works

1. **Connection**: The SDK establishes an SSE connection to `/api/realtime`
2. **Client ID**: Server sends `PB_CONNECT` event with a unique `clientId`
3. **Subscriptions**: Client submits subscription topics via POST request
4. **Events**: Server sends events when matching records change
5. **Reconnection**: SDK automatically reconnects on connection loss

## Basic Usage

### Subscribe to Collection Changes

Subscribe to all changes in a collection:

```c
#include "bosbase_c.h"
#include <stdio.h>

void on_record_event(const char* json_event, void* user_data) {
    printf("Event received: %s\n", json_event);
    // Parse json_event to get:
    // - action: "create", "update", or "delete"
    // - record: the record data
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Subscribe to all changes in the 'posts' collection
    bosbase_error* err = NULL;
    bosbase_subscription* sub = bosbase_collection_subscribe(
        pb, "posts", "*", on_record_event, NULL, "{}", "{}", &err);
    
    if (!sub && err) {
        fprintf(stderr, "Subscription failed: %s\n", err->message);
        bosbase_free_error(err);
        return 1;
    }
    
    // Keep program running to receive events
    // In a real application, you'd have an event loop here
    
    // Later, unsubscribe
    bosbase_subscription_cancel(sub);
    
    bosbase_client_free(pb);
    return 0;
}
```

### Subscribe to Specific Record

Subscribe to changes for a single record:

```c
void on_record_change(const char* json_event, void* user_data) {
    printf("Record changed: %s\n", json_event);
}

bosbase_subscription* sub = bosbase_collection_subscribe(
    pb, "posts", "RECORD_ID", on_record_change, NULL, "{}", "{}", NULL);
```

### Multiple Subscriptions

You can subscribe multiple times to the same or different topics:

```c
void handle_change(const char* json_event, void* user_data) {
    printf("Change event: %s\n", json_event);
}

void handle_all_changes(const char* json_event, void* user_data) {
    printf("Collection-wide change: %s\n", json_event);
}

// Subscribe to multiple records
bosbase_subscription* sub1 = bosbase_collection_subscribe(
    pb, "posts", "RECORD_ID_1", handle_change, NULL, "{}", "{}", NULL);
bosbase_subscription* sub2 = bosbase_collection_subscribe(
    pb, "posts", "RECORD_ID_2", handle_change, NULL, "{}", "{}", NULL);
bosbase_subscription* sub3 = bosbase_collection_subscribe(
    pb, "posts", "*", handle_all_changes, NULL, "{}", "{}", NULL);

// Unsubscribe individually
bosbase_subscription_cancel(sub1);
bosbase_subscription_cancel(sub2);
bosbase_subscription_cancel(sub3);
```

## Event Structure

Each event received contains:

```json
{
  "action": "create|update|delete",
  "record": {
    "id": "RECORD_ID",
    "collectionId": "COLLECTION_ID",
    "collectionName": "collection_name",
    "created": "2023-01-01T00:00:00.000Z",
    "updated": "2023-01-01T00:00:00.000Z",
    ...
  }
}
```

### PB_CONNECT Event

When the connection is established, you receive a `PB_CONNECT` event:

```c
void on_connect(const char* json_event, void* user_data) {
    // Parse json_event to get clientId
    printf("Connected! Event: %s\n", json_event);
    // e.clientId - unique client identifier
}
```

## Subscription Topics

### Collection-Level Subscription

Subscribe to all changes in a collection:

```c
// Wildcard subscription - all records in collection
bosbase_subscription* sub = bosbase_collection_subscribe(
    pb, "posts", "*", handler, NULL, "{}", "{}", NULL);
```

**Access Control**: Uses the collection's `ListRule` to determine if the subscriber has access to receive events.

### Record-Level Subscription

Subscribe to changes for a specific record:

```c
// Specific record subscription
bosbase_subscription* sub = bosbase_collection_subscribe(
    pb, "posts", "RECORD_ID", handler, NULL, "{}", "{}", NULL);
```

**Access Control**: Uses the collection's `ViewRule` to determine if the subscriber has access to receive events.

## Subscription Options

You can pass additional options via query_json:

```c
// With expand relations
const char* query = "{\"expand\":\"author,categories\"}";
bosbase_subscription* sub = bosbase_collection_subscribe(
    pb, "posts", "RECORD_ID", handler, NULL, query, "{}", NULL);

// With filter
const char* query2 = "{\"filter\":\"status = \\\"published\\\"\"}";
bosbase_subscription* sub2 = bosbase_collection_subscribe(
    pb, "posts", "*", handler, NULL, query2, "{}", NULL);
```

### Expand Relations

Expand relations in the event data:

```c
const char* query = "{\"expand\":\"author,categories\"}";
bosbase_subscription* sub = bosbase_collection_subscribe(
    pb, "posts", "RECORD_ID", handler, NULL, query, "{}", NULL);

// In handler, parse json_event to access:
// - record.expand.author
// - record.expand.categories
```

### Filter with Query Parameters

Use query parameters for API rule filtering:

```c
const char* query = "{\"filter\":\"status = \\\"published\\\"\"}";
bosbase_subscription* sub = bosbase_collection_subscribe(
    pb, "posts", "*", handler, NULL, query, "{}", NULL);
```

## Unsubscribing

### Unsubscribe from Specific Topic

```c
// Cancel subscription
bosbase_subscription_cancel(sub);
```

## User Data in Callbacks

You can pass user data to callbacks:

```c
typedef struct {
    int count;
    char* collection_name;
} subscription_context;

void on_event(const char* json_event, void* user_data) {
    subscription_context* ctx = (subscription_context*)user_data;
    ctx->count++;
    printf("Event %d in %s: %s\n", ctx->count, ctx->collection_name, json_event);
}

int main() {
    subscription_context ctx = {.count = 0, .collection_name = "posts"};
    
    bosbase_subscription* sub = bosbase_collection_subscribe(
        pb, "posts", "*", on_event, &ctx, "{}", "{}", NULL);
    
    // ctx will be passed to on_event callback
    
    bosbase_subscription_cancel(sub);
    return 0;
}
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <unistd.h>

typedef struct {
    int event_count;
} app_state;

void event_handler(const char* json_event, void* user_data) {
    app_state* state = (app_state*)user_data;
    state->event_count++;
    
    printf("Event #%d: %s\n", state->event_count, json_event);
    // Parse json_event to handle different action types
    // - "create": new record created
    // - "update": record updated
    // - "delete": record deleted
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate if needed
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "users", 
        "user@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    app_state state = {.event_count = 0};
    
    // Subscribe to collection changes
    bosbase_subscription* sub = bosbase_collection_subscribe(
        pb, "posts", "*", event_handler, &state, "{}", "{}", NULL);
    
    if (!sub) {
        fprintf(stderr, "Failed to subscribe\n");
        bosbase_client_free(pb);
        return 1;
    }
    
    printf("Subscribed to posts collection. Waiting for events...\n");
    
    // Keep running to receive events
    // In a real application, use an event loop (e.g., libevent, libuv)
    sleep(60);  // Example: wait 60 seconds
    
    // Cleanup
    bosbase_subscription_cancel(sub);
    bosbase_client_free(pb);
    
    printf("Received %d events\n", state.event_count);
    return 0;
}
```

## Error Handling

```c
bosbase_error* err = NULL;
bosbase_subscription* sub = bosbase_collection_subscribe(
    pb, "posts", "*", handler, NULL, "{}", "{}", &err);

if (!sub) {
    if (err) {
        fprintf(stderr, "Subscription error %d: %s\n", err->status, err->message);
        bosbase_free_error(err);
    }
    return 1;
}
```

## Best Practices

1. **Connection Management**: The SDK handles reconnection automatically
2. **Memory Management**: Always cancel subscriptions before freeing the client
3. **Error Handling**: Check for errors when subscribing
4. **User Data**: Use user_data parameter to pass context to callbacks
5. **Event Parsing**: Parse JSON events efficiently (consider using a JSON library)
6. **Multiple Subscriptions**: Manage multiple subscriptions carefully

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection configuration
- [API Records](./API_RECORDS.md) - Record operations
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Access control

