#include "rate_limiter.h"
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#define MAX_IP_TRACK 1024

typedef struct {
    char ip[46];
    int count;
    time_t last_request_time;
} RateLimitEntry;

static RateLimitEntry ip_table[MAX_IP_TRACK];
static int initialized = 0;

#ifdef _WIN32
static CRITICAL_SECTION rl_mutex;
#else
static pthread_mutex_t rl_mutex;
#endif

void rate_limiter_init() {
    if (initialized) return;
    memset(ip_table, 0, sizeof(ip_table));
#ifdef _WIN32
    InitializeCriticalSection(&rl_mutex);
#else
    pthread_mutex_init(&rl_mutex, NULL);
#endif
    initialized = 1;
}

void rate_limiter_cleanup() {
    if (!initialized) return;
#ifdef _WIN32
    DeleteCriticalSection(&rl_mutex);
#else
    pthread_mutex_destroy(&rl_mutex);
#endif
    initialized = 0;
}

static int rate_limiter_check(const char *ip, int max_req) {
    if (max_req <= 0) return 1; // Kapali
    if (!initialized) rate_limiter_init();
    
    time_t now = time(NULL);
    int allowed = 1;

#ifdef _WIN32
    EnterCriticalSection(&rl_mutex);
#else
    pthread_mutex_lock(&rl_mutex);
#endif

    int found_idx = -1;
    int oldest_idx = 0;
    time_t oldest_time = now;
    
    for (int i = 0; i < MAX_IP_TRACK; i++) {
        if (ip_table[i].ip[0] != '\0') {
            if (strcmp(ip_table[i].ip, ip) == 0) {
                found_idx = i;
                break;
            }
            if (ip_table[i].last_request_time < oldest_time) {
                oldest_time = ip_table[i].last_request_time;
                oldest_idx = i;
            }
        } else if (found_idx == -1) {
            found_idx = i;
        }
    }
    
    if (found_idx == -1) {
        // Tablo doluysa en eskisini ez
        found_idx = oldest_idx;
        memset(&ip_table[found_idx], 0, sizeof(RateLimitEntry));
    }
    
    if (strcmp(ip_table[found_idx].ip, ip) != 0) {
        strncpy(ip_table[found_idx].ip, ip, 45);
        ip_table[found_idx].ip[45] = '\0';
        ip_table[found_idx].count = 1;
        ip_table[found_idx].last_request_time = now;
    } else {
        if (ip_table[found_idx].last_request_time == now) {
            ip_table[found_idx].count++;
            if (ip_table[found_idx].count > max_req) {
                allowed = 0; // Limit asildi
            }
        } else {
            ip_table[found_idx].count = 1;
            ip_table[found_idx].last_request_time = now;
        }
    }

#ifdef _WIN32
    LeaveCriticalSection(&rl_mutex);
#else
    pthread_mutex_unlock(&rl_mutex);
#endif

    return allowed;
}

int rate_limit_middleware(Request *req, Response *res) {
    if (!g_app || g_app->max_requests_per_second <= 0) return 1;
    
    if (!rate_limiter_check(req->client_ip, g_app->max_requests_per_second)) {
        response_status(res, 429);
        response_text(res, "Too Many Requests");
        return 0; // stop
    }
    return 1; // pass
}
