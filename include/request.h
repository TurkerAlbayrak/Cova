#ifndef COVA_REQUEST_H
#define COVA_REQUEST_H

#include <stddef.h>

// Enum representing HTTP Methods
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_PATCH,
    HTTP_UNKNOWN
} HttpMethod;

#define MAX_HEADERS 50
#define MAX_PARAMS 10
#define MAX_QUERIES 10

typedef struct {
    char *name;
    char *value;
} RequestHeader;

typedef struct {
    char *name;
    char *value;
} RequestParam;

typedef struct {
    char *name;
    char *value;
} RequestQuery;

// V22: File upload (Multipart/form-data)
#define MAX_FILES 10
typedef struct {
    char name[64];       // Form field name (e.g., "profile_pic")
    char filename[256];  // Original filename (e.g., "avatar.png")
    char content_type[64]; // e.g., "image/png"
    unsigned char *data; // Binary data pointer
    size_t size;         // File size
} UploadedFile;

// Struct representing the incoming HTTP Request
typedef struct {
    HttpMethod method;
    char *path;
    char *version;
    char *raw_headers;
    char *body;
    
    // V22: Binary (File) body data
    unsigned char *body_data;
    size_t body_len;

    RequestHeader headers[MAX_HEADERS];
    int header_count;

    // V21: IP Address for Rate Limiting
    char client_ip[46];

    // To hold dynamic URL parameters (/users/:id)
    char param_buffer[256]; 
    RequestParam params[MAX_PARAMS];
    int param_count;

    // To hold URL query parameters (?q=Cova)
    RequestQuery queries[MAX_QUERIES];
    int query_count;

    // V22: Uploaded files
    UploadedFile files[MAX_FILES];
    int file_count;
} Request;

// Parses the incoming string buffer into the Request struct
void request_parse(char *buffer, Request *req);

// Helper function to convert HttpMethod enum back to string ("GET", "POST")
const char* http_method_str(HttpMethod method);

// Returns the value of a specific header (Case-insensitive)
const char* request_header(Request *req, const char *name);

// Returns a dynamic path parameter from the URL (e.g., "id")
const char* request_param(Request *req, const char *name);

// Returns a query parameter from the URL (?q=...)
const char* request_query(Request *req, const char *name);

// Forward declaration for cJSON
typedef struct cJSON Json;

// Parses the JSON body of the request and returns a Json object
Json* request_json(Request *req);

#endif // COVA_REQUEST_H
