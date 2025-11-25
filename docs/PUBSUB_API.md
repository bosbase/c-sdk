# Pub/Sub API - C SDK Documentation

## Overview

BosBase exposes a lightweight WebSocket-based publish/subscribe channel so SDK users can push and receive custom messages. The Go backend uses the `ws` transport and persists each published payload in the `_pubsub_messages` table so every node in a cluster can replay and fan-out messages to its local subscribers.

- Endpoint: `/api/pubsub` (WebSocket)
- Auth: the SDK automatically forwards auth token as a `token` query parameter; cookie-based auth also works. Anonymous clients may subscribe, but publishing requires an authenticated token.
- Reliability: automatic reconnect with topic re-subscription; messages are stored in the database and broadcasted to all connected nodes.

## Quick Start

```c
#include "bosbase_c.h"
#include <stdio.h>

void on_message(const char* topic, const char* message_id, 
                const char* created, const char* data_json, 
                void* user_data) {
    printf("Message on %s: %s\n", topic, data_json);
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate (required for publishing)
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "users", 
        "user@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // Subscribe to a topic
    bosbase_error* err = NULL;
    bosbase_subscription* sub = bosbase_pubsub_subscribe(
        pb, "chat/general", on_message, NULL, &err);
    
    if (!sub && err) {
        fprintf(stderr, "Subscribe failed: %s\n", err->message);
        bosbase_free_error(err);
        return 1;
    }
    
    // Publish to a topic
    const char* message_data = "{\"text\":\"Hello team!\"}";
    char* publish_result = NULL;
    if (bosbase_pubsub_publish(pb, "chat/general", message_data, 
            &publish_result, NULL) == 0) {
        printf("Published: %s\n", publish_result);
        bosbase_free_string(publish_result);
    }
    
    // Later, unsubscribe
    bosbase_subscription_cancel(sub);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Publishing Messages

Publish a message to a topic. Resolves when the server stores and accepts it.

```c
const char* message = "{\"text\":\"Hello world!\"}";
char* result_json = NULL;
if (bosbase_pubsub_publish(pb, "chat/general", message, 
        &result_json, NULL) == 0) {
    // Parse result_json to get:
    // - id: message ID
    // - topic: topic name
    // - created: timestamp
    printf("Published: %s\n", result_json);
    bosbase_free_string(result_json);
}
```

## Subscribing to Topics

Subscribe to a topic and receive messages via callback.

```c
void message_handler(const char* topic, const char* message_id, 
                     const char* created, const char* data_json, 
                     void* user_data) {
    printf("Topic: %s\n", topic);
    printf("Message ID: %s\n", message_id);
    printf("Created: %s\n", created);
    printf("Data: %s\n", data_json);
}

bosbase_subscription* sub = bosbase_pubsub_subscribe(
    pb, "chat/general", message_handler, NULL, NULL);
```

## Unsubscribing

Cancel a subscription:

```c
bosbase_subscription_cancel(sub);
```

## Disconnect

Explicitly close the socket and clear pending requests:

```c
bosbase_pubsub_disconnect(pb);
```

## User Data in Callbacks

You can pass user data to callbacks:

```c
typedef struct {
    int message_count;
    char* user_name;
} pubsub_context;

void on_message(const char* topic, const char* message_id, 
                const char* created, const char* data_json, 
                void* user_data) {
    pubsub_context* ctx = (pubsub_context*)user_data;
    ctx->message_count++;
    printf("[%s] Message %d on %s: %s\n", 
           ctx->user_name, ctx->message_count, topic, data_json);
}

int main() {
    pubsub_context ctx = {.message_count = 0, .user_name = "User1"};
    
    bosbase_subscription* sub = bosbase_pubsub_subscribe(
        pb, "chat/general", on_message, &ctx, NULL);
    
    // ctx will be passed to on_message callback
    
    bosbase_subscription_cancel(sub);
    return 0;
}
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <unistd.h>

void chat_handler(const char* topic, const char* message_id, 
                  const char* created, const char* data_json, 
                  void* user_data) {
    printf("Chat message: %s\n", data_json);
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Authenticate
    char* auth_json = NULL;
    bosbase_collection_auth_with_password(pb, "users", 
        "user@example.com", "password", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, NULL);
    bosbase_free_string(auth_json);
    
    // Subscribe
    bosbase_subscription* sub = bosbase_pubsub_subscribe(
        pb, "chat/general", chat_handler, NULL, NULL);
    
    if (!sub) {
        fprintf(stderr, "Failed to subscribe\n");
        bosbase_client_free(pb);
        return 1;
    }
    
    // Publish a message
    const char* msg = "{\"text\":\"Hello from C SDK!\"}";
    char* result = NULL;
    bosbase_pubsub_publish(pb, "chat/general", msg, &result, NULL);
    bosbase_free_string(result);
    
    // Keep running to receive messages
    sleep(10);
    
    // Cleanup
    bosbase_subscription_cancel(sub);
    bosbase_pubsub_disconnect(pb);
    bosbase_client_free(pb);
    
    return 0;
}
```

## Notes for Clusters

- Messages are written to `_pubsub_messages` with a timestamp; every running node polls the table and pushes new rows to its connected WebSocket clients.
- Old pub/sub rows are cleaned up automatically after a day to keep the table small.
- If a node restarts, it resumes from the latest message and replays new rows as they are inserted, so connected clients on other nodes stay in sync.

## Related Documentation

- [Realtime](./REALTIME.md) - Real-time collection subscriptions
- [Authentication](./AUTHENTICATION.md) - User authentication

