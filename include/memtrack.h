#ifndef COVA_MEMTRACK_H
#define COVA_MEMTRACK_H

#include <stddef.h>

// Kendi Malloc ve Free fonksiyonlarımız
void* cova_malloc(size_t size);
void cova_free(void *ptr);

// Sunucu kapandığında raporu basan fonksiyon
void cova_mem_report(void);

#endif // COVA_MEMTRACK_H
