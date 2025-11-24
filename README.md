BosBase C SDK
=============

Thin C11 wrapper over the feature-complete C++ core that mirrors the JavaScript SDK surface (auth, CRUD, realtime, pub/sub, vector, cache, files, GraphQL, etc.). The ABI is C-friendly while the heavy lifting is delegated to the existing `cpp-sdk` library, so the backend integration stays in sync with the Go service.

## Build

Dependencies: CMake 3.16+, a C11 compiler, and the libraries used by the C++ SDK (libcurl, OpenSSL, Boost, websocketpp headers). Build the C wrapper and the C++ core together:

```bash
cd c-sdk
cmake -S . -B build
cmake --build build
```

The build produces `libbosbase_c.a`/`libbosbase_c.so` plus the public header `include/bosbase_c.h`.

## Usage

```c
#include "bosbase_c.h"

void on_change(const char* evt_json, void* user) {
    printf("change: %s\n", evt_json);
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");

    char* auth_json = NULL;
    if (bosbase_collection_auth_with_password(pb, "_superusers", "admin@example.com", "password",
            "{}", NULL, NULL, "{}", "{}", &auth_json, NULL) == 0) {
        printf("auth: %s\n", auth_json);
        bosbase_free_string(auth_json);
    }

    char* list_json = NULL;
    bosbase_collection_get_list(pb, "articles", 1, 20, NULL, NULL, NULL, NULL, "{}", "{}", &list_json, NULL);
    printf("list: %s\n", list_json);
    bosbase_free_string(list_json);

    bosbase_subscription* sub = bosbase_collection_subscribe(pb, "articles", "*", on_change, NULL, "{}", "{}", NULL);
    /* ... later ... */
    bosbase_subscription_cancel(sub);

    bosbase_client_free(pb);
    return 0;
}
```

- Every function returns `0` on success or `-1` on failure. When an error occurs, inspect the filled `bosbase_error` (remember to free it with `bosbase_free_error`).
- All JSON parameters are plain UTF-8 strings. Unused inputs can be `NULL` to keep defaults.
- Use `bosbase_send` for endpoints that are not covered by the convenience helpers; it accepts path, method, JSON body/query/headers, timeout, and optional file attachments.
- Returned strings must be released with `bosbase_free_string`.

## Covered surface

- Record/collection CRUD and auth helpers (password flows, verification, password reset)
- Realtime subscriptions on collections
- File URLs/tokens
- Health and GraphQL
- Vector endpoints (collections, documents, search)
- Cache endpoints
- Pub/Sub publish/subscribe
- Generic `bosbase_send` for any other API route (backups, crons, settings, langchaingo, etc.)

Because the wrapper is built directly on top of the C++ SDK, any new backend feature added there is immediately reachable from C via `bosbase_send` or by extending the thin helper set in `bosbase_c.h`.
