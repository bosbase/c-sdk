# File API - C SDK Documentation

## Overview

The File API provides endpoints for downloading and accessing files stored in collection records. It supports thumbnail generation for images, protected file access with tokens, and force download options.

**Key Features:**
- Download files from collection records
- Generate thumbnails for images (crop, fit, resize)
- Protected file access with short-lived tokens
- Force download option for any file type
- Automatic content-type detection
- Support for Range requests and caching

**Backend Endpoints:**
- `GET /api/files/{collection}/{recordId}/{filename}` - Download/fetch file
- `POST /api/files/token` - Generate protected file token

## Download / Fetch File

Downloads a single file resource from a record.

### Basic Usage

```c
#include "bosbase_c.h"
#include <stdio.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    // Get a record with a file field
    char* record_json = NULL;
    if (bosbase_collection_get_one(pb, "posts", "RECORD_ID", 
            NULL, NULL, "{}", "{}", &record_json, NULL) == 0) {
        // Parse record_json to get filename
        const char* filename = "photo.jpg";  // From parsed record
        
        // Get the file URL
        char* file_url = NULL;
        if (bosbase_files_get_url(pb, record_json, filename, NULL, NULL, 0, 
                "{}", &file_url, NULL) == 0) {
            printf("File URL: %s\n", file_url);
            // Use the URL to download or display the file
            bosbase_free_string(file_url);
        }
        bosbase_free_string(record_json);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

### File URL Structure

The file URL follows this pattern:
```
/api/files/{collectionIdOrName}/{recordId}/{filename}
```

Example:
```
http://127.0.0.1:8090/api/files/posts/abc123/photo_xyz789.jpg
```

## Thumbnails

Generate thumbnails for image files on-the-fly.

### Thumbnail Formats

The following thumbnail formats are supported:

| Format | Example | Description |
|--------|---------|-------------|
| `WxH` | `100x300` | Crop to WxH viewbox (from center) |
| `WxHt` | `100x300t` | Crop to WxH viewbox (from top) |
| `WxHb` | `100x300b` | Crop to WxH viewbox (from bottom) |
| `WxHf` | `100x300f` | Fit inside WxH viewbox (without cropping) |
| `0xH` | `0x300` | Resize to H height preserving aspect ratio |
| `Wx0` | `100x0` | Resize to W width preserving aspect ratio |

### Using Thumbnails

```c
char* record_json = NULL;
bosbase_collection_get_one(pb, "posts", "RECORD_ID", 
    NULL, NULL, "{}", "{}", &record_json, NULL);

const char* filename = "photo.jpg";

// Get thumbnail URL
char* thumb_url = NULL;
if (bosbase_files_get_url(pb, record_json, filename, "100x100", 
        NULL, 0, "{}", &thumb_url, NULL) == 0) {
    printf("Thumbnail: %s\n", thumb_url);
    bosbase_free_string(thumb_url);
}

// Different thumbnail sizes
char* small_thumb = NULL;
bosbase_files_get_url(pb, record_json, filename, "50x50", 
    NULL, 0, "{}", &small_thumb, NULL);
bosbase_free_string(small_thumb);

char* medium_thumb = NULL;
bosbase_files_get_url(pb, record_json, filename, "200x200", 
    NULL, 0, "{}", &medium_thumb, NULL);
bosbase_free_string(medium_thumb);

char* fit_thumb = NULL;
bosbase_files_get_url(pb, record_json, filename, "200x200f", 
    NULL, 0, "{}", &fit_thumb, NULL);
bosbase_free_string(fit_thumb);

// Resize to specific width
char* width_thumb = NULL;
bosbase_files_get_url(pb, record_json, filename, "300x0", 
    NULL, 0, "{}", &width_thumb, NULL);
bosbase_free_string(width_thumb);

// Resize to specific height
char* height_thumb = NULL;
bosbase_files_get_url(pb, record_json, filename, "0x200", 
    NULL, 0, "{}", &height_thumb, NULL);
bosbase_free_string(height_thumb);

bosbase_free_string(record_json);
```

### Thumbnail Behavior

- **Image Files Only**: Thumbnails are only generated for image files (PNG, JPG, JPEG, GIF, WEBP)
- **Non-Image Files**: For non-image files, the thumb parameter is ignored and the original file is returned
- **Caching**: Thumbnails are cached and reused if already generated
- **Fallback**: If thumbnail generation fails, the original file is returned

## Force Download

To force browser download instead of preview:

```c
char* url = NULL;
if (bosbase_files_get_url(pb, record_json, filename, NULL, NULL, 1, 
        "{}", &url, NULL) == 0) {
    // download=1 forces download
    printf("Download URL: %s\n", url);
    bosbase_free_string(url);
}
```

## Protected Files

Protected files require authentication and a file token for access.

### Get File Token

```c
// Authenticate first
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "users", 
    "user@example.com", "password", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Get file token (valid for ~2 minutes)
char* token_json = NULL;
if (bosbase_files_get_token(pb, "{}", "{}", &token_json, NULL) == 0) {
    // Parse token_json to get token string
    printf("File token: %s\n", token_json);
    bosbase_free_string(token_json);
}
```

### Access Protected File

```c
// Get record
char* record_json = NULL;
bosbase_collection_get_one(pb, "documents", "RECORD_ID", 
    NULL, NULL, "{}", "{}", &record_json, NULL);

// Get token
char* token_json = NULL;
bosbase_files_get_token(pb, "{}", "{}", &token_json, NULL);
// Parse to get token string
const char* token = "FILE_TOKEN";  // From parsed token_json

// Get protected file URL with token
char* protected_url = NULL;
if (bosbase_files_get_url(pb, record_json, "privateDocument", 
        NULL, token, 0, "{}", &protected_url, NULL) == 0) {
    printf("Protected file URL: %s\n", protected_url);
    bosbase_free_string(protected_url);
}

bosbase_free_string(record_json);
bosbase_free_string(token_json);
```

**Important:**
- File tokens are short-lived (~2 minutes)
- Only authenticated users satisfying the collection's `viewRule` can access protected files
- Tokens must be regenerated when they expire

## Complete Example

```c
#include "bosbase_c.h"
#include <stdio.h>

void display_image_thumbnails(bosbase_client* pb, 
                               const char* collection,
                               const char* record_id,
                               const char* filename) {
    // Get record
    char* record_json = NULL;
    if (bosbase_collection_get_one(pb, collection, record_id, 
            NULL, NULL, "{}", "{}", &record_json, NULL) == 0) {
        
        // Get different thumbnail sizes
        const char* sizes[] = {"100x100", "200x200", "300x300f", "800x600"};
        int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
        
        for (int i = 0; i < num_sizes; i++) {
            char* thumb_url = NULL;
            if (bosbase_files_get_url(pb, record_json, filename, sizes[i], 
                    NULL, 0, "{}", &thumb_url, NULL) == 0) {
                printf("Thumbnail %s: %s\n", sizes[i], thumb_url);
                bosbase_free_string(thumb_url);
            }
        }
        
        bosbase_free_string(record_json);
    }
}

int main() {
    bosbase_client* pb = bosbase_client_new("http://127.0.0.1:8090", "en-US");
    
    display_image_thumbnails(pb, "products", "PRODUCT_ID", "image.jpg");
    
    bosbase_client_free(pb);
    return 0;
}
```

## Related Documentation

- [Files](./FILES.md) - File upload and management
- [Authentication](./AUTHENTICATION.md) - Required for protected files
- [Collections](./COLLECTIONS.md) - Collection configuration

