#ifndef COVA_REQUEST_H
#define COVA_REQUEST_H

// HTTP Metotlarını temsil eden Enum yapısı
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_PATCH,
    HTTP_UNKNOWN
} HttpMethod;

#define MAX_HEADERS 50
#define MAX_PARAMS 10
#define MAX_QUERIES 10

typedef struct {
    char *name;
    char *value;
} RequestHeader;

typedef struct {
    char *name;
    char *value;
} RequestParam;

typedef struct {
    char *name;
    char *value;
} RequestQuery;

// V22: Dosya yukleme (Multipart/form-data)
#define MAX_FILES 10
typedef struct {
    char name[64];       // Form alani adi (Örn: "profile_pic")
    char filename[256];  // Orijinal dosya adi (Örn: "avatar.png")
    char content_type[64]; // Örn: "image/png"
    unsigned char *data; // Binary veri (pointer)
    size_t size;         // Dosya boyutu
} UploadedFile;

// Gelen HTTP İsteğini (Request) temsil eden Struct
typedef struct {
    HttpMethod method;
    char *path;
    char *version;
    char *raw_headers;
    char *body;
    
    // V22: Binary (Dosya) body datasi
    unsigned char *body_data;
    size_t body_len;

    RequestHeader headers[MAX_HEADERS];
    int header_count;

    // V21: Rate Limiting icin IP Adresi
    char client_ip[46];

    // URL'deki (Path) parametreleri (/users/:id) tutmak için
    char param_buffer[256]; 
    RequestParam params[MAX_PARAMS];
    int param_count;

    // URL sonundaki (?q=Cova) Query parametreleri için
    RequestQuery queries[MAX_QUERIES];
    int query_count;

    // V22: Yüklenen dosyalar
    UploadedFile files[MAX_FILES];
    int file_count;
} Request;

// İsteği metinden (string) alıp struct'a dönüştürür (ayrıştırır)
void request_parse(char *buffer, Request *req);

// Enum methodu tekrar string'e ("GET", "POST") dönüştürmek için yardımcı fonksiyon
const char* http_method_str(HttpMethod method);

// İstenilen bir header değerini döndürür (Büyük/küçük harf duyarsız)
const char* request_header(Request *req, const char *name);

// URL içerisindeki dinamik path parametresini döndürür (örn: "id")
const char* request_param(Request *req, const char *name);

// URL sonundaki (?q=...) sorgu parametresini döndürür
const char* request_query(Request *req, const char *name);

// cJSON yapısını framework'ümüze entegre ediyoruz
typedef struct cJSON Json;

// İstek gövdesindeki (Body) JSON verisini parse edip obje olarak döner
Json* request_json(Request *req);

#endif // COVA_REQUEST_H
