# Authentication - C SDK Documentation

## Overview

Authentication in BosBase is stateless and token-based. A client is considered authenticated as long as it sends a valid `Authorization: YOUR_AUTH_TOKEN` header with requests.

**Key Points:**
- **No sessions**: BosBase APIs are fully stateless (tokens are not stored in the database)
- **No logout endpoint**: To "logout", simply clear the token from your local state (`bosbase_auth_clear()`)
- **Token generation**: Auth tokens are generated through auth collection Web APIs or programmatically
- **Admin users**: `_superusers` collection works like regular auth collections but with full access (API rules are ignored)
- **OAuth2 limitation**: OAuth2 is not supported for `_superusers` collection

## Authentication Methods

BosBase supports multiple authentication methods that can be configured individually for each auth collection:

1. **Password Authentication** - Email/username + password
2. **OTP Authentication** - One-time password via email
3. **OAuth2 Authentication** - Google, GitHub, Microsoft, etc.
4. **Multi-factor Authentication (MFA)** - Requires 2 different auth methods

## Authentication Store

The SDK maintains an authentication store that automatically manages the authentication state:

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Check authentication status
    const char* token = bosbase_auth_token(pb);
    if (token) {
        printf("Current token: %s\n", token);
    }
    
    char* record_json = bosbase_auth_record(pb);
    if (record_json) {
        printf("Authenticated user: %s\n", record_json);
        bosbase_free_string(record_json);
    }
    
    // Clear authentication (logout)
    bosbase_auth_clear(pb);
    
    bosbase_client_free(pb);
    return 0;
}
```

## Password Authentication

Authenticate using email/username and password. The identity field can be configured in the collection options (default is email).

**Backend Endpoint:** `POST /api/collections/{collection}/auth-with-password`

### Basic Usage

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Authenticate with email and password
    char* auth_json = NULL;
    bosbase_error* err = NULL;
    
    if (bosbase_collection_auth_with_password(pb, "users", 
            "test@example.com", "password123", "{}", NULL, NULL, 
            "{}", "{}", &auth_json, &err) == 0) {
        printf("Authentication successful: %s\n", auth_json);
        
        // Token is automatically stored
        const char* token = bosbase_auth_token(pb);
        printf("Token: %s\n", token);
        
        bosbase_free_string(auth_json);
    } else {
        fprintf(stderr, "Authentication failed: %s\n", err->message);
        bosbase_free_error(err);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

### Response Format

The authentication response is a JSON object:

```json
{
  "token": "eyJhbGciOiJIUzI1NiJ9...",
  "record": {
    "id": "record_id",
    "email": "test@example.com",
    ...
  }
}
```

### Error Handling with MFA

```c
char* auth_json = NULL;
bosbase_error* err = NULL;

if (bosbase_collection_auth_with_password(pb, "users", 
        "test@example.com", "pass123", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, &err) != 0) {
    if (err && err->status == 401) {
        // Check for MFA requirement (parse err->response JSON)
        // Handle MFA flow (see Multi-factor Authentication section)
        printf("MFA required\n");
    } else {
        fprintf(stderr, "Authentication failed: %s\n", err->message);
    }
    bosbase_free_error(err);
} else {
    bosbase_free_string(auth_json);
}
```

## OTP Authentication

One-time password authentication via email.

**Backend Endpoints:**
- `POST /api/collections/{collection}/request-otp` - Request OTP
- `POST /api/collections/{collection}/auth-with-otp` - Authenticate with OTP

### Request OTP

```c
// Send OTP to user's email
const char* otp_body = "{\"email\":\"test@example.com\"}";
char* otp_result = NULL;
bosbase_error* err = NULL;

if (bosbase_send(pb, "/api/collections/users/request-otp", "POST", 
        otp_body, "{}", "{}", 30000, NULL, 0, &otp_result, &err) == 0) {
    // Parse otp_result JSON to get otpId
    printf("OTP requested: %s\n", otp_result);
    bosbase_free_string(otp_result);
} else {
    fprintf(stderr, "OTP request failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

### Authenticate with OTP

```c
// Step 1: Request OTP (see above)

// Step 2: User enters OTP from email
const char* auth_otp_body = 
    "{\"otpId\":\"OTP_ID_FROM_STEP_1\",\"otp\":\"123456\"}";

char* auth_json = NULL;
if (bosbase_send(pb, "/api/collections/users/auth-with-otp", "POST", 
        auth_otp_body, "{}", "{}", 30000, NULL, 0, &auth_json, &err) == 0) {
    printf("OTP authentication successful: %s\n", auth_json);
    bosbase_free_string(auth_json);
} else {
    fprintf(stderr, "OTP authentication failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## OAuth2 Authentication

**Backend Endpoint:** `POST /api/collections/{collection}/auth-with-oauth2`

### Manual Code Exchange

```c
// Get auth methods first
char* methods_json = NULL;
if (bosbase_send(pb, "/api/collections/users/auth-methods", "GET", 
        NULL, "{}", "{}", 30000, NULL, 0, &methods_json, NULL) == 0) {
    // Parse methods_json to find OAuth2 provider
    // Extract provider name, codeVerifier, etc.
    bosbase_free_string(methods_json);
}

// Exchange code for token (after OAuth2 redirect)
const char* oauth2_body = 
    "{\"provider\":\"google\","
    "\"code\":\"AUTHORIZATION_CODE\","
    "\"codeVerifier\":\"CODE_VERIFIER\","
    "\"redirectUrl\":\"https://yourapp.com/callback\"}";

char* oauth2_json = NULL;
if (bosbase_send(pb, "/api/collections/users/auth-with-oauth2", "POST", 
        oauth2_body, "{}", "{}", 30000, NULL, 0, &oauth2_json, &err) == 0) {
    printf("OAuth2 authentication successful: %s\n", oauth2_json);
    bosbase_free_string(oauth2_json);
} else {
    fprintf(stderr, "OAuth2 failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Multi-Factor Authentication (MFA)

Requires 2 different auth methods.

```c
char* auth_json = NULL;
bosbase_error* err = NULL;

// First auth method (password)
if (bosbase_collection_auth_with_password(pb, "users", 
        "test@example.com", "pass123", "{}", NULL, NULL, 
        "{}", "{}", &auth_json, &err) != 0) {
    if (err && err->status == 401) {
        // Parse err->response to check for mfaId
        // If mfaId present, proceed with second factor
        
        // Second auth method (OTP)
        const char* otp_body = "{\"email\":\"test@example.com\"}";
        char* otp_result = NULL;
        if (bosbase_send(pb, "/api/collections/users/request-otp", "POST", 
                otp_body, "{}", "{}", 30000, NULL, 0, &otp_result, NULL) == 0) {
            // Parse otp_result to get otpId
            // Then authenticate with OTP and mfaId
            const char* mfa_otp_body = 
                "{\"otpId\":\"OTP_ID\",\"otp\":\"123456\","
                "\"mfaId\":\"MFA_ID_FROM_FIRST_AUTH\"}";
            
            char* mfa_auth_json = NULL;
            bosbase_send(pb, "/api/collections/users/auth-with-otp", "POST", 
                    mfa_otp_body, "{}", "{}", 30000, NULL, 0, 
                    &mfa_auth_json, NULL);
            bosbase_free_string(mfa_auth_json);
        }
        bosbase_free_string(otp_result);
    }
    bosbase_free_error(err);
} else {
    bosbase_free_string(auth_json);
}
```

## Auth Token Verification

Verify token by calling `authRefresh()`.

**Backend Endpoint:** `POST /api/collections/{collection}/auth-refresh`

```c
char* refresh_json = NULL;
bosbase_error* err = NULL;

if (bosbase_collection_auth_refresh(pb, "users", "{}", NULL, NULL, 
        "{}", "{}", &refresh_json, &err) == 0) {
    printf("Token is valid: %s\n", refresh_json);
    bosbase_free_string(refresh_json);
} else {
    fprintf(stderr, "Token verification failed: %s\n", err->message);
    bosbase_auth_clear(pb);  // Clear invalid token
    bosbase_free_error(err);
}
```

## List Available Auth Methods

**Backend Endpoint:** `GET /api/collections/{collection}/auth-methods`

```c
char* methods_json = NULL;
if (bosbase_send(pb, "/api/collections/users/auth-methods", "GET", 
        NULL, "{}", "{}", 30000, NULL, 0, &methods_json, NULL) == 0) {
    // Parse JSON to check:
    // - methods.password.enabled
    // - methods.oauth2.providers
    // - methods.mfa.enabled
    printf("Auth methods: %s\n", methods_json);
    bosbase_free_string(methods_json);
}
```

## Password Reset

```c
// Request password reset email
const char* reset_request = "{\"email\":\"user@example.com\"}";
bosbase_error* err = NULL;

if (bosbase_collection_request_password_reset(pb, "users", 
        "user@example.com", "{}", "{}", "{}", &err) != 0) {
    fprintf(stderr, "Reset request failed: %s\n", err->message);
    bosbase_free_error(err);
}

// Confirm password reset (on reset page)
if (bosbase_collection_confirm_password_reset(pb, "users", 
        "RESET_TOKEN", "newpassword123", "newpassword123", 
        "{}", "{}", &err) != 0) {
    fprintf(stderr, "Reset confirmation failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Email Verification

```c
// Request verification email
if (bosbase_collection_request_verification(pb, "users", 
        "user@example.com", "{}", "{}", "{}", &err) != 0) {
    fprintf(stderr, "Verification request failed: %s\n", err->message);
    bosbase_free_error(err);
}

// Confirm verification (on verification page)
if (bosbase_collection_confirm_verification(pb, "users", 
        "VERIFICATION_TOKEN", "{}", "{}", "{}", &err) != 0) {
    fprintf(stderr, "Verification failed: %s\n", err->message);
    bosbase_free_error(err);
}
```

## Saving and Loading Auth Tokens

```c
// Save auth token and record
const char* token = "eyJhbGciOiJIUzI1NiJ9...";
const char* record_json = "{\"id\":\"user_id\",\"email\":\"user@example.com\"}";

if (bosbase_auth_save(pb, token, record_json, &err) == 0) {
    printf("Auth saved successfully\n");
} else {
    fprintf(stderr, "Failed to save auth: %s\n", err->message);
    bosbase_free_error(err);
}

// Later, retrieve saved auth
const char* saved_token = bosbase_auth_token(pb);
char* saved_record = bosbase_auth_record(pb);
if (saved_token && saved_record) {
    printf("Restored auth: %s\n", saved_record);
    bosbase_free_string(saved_record);
}
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <stdlib.h>

int authenticate_user(bosbase_client* pb, const char* email, const char* password) {
    char* auth_json = NULL;
    bosbase_error* err = NULL;
    
    // Try password authentication
    if (bosbase_collection_auth_with_password(pb, "users", email, password, 
            "{}", NULL, NULL, "{}", "{}", &auth_json, &err) == 0) {
        printf("Successfully authenticated: %s\n", email);
        bosbase_free_string(auth_json);
        return 0;
    }
    
    // Check for MFA requirement
    if (err && err->status == 401) {
        // Parse err->response to check for mfaId
        // Handle MFA flow...
        printf("MFA required\n");
    } else {
        fprintf(stderr, "Authentication failed: %s\n", err->message);
    }
    
    bosbase_free_error(err);
    return 1;
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    if (authenticate_user(pb, "user@example.com", "password123") == 0) {
        // User is authenticated, make authenticated requests
        const char* token = bosbase_auth_token(pb);
        printf("Using token: %s\n", token);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

## Best Practices

1. **Secure Token Storage**: Never expose tokens in logs or error messages
2. **Token Refresh**: Implement automatic token refresh before expiration
3. **Error Handling**: Always handle MFA requirements and token expiration
4. **OAuth2 Security**: Always validate the `state` parameter in OAuth2 callbacks
5. **Memory Management**: Always free strings returned by SDK functions
6. **Error Cleanup**: Always free error objects after handling

## Related Documentation

- [Collections](./COLLECTIONS.md)
- [API Rules](./API_RULES_AND_FILTERS.md)
- [API Records](./API_RECORDS.md)

