#ifndef COVA_H
#define COVA_H

#include <stdint.h>
#include "request.h"
#include "response.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#define MAX_ROUTES 100
#define MAX_MIDDLEWARES 10

// Type definition for endpoint handlers (e.g., void hello(Request *req, Response *res))
typedef void (*Handler)(Request *req, Response *res);

// Type definition for middleware functions (returns 1 to continue, 0 to abort)
typedef int (*Middleware)(Request *req, Response *res);

typedef struct {
    HttpMethod method;
    const char *path;
    Handler handler;
} Route;

// Structure representing a static file route (e.g., "/public" -> "./public_folder")
typedef struct {
    const char *url_prefix;
    const char *folder_path;
} StaticRoute;

// Object representing the entire Framework application
typedef struct {
    Route routes[MAX_ROUTES];
    int route_count;
    
    Middleware middlewares[MAX_MIDDLEWARES];
    int middleware_count;
    
    StaticRoute static_routes[10];
    int static_route_count;
    
    // Global Error Handlers (V13)
    Handler not_found_handler; // 404
    Handler error_handler;     // 500

    // V17: Thread Pool & HTTPS
    void *thread_pool;
    
    int use_https;
#ifdef USE_OPENSSL
    SSL_CTX *ssl_ctx;
#else
    void *ssl_ctx;
#endif

    // V20: JWT
    char jwt_secret[128];
    
    // V21: Rate Limiting
    int max_requests_per_second;

    // V22: Multipart / File Upload max size (bytes)
    size_t max_body_size;
} App;

extern App *g_app;

// Initializes the App object
void app_init(App *app);

// Adds a global Middleware to the application
void app_use(App *app, Middleware middleware);

// Adds a static file directory (e.g., app_static(&app, "/public", "./public");)
void app_static(App *app, const char *url_prefix, const char *folder_path);

// Enables HTTPS support and loads certificates
int app_use_https(App *app, const char *cert_file, const char *key_file);

// Defines a custom 404 (Not Found) handler
void app_on_404(App *app, Handler handler);

// Defines a custom 500 (Internal Server Error) handler
void app_on_500(App *app, Handler handler);

void app_run(App *app, uint16_t port);

// Router APIs
void app_get(App *app, const char *path, Handler handler);
void app_post(App *app, const char *path, Handler handler);

// JWT Secret configuration
void app_set_jwt_secret(App *app, const char *secret);

// Rate Limiting configuration (Max Requests Per Second)
void app_set_rate_limit(App *app, int max_req);

// Maximum Request Body Size (e.g., File Upload limit)
void app_set_max_body_size(App *app, size_t max_size);

#endif // COVA_H
