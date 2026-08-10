#ifndef COVA_RATE_LIMITER_H
#define COVA_RATE_LIMITER_H

#include "cova.h"

// Middleware to block requests if rate limit exceeded
int rate_limit_middleware(Request *req, Response *res);

// Init and cleanup
void rate_limiter_init();
void rate_limiter_cleanup();

#endif // COVA_RATE_LIMITER_H
