#include "memtrack.h"
#include <stdlib.h>
#include <stdio.h>

// İstatistik sayaçları
static int current_allocs = 0; // Şu an bellekte olan obje sayısı
static int total_allocs = 0;   // Toplam malloc çağrısı
static int total_frees = 0;    // Toplam free çağrısı

void* cova_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr) {
        current_allocs++;
        total_allocs++;
    }
    return ptr;
}

void cova_free(void *ptr) {
    if (ptr) {
        current_allocs--;
        total_frees++;
        free(ptr);
    }
}

void cova_mem_report(void) {
    printf("\n\n==========================================\n");
    printf("[V16] MEMORY LEAK (BELLEK SIZINTISI) RAPORU\n");
    printf("==========================================\n");
    printf("- Toplam Tahsis (Malloc) : %d adet\n", total_allocs);
    printf("- Toplam Temizleme (Free): %d adet\n", total_frees);
    printf("------------------------------------------\n");
    if (current_allocs == 0) {
        printf(">> SONUC: KUSURSUZ! (0 Bellek Sizintisi) <<\n");
    } else {
        printf(">> SONUC: TEHLIKE! %d adet bellek temizlenmedi! <<\n", current_allocs);
    }
    printf("==========================================\n\n");
}
