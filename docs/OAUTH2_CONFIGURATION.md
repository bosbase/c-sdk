# OAuth2 Configuration Guide - C SDK

This guide explains how to configure OAuth2 authentication providers for auth collections using the BosBase C SDK.

## Overview

OAuth2 allows users to authenticate with your application using third-party providers like Google, GitHub, Facebook, etc. Before you can use OAuth2 authentication, you need to:

1. **Create an OAuth2 app** in the provider's dashboard
2. **Obtain Client ID and Client Secret** from the provider
3. **Register a redirect URL** (typically: `https://yourdomain.com/api/oauth2-redirect`)
4. **Configure the provider** in your BosBase auth collection using the SDK

## Prerequisites

- An auth collection in your BosBase instance
- OAuth2 app credentials (Client ID and Client Secret) from your chosen provider
- Admin/superuser authentication to configure collections

## Supported Providers

The following OAuth2 providers are supported:

- **google** - Google OAuth2
- **github** - GitHub OAuth2
- **gitlab** - GitLab OAuth2
- **discord** - Discord OAuth2
- **facebook** - Facebook OAuth2
- **microsoft** - Microsoft OAuth2
- **apple** - Apple Sign In
- **twitter** - Twitter OAuth2
- **spotify** - Spotify OAuth2
- **kakao** - Kakao OAuth2
- **twitch** - Twitch OAuth2
- **strava** - Strava OAuth2
- **vk** - VK OAuth2
- **yandex** - Yandex OAuth2
- **patreon** - Patreon OAuth2
- **linkedin** - LinkedIn OAuth2
- **instagram** - Instagram OAuth2
- **vimeo** - Vimeo OAuth2
- **digitalocean** - DigitalOcean OAuth2
- **bitbucket** - Bitbucket OAuth2
- **dropbox** - Dropbox OAuth2
- **planningcenter** - Planning Center OAuth2
- **notion** - Notion OAuth2
- **linear** - Linear OAuth2
- **oidc**, **oidc2**, **oidc3** - OpenID Connect (OIDC) providers

## Basic Usage

### 1. Enable OAuth2 for a Collection

First, enable OAuth2 authentication for your auth collection:

```c
#include "bosbase_c.h"

bosbase_client* pb = bosbase_client_new("https://your-instance.com", "en-US");

// Authenticate as admin
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "_superusers", 
    "admin@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Get collection
char* collection_json = NULL;
bosbase_send(pb, "/api/collections/users", "GET", NULL, 
    "{}", "{}", 30000, NULL, 0, &collection_json, NULL);

// Parse JSON, enable OAuth2, then update
// Enable OAuth2 in collection options
// bosbase_send(pb, "/api/collections/users", "PATCH", updated_json, ...);

bosbase_free_string(collection_json);
```

### 2. Add an OAuth2 Provider

Add a provider configuration to your collection:

```c
// Get collection first
char* collection_json = NULL;
bosbase_send(pb, "/api/collections/users", "GET", NULL, 
    "{}", "{}", 30000, NULL, 0, &collection_json, NULL);

// Parse JSON, add OAuth2 provider configuration, then update
// Provider configuration example:
// {
//   "name": "google",
//   "clientId": "your-google-client-id",
//   "clientSecret": "your-google-client-secret",
//   "authURL": "https://accounts.google.com/o/oauth2/v2/auth",
//   "tokenURL": "https://oauth2.googleapis.com/token",
//   "userInfoURL": "https://www.googleapis.com/oauth2/v2/userinfo",
//   "displayName": "Google",
//   "pkce": true
// }

// Update collection with OAuth2 provider
// bosbase_send(pb, "/api/collections/users", "PATCH", updated_json, ...);

bosbase_free_string(collection_json);
```

### 3. Get OAuth2 Configuration

Retrieve the current OAuth2 configuration:

```c
char* collection_json = NULL;
if (bosbase_send(pb, "/api/collections/users", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &collection_json, NULL) == 0) {
    // Parse collection_json to access OAuth2 provider configurations
    printf("Collection: %s\n", collection_json);
    bosbase_free_string(collection_json);
}
```

## Using OAuth2 Authentication

After configuration, users can authenticate using OAuth2:

```c
// Get auth methods to see available OAuth2 providers
char* methods_json = NULL;
if (bosbase_send(pb, "/api/collections/users/auth-methods", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &methods_json, NULL) == 0) {
    // Parse methods_json to get OAuth2 providers
    printf("Auth methods: %s\n", methods_json);
    bosbase_free_string(methods_json);
}

// Authenticate with OAuth2 (after user completes OAuth2 flow)
const char* oauth2_body = 
    "{"
    "\"provider\":\"google\","
    "\"code\":\"AUTHORIZATION_CODE\","
    "\"codeVerifier\":\"CODE_VERIFIER\","
    "\"redirectUrl\":\"https://yourapp.com/callback\""
    "}";

char* oauth2_json = NULL;
if (bosbase_send(pb, "/api/collections/users/auth-with-oauth2", "POST", 
        oauth2_body, "{}", "{}", 30000, NULL, 0, 
        &oauth2_json, NULL) == 0) {
    printf("OAuth2 authentication: %s\n", oauth2_json);
    bosbase_free_string(oauth2_json);
}
```

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

int configure_oauth2(bosbase_client* pb) {
    // Authenticate as superuser
    char* auth_json = NULL;
    if (bosbase_collection_auth_with_password(pb, "_superusers", 
            "admin@example.com", "password", "{}", NULL, NULL, 
            "{}", "{}", &auth_json, NULL) != 0) {
        return 1;
    }
    bosbase_free_string(auth_json);
    
    // Get collection
    char* collection_json = NULL;
    if (bosbase_send(pb, "/api/collections/users", "GET", NULL, 
            "{}", "{}", 30000, NULL, 0, &collection_json, NULL) != 0) {
        return 1;
    }
    
    // Parse JSON, add OAuth2 provider, update collection
    // (In practice, use a JSON library to modify the collection)
    
    bosbase_free_string(collection_json);
    return 0;
}

int main() {
    bosbase_client* pb = bosbase_client_new("https://your-instance.com", "en-US");
    
    if (configure_oauth2(pb) == 0) {
        printf("OAuth2 configured successfully\n");
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Authentication](./AUTHENTICATION.md) - Authentication guide
- [Collections](./COLLECTIONS.md) - Collection management

