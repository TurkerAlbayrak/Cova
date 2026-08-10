#include "websocket.h"
#include "sha1.h"
#include "base64.h"
#include "memtrack.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
#endif

#define WS_MAGIC_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

int ws_handshake(Request *req, Response *res) {
    const char *ws_key = request_header(req, "Sec-WebSocket-Key");
    if (!ws_key) {
        return 0; 
    }
    
    char concat[256];
    snprintf(concat, sizeof(concat), "%s%s", ws_key, WS_MAGIC_GUID);
    
    SHA1_CTX ctx;
    unsigned char hash[20];
    SHA1Init(&ctx);
    SHA1Update(&ctx, (const unsigned char*)concat, strlen(concat));
    SHA1Final(hash, &ctx);
    
    char *accept_key = base64_encode(hash, 20);
    if (!accept_key) {
        return 0;
    }
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);
             
    send(res->client_socket, buffer, (int)strlen(buffer), 0);
    cova_free(accept_key);
    
    return 1;
}

void ws_send_text(int client_socket, const char *text) {
    size_t len = strlen(text);
    unsigned char frame[10];
    int frame_size = 0;
    
    frame[0] = 0x81; // FIN bit set (1) + Text Opcode (1)
    
    if (len <= 125) {
        frame[1] = (unsigned char)len;
        frame_size = 2;
    } else if (len <= 65535) {
        frame[1] = 126;
        frame[2] = (unsigned char)((len >> 8) & 0xFF);
        frame[3] = (unsigned char)(len & 0xFF);
        frame_size = 4;
    } else {

        return; 
    }
    

    send(client_socket, (const char*)frame, frame_size, 0);
    send(client_socket, text, (int)len, 0);
}

char* ws_read_frame(int client_socket) {
    unsigned char header[2];
    int r = recv(client_socket, (char*)header, 2, 0);
    if (r <= 0) return NULL; 
    
    int opcode = header[0] & 0x0F;
    int masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;
    
    if (opcode == 0x8) return NULL; 
    
    if (payload_len == 126) {
        unsigned char extended[2];
        recv(client_socket, (char*)extended, 2, 0);
        payload_len = (extended[0] << 8) | extended[1];
    } else if (payload_len == 127) {
        unsigned char extended[8];
        recv(client_socket, (char*)extended, 8, 0);
        payload_len = (extended[4] << 24) | (extended[5] << 16) | (extended[6] << 8) | extended[7];
    }
    
    unsigned char mask[4] = {0};
    if (masked) {
        recv(client_socket, (char*)mask, 4, 0);
    }
    
    char *payload = cova_malloc((size_t)payload_len + 1);
    if (!payload) return NULL;
    
    int total_read = 0;
    while (total_read < payload_len) {
        r = recv(client_socket, payload + total_read, (int)payload_len - total_read, 0);
        if (r <= 0) {
            cova_free(payload);
            return NULL;
        }
        total_read += r;
    }
    
    if (masked) {
        for (uint64_t i = 0; i < payload_len; i++) {
            payload[i] ^= mask[i % 4];
        }
    }
    
    payload[payload_len] = '\0';
    return payload;
}
