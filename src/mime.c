#include "mime.h"
#include <string.h>

const char* get_mime_type(const char *filename) {
    if (!filename) return "application/octet-stream";
    
    // Dosya adındaki son noktayı (.) buluyoruz
    const char *dot = strrchr(filename, '.');
    if (!dot) return "application/octet-stream";
    
    // Uzantılara göre MIME tipini belirle
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".gif") == 0) return "image/gif";
    if (strcmp(dot, ".txt") == 0) return "text/plain";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    
    // Bilinmeyen bir uzantıysa varsayılan tip
    return "application/octet-stream";
}
