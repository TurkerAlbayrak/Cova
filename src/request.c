#include "request.h"
#include <string.h>
#include "cJSON.h"

void request_parse(char *buffer, Request *req) {
    if (!buffer || !req) return;

    // Initialize values safely
    req->method = HTTP_UNKNOWN;
    req->path = "";
    req->version = "";
    req->raw_headers = NULL;
    req->body = NULL;
    req->header_count = 0;
    req->param_count = 0;
    req->query_count = 0;

    // Incoming buffer example: "GET /users?q=1 HTTP/1.1\r\nHost: localhost\r\n\r\nBody"
    
    // 1. Method
    char *method_end = strchr(buffer, ' '); // Find first space
    if (!method_end) return;
    *method_end = '\0'; // Replace space with Null (\0)
    
    char *method_str = buffer;
    if (strcmp(method_str, "GET") == 0) req->method = HTTP_GET;
    else if (strcmp(method_str, "POST") == 0) req->method = HTTP_POST;
    else if (strcmp(method_str, "PUT") == 0) req->method = HTTP_PUT;
    else if (strcmp(method_str, "DELETE") == 0) req->method = HTTP_DELETE;
    else if (strcmp(method_str, "PATCH") == 0) req->method = HTTP_PATCH;

    // 2. Path
    char *path_start = method_end + 1;
    char *path_end = strchr(path_start, ' '); // Find second space
    if (path_end) {
        *path_end = '\0'; // Terminate Path
        req->path = path_start;
    }

    // V10: Query String Parsing (?q=Cova)
    // Check if there is '?' in the Path
    char *query_start = strchr(req->path, '?');
    if (query_start) {
        *query_start = '\0'; // Cut Path at '?'
        char *query_string = query_start + 1; // q=Cova&sort=desc
        
        // In-Place Parsing: Replace & and = with \0
        char *pair = query_string;
        while (pair && *pair != '\0' && req->query_count < MAX_QUERIES) {
            char *next_pair = strchr(pair, '&');
            if (next_pair) {
                *next_pair = '\0';
                next_pair++;
            }
            
            char *eq = strchr(pair, '=');
            if (eq) {
                *eq = '\0';
                req->queries[req->query_count].name = pair;
                req->queries[req->query_count].value = eq + 1;
            } else {
                req->queries[req->query_count].name = pair;
                req->queries[req->query_count].value = ""; // Empty string if no value
            }
            req->query_count++;
            pair = next_pair;
        }
    }

    // 3. HTTP Version
    char *version_start = path_end + 1;
    char *version_end = strstr(version_start, "\r\n"); // Find end of line
    if (!version_end) return;
    *version_end = '\0'; // Split end of line
    req->version = version_start;

    // 4. Headers and Body
    char *headers_start = version_end + 2; // \r\n is 2 characters, skip it
    req->raw_headers = headers_start;

    // Body is after two \r\n\r\n according to HTTP standards
    char *body_start = strstr(headers_start, "\r\n\r\n");
    if (body_start) {
        *body_start = '\0'; // Header list ends here
        req->body = body_start + 4; // Skip \r\n\r\n (4 characters)
    }

    // Parse Headers line by line (In-Place Parsing)
    char *line = req->raw_headers;
    while (line && *line != '\0' && req->header_count < MAX_HEADERS) {
        // Find next line
        char *next_line = strstr(line, "\r\n");
        if (next_line) {
            *next_line = '\0'; // Terminate current line
            next_line += 2;    // Skip \r\n and go to next line
        }
        
        // Separate Name and Value by ':'
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *name = line;
            char *value = colon + 1;
            
            // Trim whitespace at the beginning of Value
            while (*value == ' ') value++;
            
            req->headers[req->header_count].name = name;
            req->headers[req->header_count].value = value;
            req->header_count++;
        }
        line = next_line;
    }
}

// Case-insensitive string comparison (ASCII math)
static int header_match(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == *b;
}

const char* request_header(Request *req, const char *name) {
    if (!req || !name) return NULL;
    for (int i = 0; i < req->header_count; i++) {
        if (header_match(req->headers[i].name, name)) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

const char* request_param(Request *req, const char *name) {
    if (!req || !name) return NULL;
    for (int i = 0; i < req->param_count; i++) {
        // Parameter names usually match exactly, strcmp is safe
        if (strcmp(req->params[i].name, name) == 0) {
            return req->params[i].value;
        }
    }
    return NULL;
}

const char* request_query(Request *req, const char *name) {
    if (!req || !name) return NULL;
    for (int i = 0; i < req->query_count; i++) {
        if (strcmp(req->queries[i].name, name) == 0) {
            return req->queries[i].value;
        }
    }
    return NULL;
}

Json* request_json(Request *req) {
    if (!req || !req->body || req->body[0] == '\0') {
        return NULL;
    }
    // Converts text into an actual JSON object (allocates memory!)
    return cJSON_Parse(req->body);
}

const char* http_method_str(HttpMethod method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        case HTTP_PATCH: return "PATCH";
        default: return "UNKNOWN";
    }
}
