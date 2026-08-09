#ifndef COVA_WEBSOCKET_H
#define COVA_WEBSOCKET_H

#include "request.h"
#include "response.h"

// WebSocket Handshake işlemini gerçekleştirir
// Başarılıysa 1, başarısızsa 0 döner.
int ws_handshake(Request *req, Response *res);

// Sunucudan tarayıcıya düz metin WebSocket mesajı yollar
void ws_send_text(int client_socket, const char *text);

// Tarayıcıdan sunucuya gelen WebSocket mesajını okur (Maskeyi çözer)
// Mesaj başarıyla okunduysa bellekte ayırdığı string'in adresini döner, 
// yoksa (örneğin bağlantı kapandıysa) NULL döner.
// Dönen string "cova_free()" ile temizlenmelidir.
char* ws_read_frame(int client_socket);

#endif
