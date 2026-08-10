#ifndef COVA_WEBSOCKET_H
#define COVA_WEBSOCKET_H

#include "request.h"
#include "response.h"

// Performs the WebSocket Handshake procedure.
// Returns 1 on success, 0 on failure.
int ws_handshake(Request *req, Response *res);

// Sends a plain text WebSocket message from the server to the browser
void ws_send_text(int client_socket, const char *text);

// Reads a WebSocket message from the browser to the server (Unmasks the message)
// Returns the address of the string allocated in memory if the message was read successfully, 
// otherwise returns NULL (e.g., if the connection was closed).
// The returned string must be freed with "cova_free()".
char* ws_read_frame(int client_socket);

#endif
