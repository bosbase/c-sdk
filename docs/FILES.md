# Files Upload and Handling - C SDK Documentation

## Overview

BosBase allows you to upload and manage files through file fields in your collections. Files are stored with sanitized names and a random suffix for security (e.g., `test_52iwbgds7l.png`).

**Key Features:**
- Upload multiple files per field
- Maximum file size: ~8GB (2^53-1 bytes)
- Automatic filename sanitization and random suffix
- Image thumbnails support
- Protected files with token-based access
- File modifiers for append/prepend/delete operations

**Backend Endpoints:**
- `POST /api/files/token` - Get file access token for protected files
- `GET /api/files/{collection}/{recordId}/{filename}` - Download file

## File Field Configuration

Before uploading files, you must add a file field to your collection using the Collection API:

```c
// Get existing collection
char* collection_json = NULL;
if (bosbase_send(pb, "/api/collections/example", "GET", NULL, 
        "{}", "{}", 30000, NULL, 0, &collection_json, NULL) == 0) {
    // Parse JSON, add file field, then update
    // Field definition:
    // {
    //   "name": "documents",
    //   "type": "file",
    //   "maxSelect": 5,
    //   "maxSize": 5242880,
    //   "mimeTypes": ["image/jpeg", "image/png", "application/pdf"],
    //   "thumbs": ["100x100", "300x300"],
    //   "protected": false
    // }
    bosbase_free_string(collection_json);
}
```

## Uploading Files

### Basic Upload with Create

When creating a new record, you can upload files directly:

```c
#include "bosbase_c.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    bosbase_client* pb = bosbase_client_new("http://localhost:8090", "en-US");
    
    // Prepare file attachment
    uint8_t file_data[] = { /* file content */ };
    size_t file_size = sizeof(file_data);
    
    bosbase_file_attachment files[1] = {
        {
            .field = "documents",
            .filename = "file1.txt",
            .content_type = "text/plain",
            .data = file_data,
            .data_len = file_size
        }
    };
    
    const char* body = "{\"title\":\"Hello world!\"}";
    char* result_json = NULL;
    
    if (bosbase_collection_create(pb, "example", body, files, 1, 
            NULL, NULL, "{}", "{}", &result_json, NULL) == 0) {
        printf("Created with file: %s\n", result_json);
        bosbase_free_string(result_json);
    }
    
    bosbase_client_free(pb);
    return 0;
}
```

### Upload Multiple Files

```c
bosbase_file_attachment files[2] = {
    {
        .field = "documents",
        .filename = "file1.txt",
        .content_type = "text/plain",
        .data = data1,
        .data_len = size1
    },
    {
        .field = "documents",
        .filename = "file2.txt",
        .content_type = "text/plain",
        .data = data2,
        .data_len = size2
    }
};

const char* body = "{\"title\":\"Multiple files\"}";
char* result_json = NULL;
bosbase_collection_create(pb, "example", body, files, 2, 
    NULL, NULL, "{}", "{}", &result_json, NULL);
bosbase_free_string(result_json);
```

### Upload with Update

```c
// Update record and upload new files
bosbase_file_attachment new_file[1] = {
    {
        .field = "documents",
        .filename = "file3.txt",
        .content_type = "text/plain",
        .data = new_data,
        .data_len = new_size
    }
};

const char* update_body = "{\"title\":\"Updated title\"}";
char* updated_json = NULL;
bosbase_collection_update(pb, "example", "RECORD_ID", update_body, 
    new_file, 1, NULL, NULL, "{}", "{}", &updated_json, NULL);
bosbase_free_string(updated_json);
```

### Append Files (Using + Modifier)

For multiple file fields, use the `+` modifier to append files:

```c
// Append files to existing ones
const char* append_body = "{\"documents+\":\"file4.txt\"}";
bosbase_file_attachment append_file[1] = {
    {
        .field = "documents+",  // Note: field name includes modifier
        .filename = "file4.txt",
        .content_type = "text/plain",
        .data = append_data,
        .data_len = append_size
    }
};

char* append_json = NULL;
bosbase_collection_update(pb, "example", "RECORD_ID", append_body, 
    append_file, 1, NULL, NULL, "{}", "{}", &append_json, NULL);
bosbase_free_string(append_json);
```

## Deleting Files

### Delete All Files

```c
// Delete all files in a field (set to empty array)
const char* delete_all_body = "{\"documents\":[]}";
char* delete_json = NULL;
bosbase_collection_update(pb, "example", "RECORD_ID", delete_all_body, 
    NULL, 0, NULL, NULL, "{}", "{}", &delete_json, NULL);
bosbase_free_string(delete_json);
```

### Delete Specific Files (Using - Modifier)

```c
// Delete individual files by filename
const char* delete_body = "{\"documents-\":[\"file1.pdf\",\"file2.txt\"]}";
char* delete_json = NULL;
bosbase_collection_update(pb, "example", "RECORD_ID", delete_body, 
    NULL, 0, NULL, NULL, "{}", "{}", &delete_json, NULL);
bosbase_free_string(delete_json);
```

## File URLs

### Get File URL

Each uploaded file can be accessed via its URL:

```
http://localhost:8090/api/files/COLLECTION_ID_OR_NAME/RECORD_ID/FILENAME
```

**Using SDK:**

```c
// Get record first
char* record_json = NULL;
if (bosbase_collection_get_one(pb, "example", "RECORD_ID", 
        NULL, NULL, "{}", "{}", &record_json, NULL) == 0) {
    // Parse record_json to get filename
    // Then construct URL or use bosbase_files_get_url
    
    const char* filename = "file1.txt";  // From parsed record
    char* url = NULL;
    if (bosbase_files_get_url(pb, record_json, filename, NULL, NULL, 0, 
            "{}", &url, NULL) == 0) {
        printf("File URL: %s\n", url);
        bosbase_free_string(url);
    }
    bosbase_free_string(record_json);
}
```

### Image Thumbnails

If your file field has thumbnail sizes configured, you can request thumbnails:

```c
char* record_json = NULL;
bosbase_collection_get_one(pb, "example", "RECORD_ID", 
    NULL, NULL, "{}", "{}", &record_json, NULL);

const char* filename = "photo.jpg";  // Image file
const char* thumb_size = "100x300";  // Width x Height

char* thumb_url = NULL;
if (bosbase_files_get_url(pb, record_json, filename, thumb_size, NULL, 0, 
        "{}", &thumb_url, NULL) == 0) {
    printf("Thumbnail URL: %s\n", thumb_url);
    bosbase_free_string(thumb_url);
}
bosbase_free_string(record_json);
```

**Thumbnail Formats:**

- `WxH` (e.g., `100x300`) - Crop to WxH viewbox from center
- `WxHt` (e.g., `100x300t`) - Crop to WxH viewbox from top
- `WxHb` (e.g., `100x300b`) - Crop to WxH viewbox from bottom
- `WxHf` (e.g., `100x300f`) - Fit inside WxH viewbox (no cropping)
- `0xH` (e.g., `0x300`) - Resize to H height, preserve aspect ratio
- `Wx0` (e.g., `100x0`) - Resize to W width, preserve aspect ratio

**Supported Image Formats:**
- JPEG (`.jpg`, `.jpeg`)
- PNG (`.png`)
- GIF (`.gif` - first frame only)
- WebP (`.webp` - stored as PNG)

### Force Download

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

By default, all files are publicly accessible if you know the full URL. For sensitive files, you can mark the field as "Protected" in the collection settings.

### Accessing Protected Files

Protected files require authentication and a file token:

```c
// Step 1: Authenticate
char* auth_json = NULL;
bosbase_collection_auth_with_password(pb, "users", 
    "user@example.com", "password123", "{}", NULL, NULL, 
    "{}", "{}", &auth_json, NULL);
bosbase_free_string(auth_json);

// Step 2: Get file token (valid for ~2 minutes)
char* token_json = NULL;
if (bosbase_files_get_token(pb, "{}", "{}", &token_json, NULL) == 0) {
    // Parse token_json to get token string
    
    // Step 3: Get protected file URL with token
    char* record_json = NULL;
    bosbase_collection_get_one(pb, "example", "RECORD_ID", 
        NULL, NULL, "{}", "{}", &record_json, NULL);
    
    const char* token = "FILE_TOKEN_FROM_STEP_2";
    char* protected_url = NULL;
    if (bosbase_files_get_url(pb, record_json, "privateDocument", 
            NULL, token, 0, "{}", &protected_url, NULL) == 0) {
        printf("Protected file URL: %s\n", protected_url);
        bosbase_free_string(protected_url);
    }
    bosbase_free_string(record_json);
    bosbase_free_string(token_json);
}
```

**Important:**
- File tokens are short-lived (~2 minutes)
- Only authenticated users satisfying the collection's `viewRule` can access protected files
- Tokens must be regenerated when they expire

## Complete Examples

### Example 1: Image Upload with Thumbnails

```c
// Upload product with image
bosbase_file_attachment image_file[1] = {
    {
        .field = "image",
        .filename = "product.jpg",
        .content_type = "image/jpeg",
        .data = image_data,
        .data_len = image_size
    }
};

const char* product_body = "{\"name\":\"My Product\"}";
char* product_json = NULL;
if (bosbase_collection_create(pb, "products", product_body, image_file, 1, 
        NULL, NULL, "{}", "{}", &product_json, NULL) == 0) {
    // Parse product_json to get filename
    const char* filename = "product.jpg";  // From parsed JSON
    
    // Get thumbnail URL
    char* thumb_url = NULL;
    if (bosbase_files_get_url(pb, product_json, filename, "300x300", 
            NULL, 0, "{}", &thumb_url, NULL) == 0) {
        printf("Thumbnail: %s\n", thumb_url);
        bosbase_free_string(thumb_url);
    }
    bosbase_free_string(product_json);
}
```

### Example 2: File Management

```c
void delete_file_from_record(bosbase_client* pb, 
                              const char* collection,
                              const char* record_id,
                              const char* filename) {
    // Delete specific file using - modifier
    char delete_body[256];
    snprintf(delete_body, sizeof(delete_body), 
             "{\"documents-\":[\"%s\"]}", filename);
    
    char* result_json = NULL;
    if (bosbase_collection_update(pb, collection, record_id, delete_body, 
            NULL, 0, NULL, NULL, "{}", "{}", &result_json, NULL) == 0) {
        printf("File deleted: %s\n", filename);
        bosbase_free_string(result_json);
    }
}
```

## File Field Modifiers

### Summary

- **No modifier** - Replace all files: `{"documents": []}`
- **`+` suffix** - Append files: `{"documents+": "filename"}`
- **`+` prefix** - Prepend files: `{"+documents": "filename"}`
- **`-` suffix** - Delete files: `{"documents-": ["file1.pdf"]}`

## Best Practices

1. **File Size Limits**: Always validate file sizes on the client before upload
2. **MIME Types**: Configure allowed MIME types in collection field settings
3. **Thumbnails**: Pre-generate common thumbnail sizes for better performance
4. **Protected Files**: Use protected files for sensitive documents
5. **Token Refresh**: Refresh file tokens before they expire for protected files
6. **Error Handling**: Handle 404 errors for missing files and 401 for protected file access
7. **Memory Management**: Always free strings returned by SDK functions

## Error Handling

```c
char* result = NULL;
bosbase_error* err = NULL;

if (bosbase_collection_create(pb, "example", body, files, 1, 
        NULL, NULL, "{}", "{}", &result, &err) != 0) {
    if (err) {
        if (err->status == 413) {
            fprintf(stderr, "File too large\n");
        } else if (err->status == 400) {
            fprintf(stderr, "Invalid file type or field validation failed\n");
        } else if (err->status == 403) {
            fprintf(stderr, "Insufficient permissions\n");
        } else {
            fprintf(stderr, "Upload failed: %s\n", err->message);
        }
        bosbase_free_error(err);
    }
    return 1;
}

bosbase_free_string(result);
```

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection and field configuration
- [Authentication](./AUTHENTICATION.md) - Required for protected files
- [File API](./FILE_API.md) - File download and access

