#include "cova.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "database.h"
#include "websocket.h"
#include "memtrack.h"
#include "jwt.h"
#include "rate_limiter.h"

// ---------------------------------------------------------
// Application State
// ---------------------------------------------------------
App app;

// ---------------------------------------------------------
// Middlewares
// ---------------------------------------------------------

/*
 * Logger Middleware
 * Logs incoming HTTP requests.
 */
int logger_middleware(Request *req, Response *res) {
    (void)res;
    printf("[LOGGER] %s %s\n", http_method_str(req->method), req->path);
    return 1; // Continue to next handler
}

// ---------------------------------------------------------
// Handlers
// ---------------------------------------------------------

/*
 * Hello World Route
 * Responds with a simple text message.
 */
void hello_handler(Request *req, Response *res) {
    (void)req;
    response_header(res, "X-Powered-By", "Cova-Framework");
    response_text(res, "Welcome to Cova Framework!");
}

/*
 * JSON Response Route
 * Creates a JSON object and responds with it.
 */
void api_status_handler(Request *req, Response *res) {
    (void)req;
    Json *res_json = cJSON_CreateObject();
    cJSON_AddStringToObject(res_json, "status", "online");
    cJSON_AddStringToObject(res_json, "version", "1.0.0");
    
    response_status(res, 200);
    response_json_object(res, res_json);
    
    cJSON_Delete(res_json); // Prevent memory leaks
}

/*
 * SQLite Integration Route
 * Demonstrates database querying and returning JSON arrays.
 */
void db_users_handler(Request *req, Response *res) {
    (void)req;
    const char *sql = "SELECT * FROM users LIMIT 10;";
    Json *rows = db_query(sql);
    
    if (rows) {
        response_json_object(res, rows);
        cJSON_Delete(rows);
    } else {
        response_status(res, 500);
        response_json(res, "{\"error\": \"Database error or empty table\"}");
    }
}

/*
 * WebSocket Chat Route
 * Upgrades the connection to a WebSocket and echoes messages.
 */
void websocket_chat_handler(Request *req, Response *res) {
    if (!ws_handshake(req, res)) {
        response_status(res, 400);
        response_text(res, "WebSocket Upgrade Required");
        return;
    }
    
    printf("[WEBSOCKET] Client connected.\n");
    
    while (1) {
        char *msg = ws_read_frame(res->client_socket);
        if (!msg) {
            printf("[WEBSOCKET] Client disconnected.\n");
            break;
        }
        
        // Echo the message back
        char reply[512];
        snprintf(reply, sizeof(reply), "Server Echo: %s", msg);
        ws_send_text(res->client_socket, reply);
        
        cova_free(msg);
    }
}

void login_handler(Request *req, Response *res) {
    (void)req;
    // Basit login simulasyonu (Gercekte db'den kullanici kontrol edilir)
    char *token = jwt_generate("{\"user\":\"admin\"}", "my_super_secret_key");
    if (token) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "{\"token\":\"%s\"}", token);
        response_json(res, buf);
        cova_free(token);
    } else {
        response_status(res, 500);
        response_text(res, "Token generation failed");
    }
}

// (Removed unprotected protected_handler)

// V22: Dosya Yukleme (Multipart) Handler
void upload_handler(Request *req, Response *res) {
    if (req->file_count == 0) {
        response_status(res, 400);
        response_text(res, "No files uploaded.");
        return;
    }
    
    // Ilk yuklenen dosya hakkinda bilgi don
    char buf[512];
    snprintf(buf, sizeof(buf), 
        "{\"status\":\"success\", \"filename\":\"%s\", \"size\":%zu, \"content_type\":\"%s\"}",
        req->files[0].filename, req->files[0].size, req->files[0].content_type);
        
    // (Opsiyonel) Dosyayi diske kaydetmek isterseniz:
    // FILE *f = fopen(req->files[0].filename, "wb");
    // fwrite(req->files[0].data, 1, req->files[0].size, f);
    // fclose(f);
    response_json(res, buf);
}

// V22: Testler icin rate limit degistirme endpointi
void set_rate_limit_handler(Request *req, Response *res) {
    const char *limit_str = request_param(req, "limit");
    if (limit_str) {
        g_app->max_requests_per_second = atoi(limit_str);
    }
    response_text(res, "Rate limit updated");
}

void jwt_protected_handler(Request *req, Response *res) {
    // Route seviyesinde JWT Middleware cagirimi
    if (!jwt_middleware(req, res)) return;
    
    response_text(res, "Welcome to the protected zone, admin!");
}

/*
 * Custom 404 Error Handler
 */
void not_found_handler(Request *req, Response *res) {
    (void)req;
    response_status(res, 404);
    response_json(res, "{\"error\": \"Resource not found\"}");
}

// ---------------------------------------------------------
// Main Entry Point
// ---------------------------------------------------------
int main(void) {
    // 1. Initialize SQLite Database
    if (!db_init("app.db")) {
        printf("Failed to initialize database.\n");
        return 1;
    }
    
    db_execute("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL);");
    db_execute("INSERT INTO users (name) VALUES ('Admin');"); // Sample data

    // 2. Initialize the Framework App
    app_init(&app);
    
    // 3. Register Middlewares
    app_use(&app, logger_middleware);
    
    // 4. Register Static Files Directory
    app_static(&app, "/public", "./public");
    
    // 5. Register Custom Error Handlers
    app_on_404(&app, not_found_handler);

    // V20: JWT Secret Ayari
    app_set_jwt_secret(&app, "my_super_secret_key");

    // V21: Rate Limiter Ayari (Saniyede max 1000 istek, diger testleri bloklamamasi icin)
    app_set_rate_limit(&app, 1000);
    app_use(&app, rate_limit_middleware);
    
    // 6. Define Routes
    app_get(&app, "/", hello_handler);
    app_get(&app, "/api/status", api_status_handler);
    app_get(&app, "/api/users", db_users_handler);
    app_get(&app, "/ws", websocket_chat_handler);
    app_get(&app, "/login", login_handler);
    app_get(&app, "/protected", jwt_protected_handler);
    app_get(&app, "/set_rate_limit", set_rate_limit_handler);
    
    // V22: File Upload route
    app_post(&app, "/upload", upload_handler);
    
    // 7. HTTPS Desteği (Sertifikalar varsa)
    // Örnek: app_use_https(&app, "server.crt", "server.key");
    // app_use_https(&app, "server.crt", "server.key");
    
    // 8. Start the Server
    printf("Starting Cova Framework server...\n");
    app_run(&app, 8080);
    
    return 0;
}
