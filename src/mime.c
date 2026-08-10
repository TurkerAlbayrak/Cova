#include "mime.h"
#include <string.h>

const char* get_mime_type(const char *filename) {
    if (!filename) return "application/octet-stream";
    
    // Find the last dot (.) in the filename
    const char *dot = strrchr(filename, '.');
    if (!dot) return "application/octet-stream";
    
    // Determine MIME type based on extension
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".gif") == 0) return "image/gif";
    if (strcmp(dot, ".txt") == 0) return "text/plain";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    
    // Default type for unknown extensions
    return "application/octet-stream";
}
