#ifndef COVA_MEMTRACK_H
#define COVA_MEMTRACK_H

#include <stddef.h>

// Custom Malloc and Free functions
void* cova_malloc(size_t size);
void cova_free(void *ptr);

// Function to print the memory leak report upon server shutdown
void cova_mem_report(void);

#endif // COVA_MEMTRACK_H
