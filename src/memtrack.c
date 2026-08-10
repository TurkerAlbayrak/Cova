#include "memtrack.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

// İstatistik sayaçları
static int current_allocs = 0; // Şu an bellekte olan obje sayısı
static int total_allocs = 0;   // Toplam malloc çağrısı
static int total_frees = 0;    // Toplam free çağrısı
static int mem_initialized = 0;

#ifdef _WIN32
static CRITICAL_SECTION mem_mutex;
#else
static pthread_mutex_t mem_mutex;
#endif

void cova_memtrack_init() {
    if (!mem_initialized) {
#ifdef _WIN32
        InitializeCriticalSection(&mem_mutex);
#else
        pthread_mutex_init(&mem_mutex, NULL);
#endif
        mem_initialized = 1;
    }
}

void* cova_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr) {
        if (!mem_initialized) cova_memtrack_init();
#ifdef _WIN32
        EnterCriticalSection(&mem_mutex);
#else
        pthread_mutex_lock(&mem_mutex);
#endif
        current_allocs++;
        total_allocs++;
#ifdef _WIN32
        LeaveCriticalSection(&mem_mutex);
#else
        pthread_mutex_unlock(&mem_mutex);
#endif
    }
    return ptr;
}

void cova_free(void *ptr) {
    if (ptr) {
        if (!mem_initialized) cova_memtrack_init();
#ifdef _WIN32
        EnterCriticalSection(&mem_mutex);
#else
        pthread_mutex_lock(&mem_mutex);
#endif
        current_allocs--;
        total_frees++;
#ifdef _WIN32
        LeaveCriticalSection(&mem_mutex);
#else
        pthread_mutex_unlock(&mem_mutex);
#endif
        free(ptr);
    }
}

void cova_mem_report(void) {
    printf("\n\n==========================================\n");
    printf("[V16] MEMORY LEAK REPORT\n");
    printf("==========================================\n");
    printf("- Total Allocations (Malloc) : %d count\n", total_allocs);
    printf("- Total Frees (Free): %d count\n", total_frees);
    printf("------------------------------------------\n");
    if (current_allocs == 0) {
        printf(">> RESULT: FLAWLESS! (0 Memory Leaks) <<\n");
    } else {
        printf(">> SONUC: TEHLIKE! %d count bellek temizlenmedi! <<\n", current_allocs);
    }
    printf("==========================================\n\n");
}
