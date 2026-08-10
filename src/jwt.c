#include "jwt.h"
#include "base64.h"
#include "memtrack.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef USE_OPENSSL
#include <openssl/hmac.h>
#include <openssl/evp.h>
#endif

// Generates HMAC SHA256 and base64url encodes it
static char *jwt_sign(const char *data, const char *secret) {
#ifdef USE_OPENSSL
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    HMAC(EVP_sha256(), secret, strlen(secret), (const unsigned char*)data, strlen(data), hash, &hash_len);
    return base64url_encode(hash, hash_len);
#else
    return NULL;
#endif
}

char *jwt_generate(const char *payload_json, const char *secret) {
    if (!payload_json || !secret) return NULL;
    
    const char *header_json = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char *b64_header = base64url_encode((const unsigned char*)header_json, strlen(header_json));
    char *b64_payload = base64url_encode((const unsigned char*)payload_json, strlen(payload_json));
    
    if (!b64_header || !b64_payload) {
        if (b64_header) cova_free(b64_header);
        if (b64_payload) cova_free(b64_payload);
        return NULL;
    }
    
    size_t data_len = strlen(b64_header) + 1 + strlen(b64_payload) + 1;
    char *data = cova_malloc(data_len);
    snprintf(data, data_len, "%s.%s", b64_header, b64_payload);
    
    char *signature = jwt_sign(data, secret);
    if (!signature) {
        cova_free(b64_header);
        cova_free(b64_payload);
        cova_free(data);
        return NULL;
    }
    
    size_t token_len = data_len + strlen(signature) + 1;
    char *token = cova_malloc(token_len);
    snprintf(token, token_len, "%s.%s", data, signature);
    
    cova_free(b64_header);
    cova_free(b64_payload);
    cova_free(data);
    cova_free(signature);
    
    return token;
}

int jwt_verify(const char *token, const char *secret) {
    if (!token || !secret) return 0;
    
    const char *dot1 = strchr(token, '.');
    if (!dot1) return 0;
    const char *dot2 = strchr(dot1 + 1, '.');
    if (!dot2) return 0;
    
    size_t data_len = dot2 - token;
    char *data = cova_malloc(data_len + 1);
    strncpy(data, token, data_len);
    data[data_len] = '\0';
    
    char *expected_signature = jwt_sign(data, secret);
    const char *provided_signature = dot2 + 1;
    
    int is_valid = 0;
    if (expected_signature && strcmp(expected_signature, provided_signature) == 0) {
        is_valid = 1;
    }
    
    if (expected_signature) cova_free(expected_signature);
    cova_free(data);
    return is_valid;
}

int jwt_middleware(Request *req, Response *res) {
    if (!g_app || strlen(g_app->jwt_secret) == 0) {
        response_status(res, 500);
        response_text(res, "JWT Secret Not Configured");
        return 0; // stop middleware chain
    }
    
    const char *auth_header = request_header(req, "Authorization");
    if (!auth_header || strncmp(auth_header, "Bearer ", 7) != 0) {
        response_status(res, 401);
        response_text(res, "Unauthorized");
        return 0;
    }
    
    const char *token = auth_header + 7;
    if (jwt_verify(token, g_app->jwt_secret)) {
        return 1; // Valid, continue
    } else {
        response_status(res, 401);
        response_text(res, "Unauthorized");
        return 0;
    }
}
