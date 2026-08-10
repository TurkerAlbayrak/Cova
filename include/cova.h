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

// Endpoint'lere bağlayacağımız fonksiyonların tipi (Örn: void hello(Request *req, Response *res))
typedef void (*Handler)(Request *req, Response *res);

// Araya giren ara yazılım fonksiyonlarının tipi (1 dönerse devam eder, 0 dönerse kesilir)
typedef int (*Middleware)(Request *req, Response *res);

typedef struct {
    HttpMethod method;
    const char *path;
    Handler handler;
} Route;

// Statik dosya rotasını (Örn: "/public" -> "./public_folder") temsil eden yapı
typedef struct {
    const char *url_prefix;
    const char *folder_path;
} StaticRoute;

// Tüm Framework uygulamasını temsil eden nesne
typedef struct {
    Route routes[MAX_ROUTES];
    int route_count;
    
    Middleware middlewares[MAX_MIDDLEWARES];
    int middleware_count;
    
    StaticRoute static_routes[10];
    int static_route_count;
    
    // Global Hata Yöneticileri (V13)
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

// App objesini başlatır
void app_init(App *app);

// Uygulamaya global bir Middleware (Ara Yazılım) ekler
void app_use(App *app, Middleware middleware);

// Uygulamaya statik dosya dizini ekler (Örn: app_static(&app, "/public", "./public");)
void app_static(App *app, const char *url_prefix, const char *folder_path);

// HTTPS desteğini aktifleştirir ve sertifikaları yükler
int app_use_https(App *app, const char *cert_file, const char *key_file);

// Özel 404 (Sayfa Bulunamadı) sayfası tanımlar
void app_on_404(App *app, Handler handler);

// Özel 500 (Sunucu Hatası) sayfası tanımlar
void app_on_500(App *app, Handler handler);

void app_run(App *app, uint16_t port);

// Router API'leri
void app_get(App *app, const char *path, Handler handler);
void app_post(App *app, const char *path, Handler handler);

// JWT Secret ayarlama
void app_set_jwt_secret(App *app, const char *secret);

// Rate Limiting (Saniyedeki Maksimum Istek)
void app_set_rate_limit(App *app, int max_req);

// Maksimum İstek Gövdesi Boyutu (Örn: Dosya Yükleme limiti)
void app_set_max_body_size(App *app, size_t max_size);

#endif // COVA_H
