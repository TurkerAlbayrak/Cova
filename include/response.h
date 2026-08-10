#ifndef COVA_RESPONSE_H
#define COVA_RESPONSE_H

#define MAX_RESPONSE_HEADERS 20

typedef struct {
    const char *name;
    const char *value;
} ResponseHeader;

// Handler fonksiyonlarında yanıt göndermek için kullandığımız obje
typedef struct {
    int client_socket;
    void *ssl; // V17: HTTPS için SSL nesnesi
    int keep_alive; // V18: Keep-Alive bayrağı
    int use_gzip; // V19: Gzip bayrağı
    int status_code; // Varsayılan olarak 200 olacak
    ResponseHeader headers[MAX_RESPONSE_HEADERS];
    int header_count;
} Response;

// HTTP durum kodunu (Status Code) ayarlar
void response_status(Response *res, int status_code);

// Yanıta özel bir Header ekler
void response_header(Response *res, const char *name, const char *value);

// Metin (Plain Text) yanıtı gönderir
void response_text(Response *res, const char *text);

// JSON formatında string yanıt gönderir
void response_json(Response *res, const char *json_str);

// cJSON objesini otomatik string'e çevirip JSON yanıtı gönderir
typedef struct cJSON Json;
void response_json_object(Response *res, Json *json);

// Diskteki bir dosyayı (HTML, CSS, Image) istemciye gönderir
void response_file(Response *res, const char *filepath);

// Dinamik HTML string gönderir
void response_html(Response *res, const char *html_str);

// HTML Template dosyasını okur, {{degisken}} kısımlarını JSON ile doldurur
void response_render(Response *res, const char *filepath, Json *data);

#endif // COVA_RESPONSE_H
