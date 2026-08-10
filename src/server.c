#include "cova.h"
#include "request.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "cJSON.h"
#include "memtrack.h"
#include "threadpool.h"
#include "rate_limiter.h"
#include "multipart.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h> // CreateThread için
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#ifndef CLOSE_SOCKET
    #define CLOSE_SOCKET(s) close(s)
#endif
#endif

// Router Eşleştirme Motoru (YENİ - V9) - İleri Tanımlama (Forward Declaration)
static int match_route(const char *route_path, const char *req_path, Request *req);

// Global app pointer for graceful shutdown and JWT access
App *g_app = NULL;

// Thread'e gönderilecek argümanları taşıyan yapı
typedef struct {
    App *app;
    int client_socket;
    char client_ip[46];
} ClientArgs;

// --- THREAD FONKSİYONU ---
// Her gelen bağlantı (client) kendi bağımsız kanalında (thread) bu fonksiyonu çalıştırır.
#ifdef _WIN32
DWORD WINAPI handle_client_thread(LPVOID arg) {
#else
void* handle_client_thread(void *arg) {
#endif
    ClientArgs *client_args = (ClientArgs*)arg;
    int client_socket = client_args->client_socket;
    App *app = client_args->app;
    char client_ip[46];
    strncpy(client_ip, client_args->client_ip, sizeof(client_ip));
    client_ip[45] = '\0';
    
    // Argümanlar kopyalandı, belleği temizle (V16 - Kendi free fonksiyonumuz)
    cova_free(client_args);

    // Zaman Aşımı (Timeout) ayarı (5 saniye)
#ifdef _WIN32
    DWORD timeout = 5000;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const void*)&tv, sizeof(tv));
#endif

#ifdef USE_OPENSSL
    SSL *ssl = NULL;
    if (app->use_https && app->ssl_ctx) {
        ssl = SSL_new((SSL_CTX*)app->ssl_ctx);
        SSL_set_fd(ssl, client_socket);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            CLOSE_SOCKET(client_socket);
#ifdef _WIN32
            return 0;
#else
            return NULL;
#endif
        }
    }
#endif

    while (1) {
        int header_buf_size = 8192; // Max header boyutu
        char *header_buffer = (char*)cova_malloc(header_buf_size);
        memset(header_buffer, 0, header_buf_size);
        
        int total_read = 0;
        int header_ended = 0;

        while (total_read < header_buf_size - 1) {
            int bytes_read = 0;
#ifdef USE_OPENSSL
            if (ssl) {
                bytes_read = SSL_read(ssl, header_buffer + total_read, header_buf_size - 1 - total_read);
            } else
#endif
            {
                bytes_read = recv(client_socket, header_buffer + total_read, header_buf_size - 1 - total_read, 0);
            }
            
            if (bytes_read <= 0) break;
            total_read += bytes_read;
            
            if (strstr(header_buffer, "\r\n\r\n")) {
                header_ended = 1;
                break;
            }
        }
        
        if (total_read <= 0 || !header_ended) {
            cova_free(header_buffer);
            break; // Bağlantı kapandı veya Timeout oldu veya geçersiz istek
        }

        // Content-Length'i bul
        int content_length = 0;
        char *cl_ptr = strstr(header_buffer, "Content-Length:");
        if (!cl_ptr) cl_ptr = strstr(header_buffer, "content-length:");
        if (cl_ptr) {
            content_length = atoi(cl_ptr + 15);
        }
        
        // V22: Limit Kontrolü
        if (content_length > (int)app->max_body_size) {
            Response res;
            res.client_socket = client_socket;
            res.ssl = ssl;
            res.status_code = 413;
            res.header_count = 0;
            res.keep_alive = 0;
            res.use_gzip = 0;
            response_text(&res, "Payload Too Large");
            cova_free(header_buffer);
            break;
        }

        char *header_end = strstr(header_buffer, "\r\n\r\n");
        int header_len = (header_end - header_buffer) + 4;
        int body_read_so_far = total_read - header_len;
        
        unsigned char *body_data = NULL;
        if (content_length > 0) {
            body_data = (unsigned char*)cova_malloc(content_length + 1);
            if (body_read_so_far > 0) {
                memcpy(body_data, header_buffer + header_len, body_read_so_far);
            }
            
            int body_total = body_read_so_far;
            while (body_total < content_length) {
                int bytes_read = 0;
#ifdef USE_OPENSSL
                if (ssl) {
                    bytes_read = SSL_read(ssl, body_data + body_total, content_length - body_total);
                } else
#endif
                {
                    bytes_read = recv(client_socket, (char*)(body_data + body_total), content_length - body_total, 0);
                }
                
                if (bytes_read <= 0) break;
                body_total += bytes_read;
            }
            body_data[content_length] = '\0';
        }

        // Buffer'ı Request struct'ına çevir
        Request req;
        memset(&req, 0, sizeof(Request));
        strncpy(req.client_ip, client_ip, sizeof(req.client_ip));
        req.client_ip[45] = '\0';
        request_parse(header_buffer, &req);
        
        req.body_data = body_data;
        req.body_len = content_length;
        if (content_length > 0) {
            req.body = (char*)body_data; // Eski yapi icin (JSON)
        }
        
        // V22: Multipart Parse
        multipart_parse(&req);

        // Eğer parse edilemeyen anlamsız bir istekse (V13 - 500 Hatası)
        if (req.method == HTTP_UNKNOWN) {
            Response res;
            res.client_socket = client_socket;
            res.status_code = 500;
            res.header_count = 0;
            
            if (app->error_handler) {
                app->error_handler(&req, &res);
            } else {
                response_text(&res, "Internal Server Error - Invalid Request");
            }
            if (req.body_data) cova_free(req.body_data);
            cova_free(header_buffer);
            CLOSE_SOCKET(client_socket);
#ifdef _WIN32
            return 0;
#else
            return NULL;
#endif
        }

        printf("[REQUEST] Method: %s, Path: %s\n", http_method_str(req.method), req.path);
        
        // Router Mantığı
        Response res;
        res.client_socket = client_socket;
        res.ssl = NULL;
#ifdef USE_OPENSSL
        res.ssl = ssl;
#endif
        res.status_code = 200; // Varsayılan durum kodu
        res.header_count = 0;  // Başlangıçta hiç özel header yok
        
        // V18: Keep-Alive tespiti (HTTP/1.1 varsayılan olarak kalıcıdır)
        int keep_alive = 1;
        const char *conn_header = request_header(&req, "Connection");
        // strcasecmp POSIX, stricmp Windows
#ifdef _WIN32
        if (conn_header && _stricmp(conn_header, "close") == 0) keep_alive = 0;
#else
        if (conn_header && strcasecmp(conn_header, "close") == 0) keep_alive = 0;
#endif
        res.keep_alive = keep_alive;
        
        // V19: Gzip Sıkıştırma Tespiti
        res.use_gzip = 0;
        const char *enc_header = request_header(&req, "Accept-Encoding");
        if (enc_header && strstr(enc_header, "gzip")) {
            res.use_gzip = 1;
        }
        
        // Middleware zinciri (V11)
        int continue_chain = 1;
        for (int i = 0; i < app->middleware_count; i++) {
            if (app->middlewares[i](&req, &res) == 0) {
                continue_chain = 0;
                break;
            }
        }

        // Middleware'lerden onay çıkarsa Router'a geç
        if (continue_chain) {
            int route_found = 0;
            
            // 1. Önce statik dosyalara bak (Eğer GET isteği ise)
            if (req.method == HTTP_GET) {
                for (int i = 0; i < app->static_route_count; i++) {
                    int prefix_len = strlen(app->static_routes[i].url_prefix);
                    
                    if (strncmp(req.path, app->static_routes[i].url_prefix, prefix_len) == 0) {
                        const char *remaining_path = req.path + prefix_len;
                        if (*remaining_path == '\0') remaining_path = "/index.html"; 
                        
                        char filepath[512];
                        snprintf(filepath, sizeof(filepath), "%s%s", 
                                 app->static_routes[i].folder_path, 
                                 remaining_path);
                        
                        response_file(&res, filepath);
                        route_found = 1;
                        break; 
                    }
                }
            }

            // 2. Dinamik Router Mantığı çalışır
            if (!route_found) {
                for (int i = 0; i < app->route_count; i++) {
                    if (app->routes[i].method == req.method &&
                        match_route(app->routes[i].path, req.path, &req)) {
                        
                        app->routes[i].handler(&req, &res);
                        route_found = 1;
                        break;
                    }
                }
            }

            if (!route_found) {
                if (app->not_found_handler) {
                    app->not_found_handler(&req, &res);
                } else {
                    response_status(&res, 404);
                    response_text(&res, "Not Found");
                }
            }
        }
        
        // V22: Temizlik
        if (req.body_data) cova_free(req.body_data);
        cova_free(header_buffer);
        
        // V18: Eğer Keep-Alive istenmemişse veya hata varsa döngüyü kır
        if (!keep_alive) {
            break;
        }
    } // while(1) Bitişi

    // Bağlantıyı kapat
#ifdef USE_OPENSSL
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
#endif
    CLOSE_SOCKET(client_socket);
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

void app_init(App *app) {
    if (app == NULL) return;
    app->route_count = 0;
    app->middleware_count = 0;
    app->static_route_count = 0;
    app->not_found_handler = NULL;
    app->error_handler = NULL;
    
    app->use_https = 0;
    app->ssl_ctx = NULL;
    
    // JWT varsayilan bos
    memset(app->jwt_secret, 0, sizeof(app->jwt_secret));
    
    // Rate Limit varsayilan kapali (0)
    app->max_requests_per_second = 0;
    
    // V22: Dosya yukleme varsayilan max limit 10 MB
    app->max_body_size = 10 * 1024 * 1024;
    
    // Thread pool'u hemen baslatma, app_run'da baslatilacak
    app->thread_pool = NULL;
    
    // V16: JSON kütüphanesinin (cJSON) arka planda bizim Tracker'ımızı kullanmasını sağlıyoruz!
    cJSON_Hooks hooks;
    hooks.malloc_fn = cova_malloc;
    hooks.free_fn = cova_free;
    cJSON_InitHooks(&hooks);
}

int app_use_https(App *app, const char *cert_file, const char *key_file) {
    if (!app) return 0;
#ifdef USE_OPENSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return 0;
    }
    
    if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 0;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        return 0;
    }
    
    app->use_https = 1;
    app->ssl_ctx = ctx;
    return 1;
#else
    printf("OpenSSL is not enabled in this build.\n");
    return 0;
#endif
}

void app_use(App *app, Middleware middleware) {
    if (app && app->middleware_count < MAX_MIDDLEWARES) {
        app->middlewares[app->middleware_count++] = middleware;
    }
}

void app_static(App *app, const char *url_prefix, const char *folder_path) {
    if (app && app->static_route_count < 10) {
        app->static_routes[app->static_route_count].url_prefix = url_prefix;
        app->static_routes[app->static_route_count].folder_path = folder_path;
        app->static_route_count++;
    }
}

void app_on_404(App *app, Handler handler) {
    if (app) app->not_found_handler = handler;
}

void app_on_500(App *app, Handler handler) {
    if (app) app->error_handler = handler;
}

// Router Eşleştirme Motoru (YENİ - V9)
static int match_route(const char *route_path, const char *req_path, Request *req) {
    req->param_count = 0;
    int buffer_offset = 0;
    
    const char *rp = route_path; // Örn: "/users/:id"
    const char *rq = req_path;   // Örn: "/users/42"
    
    while (*rp && *rq) {
        if (*rp == ':') {
            rp++; // ':' karakterini atla
            const char *param_name_start = rp;
            while (*rp && *rp != '/') rp++; // Parametre adının sonunu bul ("id")
            
            const char *param_val_start = rq;
            while (*rq && *rq != '/') rq++; // Gelen değerin sonunu bul ("42")
            
            int name_len = rp - param_name_start;
            int val_len = rq - param_val_start;
            
            // Buffer'da yeterli yer var mı kontrolü
            if (req->param_count < MAX_PARAMS && buffer_offset + name_len + val_len + 2 < 256) {
                // İsmi kopyala (Name)
                char *name_ptr = req->param_buffer + buffer_offset;
                strncpy(name_ptr, param_name_start, name_len);
                name_ptr[name_len] = '\0';
                buffer_offset += name_len + 1;
                
                // Değeri kopyala (Value)
                char *val_ptr = req->param_buffer + buffer_offset;
                strncpy(val_ptr, param_val_start, val_len);
                val_ptr[val_len] = '\0';
                buffer_offset += val_len + 1;
                
                req->params[req->param_count].name = name_ptr;
                req->params[req->param_count].value = val_ptr;
                req->param_count++;
            }
        } else if (*rp == *rq) {
            rp++;
            rq++;
        } else {
            return 0; // Karakterler uyuşmadı, route eşleşmedi!
        }
    }
    
    // Her iki string de tamamen eşleşerek bittiyse başarılıdır
    return (*rp == '\0' && *rq == '\0');
}

void app_get(App *app, const char *path, Handler handler) {
    if (app->route_count >= MAX_ROUTES) return;
    app->routes[app->route_count].method = HTTP_GET;
    app->routes[app->route_count].path = (char*)path;
    app->routes[app->route_count].handler = handler;
    app->route_count++;
}

void app_post(App *app, const char *path, Handler handler) {
    if (app == NULL || app->route_count >= MAX_ROUTES) return;
    app->routes[app->route_count].method = HTTP_POST;
    app->routes[app->route_count].path = (char*)path;
    app->routes[app->route_count].handler = handler;
    app->route_count++;
}

void app_set_jwt_secret(App *app, const char *secret) {
    if (!app || !secret) return;
    strncpy(app->jwt_secret, secret, sizeof(app->jwt_secret) - 1);
    app->jwt_secret[sizeof(app->jwt_secret) - 1] = '\0';
}

void app_set_rate_limit(App *app, int max_req) {
    if (!app) return;
    app->max_requests_per_second = max_req;
}

void app_set_max_body_size(App *app, size_t max_size) {
    if (!app) return;
    app->max_body_size = max_size;
}

void app_free(App *app) {
    if (!app) return;
    if (app->thread_pool) {
        threadpool_destroy(app->thread_pool);
        app->thread_pool = NULL;
    }
#ifdef USE_OPENSSL
    if (app->ssl_ctx) {
        SSL_CTX_free((SSL_CTX*)app->ssl_ctx);
        app->ssl_ctx = NULL;
    }
#endif
    
    rate_limiter_cleanup(); // V21 Rate Limiter temizligi
}

// V16: Sunucu CTRL+C ile durdurulduğunda rapor basmak için sinyal yakalayıcı
void handle_sigint(int sig) {
    (void)sig; // unused warning engelle
    printf("\n[INFO] Shutting down server gracefully...\n");
    if (g_app) {
        app_free(g_app);
    }
    cova_mem_report(); // Raporu bas
    exit(0); // Çık
}

void app_run(App *app, uint16_t port) {
    if (app == NULL) return;
    g_app = app;

    if (app->thread_pool == NULL) {
        app->thread_pool = threadpool_create(16);
    }

    // İşletim sisteminden CTRL+C (Interrupt) sinyali gelirse kendi fonksiyonumuzu çağır
    signal(SIGINT, handle_sigint);

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed.\n");
        exit(EXIT_FAILURE);
    }
#endif

    int server_fd;
    struct sockaddr_in address;
    
#ifdef _WIN32
    char opt = 1; // Windows'ta setsockopt char* bekler
#else
    int opt = 1;  // Linux'ta int bekler
#endif

    // 1. Socket oluşturma (IPv4, TCP)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Portun tekrar kullanılabilmesi için SO_REUSEADDR ayarı
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 3. Adres yapısını ayarlama
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Bütün IP'leri dinle
    address.sin_port = htons(port);       // Host byte order -> Network byte order (Big Endian)

    // 4. Socket'i porta bağlama (Bind)
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 5. Dinlemeye başlama (Listen)
    if (listen(server_fd, 10) < 0) { // 10: Bağlantı kuyruğu uzunluğu (backlog)
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Server listening on port %d...\n", port);

    while (1) {
        int client_socket;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // Bağlantı bekle ve kabul et (Bloklayıcı çağrı)
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        // V14: Multi-threading (Eşzamanlılık)
        // Her bağlantı (request) için ayrı bir hafıza ayır ve Thread'e gönder
        ClientArgs *args = (ClientArgs*)cova_malloc(sizeof(ClientArgs)); // V16 Tracker
        if (!args) {
            CLOSE_SOCKET(client_socket);
            continue;
        }
        args->client_socket = client_socket;
        args->app = app;
        
        // V21: IP adresini kopyala
        strncpy(args->client_ip, inet_ntoa(client_addr.sin_addr), sizeof(args->client_ip));
        args->client_ip[45] = '\0';

        // V17: Thread Pool kullanımı
        threadpool_add_task((ThreadPool*)app->thread_pool, (ThreadFunc)handle_client_thread, args);
    }

    CLOSE_SOCKET(server_fd);

#ifdef _WIN32
    WSACleanup();
#endif
}
