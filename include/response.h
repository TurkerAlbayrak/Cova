#ifndef COVA_RESPONSE_H
#define COVA_RESPONSE_H

#define MAX_RESPONSE_HEADERS 20

typedef struct {
    const char *name;
    const char *value;
} ResponseHeader;

// Object used to send a response in handler functions
typedef struct {
    int client_socket;
    void *ssl; // V17: SSL object for HTTPS
    int keep_alive; // V18: Keep-Alive flag
    int use_gzip; // V19: Gzip flag
    int status_code; // Defaults to 200
    ResponseHeader headers[MAX_RESPONSE_HEADERS];
    int header_count;
} Response;

// Sets the HTTP status code
void response_status(Response *res, int status_code);

// Adds a custom header to the response
void response_header(Response *res, const char *name, const char *value);

// Sends a Plain Text response
void response_text(Response *res, const char *text);

// Sends a JSON formatted string response
void response_json(Response *res, const char *json_str);

// Automatically converts a cJSON object to string and sends a JSON response
typedef struct cJSON Json;
void response_json_object(Response *res, Json *json);

// Sends a file from disk (HTML, CSS, Image) to the client
void response_file(Response *res, const char *filepath);

// Sends a dynamic HTML string
void response_html(Response *res, const char *html_str);

// Reads an HTML Template file, populates {{variable}} parts with JSON data
void response_render(Response *res, const char *filepath, Json *data);

#endif // COVA_RESPONSE_H
