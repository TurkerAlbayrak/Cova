#include "request.h"
#include <string.h>
#include "cJSON.h"

void request_parse(char *buffer, Request *req) {
    if (!buffer || !req) return;

    // Başlangıç değerlerini güvenli olması için sıfırlayalım
    req->method = HTTP_UNKNOWN;
    req->path = "";
    req->version = "";
    req->raw_headers = NULL;
    req->body = NULL;
    req->header_count = 0;
    req->param_count = 0;
    req->query_count = 0;

    // Gelen buffer örneği: "GET /users?q=1 HTTP/1.1\r\nHost: localhost\r\n\r\nBody"
    
    // 1. Method
    char *method_end = strchr(buffer, ' '); // İlk boşluğu bul
    if (!method_end) return;
    *method_end = '\0'; // Boşluğun yerine Null (\0) koy (String'i buradan böl)
    
    char *method_str = buffer;
    if (strcmp(method_str, "GET") == 0) req->method = HTTP_GET;
    else if (strcmp(method_str, "POST") == 0) req->method = HTTP_POST;
    else if (strcmp(method_str, "PUT") == 0) req->method = HTTP_PUT;
    else if (strcmp(method_str, "DELETE") == 0) req->method = HTTP_DELETE;
    else if (strcmp(method_str, "PATCH") == 0) req->method = HTTP_PATCH;

    // 2. Path (Yol)
    char *path_start = method_end + 1;
    char *path_end = strchr(path_start, ' '); // İkinci boşluğu bul
    if (path_end) {
        *path_end = '\0'; // Path'i sonlandır (/users kısmı için)
        req->path = path_start;
    }

    // V10: Query String Ayrıştırması (?q=Cova)
    // Path'in içinde '?' var mı diye bakıyoruz
    char *query_start = strchr(req->path, '?');
    if (query_start) {
        *query_start = '\0'; // Path'i '?' olduğu yerden kes (/search olarak kalır)
        char *query_string = query_start + 1; // q=Cova&sort=desc
        
        // In-Place Parsing: & ve = işaretlerini \0 ile değiştiriyoruz
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
                req->queries[req->query_count].value = ""; // Değer yoksa boş string ("")
            }
            req->query_count++;
            pair = next_pair;
        }
    }

    // 3. HTTP Versiyonu
    char *version_start = path_end + 1;
    char *version_end = strstr(version_start, "\r\n"); // Satır sonunu bul
    if (!version_end) return;
    *version_end = '\0'; // Satır sonunu böl
    req->version = version_start;

    // 4. Header'lar ve Body (Gövde)
    char *headers_start = version_end + 2; // \r\n 2 karakterdir, onu atla
    req->raw_headers = headers_start;

    // Body (Gövde), HTTP standardına göre iki kere \r\n\r\n sonrasındadır
    char *body_start = strstr(headers_start, "\r\n\r\n");
    if (body_start) {
        *body_start = '\0'; // Header listesi burada biter
        req->body = body_start + 4; // \r\n\r\n kısmını atla (4 karakter)
    }

    // Header'ları satır satır ayrıştır (In-Place Parsing)
    char *line = req->raw_headers;
    while (line && *line != '\0' && req->header_count < MAX_HEADERS) {
        // Bir sonraki satırı bul
        char *next_line = strstr(line, "\r\n");
        if (next_line) {
            *next_line = '\0'; // Mevcut satırı sonlandır
            next_line += 2;    // \r\n'i atlayıp sonraki satıra geç
        }
        
        // ':' karakterine göre Name ve Value'yu ayır
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *name = line;
            char *value = colon + 1;
            
            // Value'nun başındaki boşlukları atla (Trim whitespace)
            while (*value == ' ') value++;
            
            req->headers[req->header_count].name = name;
            req->headers[req->header_count].value = value;
            req->header_count++;
        }
        line = next_line;
    }
}

// Büyük/küçük harf duyarsız string karşılaştırma (ASCII matematiği)
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
        // Parametre isimleri genelde birebir eşleşir, strcmp güvenlidir
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
    // Metni gerçek bir JSON objesine dönüştürüyoruz (Bellekte yer ayrılır!)
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
