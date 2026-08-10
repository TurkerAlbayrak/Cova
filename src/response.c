#include "response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "mime.h"
#include "memtrack.h" // V16: Bellek takibi

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
#endif

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#endif

static int net_send(Response *res, const void *buf, size_t len) {
#ifdef USE_OPENSSL
    if (res->ssl) {
        return SSL_write((SSL*)res->ssl, buf, (int)len);
    }
#endif
    return send(res->client_socket, buf, (int)len, 0);
}

// HTTP durum kodlarına karşılık gelen metinler (Reason Phrases)
static const char* http_status_text(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

void response_status(Response *res, int status_code) {
    if (res) res->status_code = status_code;
}

void response_header(Response *res, const char *name, const char *value) {
    if (!res || !name || !value) return;
    if (res->header_count < MAX_RESPONSE_HEADERS) {
        res->headers[res->header_count].name = name;
        res->headers[res->header_count].value = value;
        res->header_count++;
    }
}

void response_text(Response *res, const char *text) {
    if (!res || !text) return;
    
    char buffer[4096];
    int status = res->status_code;
    const char *reason = http_status_text(status);
    
    // Özel header'ları string olarak birleştiriyoruz
    char header_buf[2048] = {0};
    int offset = 0;
    for (int i = 0; i < res->header_count; i++) {
        offset += snprintf(header_buf + offset, sizeof(header_buf) - offset, 
                           "%s: %s\r\n", res->headers[i].name, res->headers[i].value);
    }
    
    snprintf(buffer, sizeof(buffer),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "%s" // Connection header
        "%s" // Özel header'lar buraya gelir
        "\r\n" // Boş satır, headerların bittiğini gösterir
        "%s", status, reason, strlen(text), res->keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n", header_buf, text);
        
    net_send(res, buffer, strlen(buffer));
}

void response_json(Response *res, const char *json_str) {
    if (!res || !json_str) return;
    
    char buffer[4096];
    int status = res->status_code;
    const char *reason = http_status_text(status);
    
    char header_buf[2048] = {0};
    int offset = 0;
    for (int i = 0; i < res->header_count; i++) {
        offset += snprintf(header_buf + offset, sizeof(header_buf) - offset, 
                           "%s: %s\r\n", res->headers[i].name, res->headers[i].value);
    }
    
    snprintf(buffer, sizeof(buffer),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "%s"
        "%s"
        "\r\n"
        "%s", status, reason, strlen(json_str), res->keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n", header_buf, json_str);
        
    net_send(res, buffer, strlen(buffer));
}

void response_json_object(Response *res, Json *json) {
    if (!res || !json) return;
    
    // cJSON nesnesini okunabilir bir string'e (char array) çevirir
    char *json_str = cJSON_PrintUnformatted(json);
    if (json_str) {
        response_json(res, json_str); // Mevcut fonksiyonumuzu kullanıyoruz
        cova_free(json_str); // cJSON_Print cova_malloc kullanır, cova_free ile silmeliyiz!
    }
}

void response_file(Response *res, const char *filepath) {
    if (!res || !filepath) return;

    FILE *file = fopen(filepath, "rb"); // Dosyayı ikili (binary) modda oku
    if (!file) {
        response_status(res, 404);
        response_text(res, "File Not Found");
        return;
    }
    
    // Dosya boyutunu hesapla
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET); // Başa sar
    
    // MIME tipini bul (mime.h)
    const char *mime = get_mime_type(filepath);
    
    int status = res->status_code;
    const char *reason = http_status_text(status);
    
    // Header'ları hazırla
    char header_buf[2048] = {0};
    int offset = 0;
    for (int i = 0; i < res->header_count; i++) {
        offset += snprintf(header_buf + offset, sizeof(header_buf) - offset, 
                           "%s: %s\r\n", res->headers[i].name, res->headers[i].value);
    }
    
    // HTTP Yanıt başlığını gönder
    char buffer[4096];
    snprintf(buffer, sizeof(buffer),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "%s"
        "%s"
        "\r\n", status, reason, mime, fsize, res->keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n", header_buf);
        
    net_send(res, buffer, strlen(buffer));
    
    // Dosya içeriğini 4KB (4096 byte) parçalar halinde ağa (Network) gönder
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        net_send(res, buffer, bytes_read);
    }
    
    fclose(file);
}

void response_html(Response *res, const char *html_str) {
    if (!res || !html_str) return;
    
    int status = res->status_code;
    const char *reason = http_status_text(status);
    size_t length = strlen(html_str);
    
    char header_buf[2048] = {0};
    int offset = 0;
    for (int i = 0; i < res->header_count; i++) {
        offset += snprintf(header_buf + offset, sizeof(header_buf) - offset, 
                           "%s: %s\r\n", res->headers[i].name, res->headers[i].value);
    }
    
    // Yeterince büyük bir buffer ayırıyoruz
    char *buffer = cova_malloc(length + 2048);
    if (!buffer) return;

    snprintf(buffer, length + 2048,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "%s"
        "%s"
        "\r\n"
        "%s", status, reason, length, res->keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n", header_buf, html_str);
        
    net_send(res, buffer, strlen(buffer));
    cova_free(buffer);
}

void response_render(Response *res, const char *filepath, Json *data) {
    if (!res || !filepath || !data) return;

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        response_status(res, 404);
        response_text(res, "Template Not Found");
        return;
    }
    
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // HTML dosyasını tamamen belleğe al
    char *html_buf = cova_malloc(fsize + 1);
    if (!html_buf) { fclose(file); return; }
    
    fread(html_buf, 1, fsize, file);
    html_buf[fsize] = '\0';
    fclose(file);
    
    // Değişkenler metni uzatabileceği için x2 büyüklüğünde bir çıktı buffer'ı ayır (Geçici basit çözüm)
    size_t out_max = (fsize * 2) + 1024;
    char *out_buf = cova_malloc(out_max);
    if (!out_buf) { cova_free(html_buf); return; }
    
    size_t out_pos = 0;
    char *p = html_buf;
    
    // Basit Template Engine Algoritması (Karakter Karakter Oku)
    while (*p) {
        // "{{..." başlangıcı
        if (*p == '{' && *(p+1) == '{') {
            p += 2; // "{{" işaretini atla
            
            char key[128] = {0};
            int k = 0;
            
            // "}}" kapanışını bulana kadar anahtar kelimeyi (key) oku
            while (*p && !(*p == '}' && *(p+1) == '}') && k < 127) {
                if (*p != ' ') { // Boşlukları yoksay (Örn: {{ isim }} -> isim)
                    key[k++] = *p;
                }
                p++;
            }
            
            if (*p == '}' && *(p+1) == '}') {
                p += 2; // "}}" işaretini atla
            }
            
            // JSON verisinden anahtarı bul (Örn: "isim")
            Json *item = cJSON_GetObjectItemCaseSensitive(data, key);
            if (item) {
                if (cJSON_IsString(item)) {
                    size_t val_len = strlen(item->valuestring);
                    if (out_pos + val_len < out_max) {
                        strcpy(out_buf + out_pos, item->valuestring);
                        out_pos += val_len;
                    }
                } else if (cJSON_IsNumber(item)) {
                    char num_buf[32];
                    snprintf(num_buf, sizeof(num_buf), "%d", item->valueint);
                    size_t val_len = strlen(num_buf);
                    if (out_pos + val_len < out_max) {
                        strcpy(out_buf + out_pos, num_buf);
                        out_pos += val_len;
                    }
                }
            }
        } else {
            // Normal HTML karakterini kopyala
            if (out_pos < out_max - 1) {
                out_buf[out_pos++] = *p;
            }
            p++;
        }
    }
    out_buf[out_pos] = '\0';
    
    // İşlenmiş HTML'i istemciye yolla
    response_html(res, out_buf);
    
    // Bellek sızıntısını önle
    cova_free(html_buf);
    cova_free(out_buf);
}
