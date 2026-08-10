#ifndef COVA_JWT_H
#define COVA_JWT_H

#include "cova.h"

// Generates a JWT Token
char *jwt_generate(const char *payload_json, const char *secret);

// Verifies a JWT Token
int jwt_verify(const char *token, const char *secret);

// JWT Middleware
int jwt_middleware(Request *req, Response *res);

#endif // COVA_JWT_H
