#ifndef COVA_DATABASE_H
#define COVA_DATABASE_H

#include "cJSON.h"
#include <stdbool.h>

// Veritabanına bağlanır (Dosya yoksa oluşturur)
bool db_init(const char *filename);

// Veritabanı bağlantısını kapatır
void db_close(void);

// INSERT, UPDATE, DELETE, CREATE TABLE gibi sonuç döndürmeyen komutları çalıştırır
bool db_execute(const char *sql);

// SELECT sorgusu çalıştırır, sonuçları bizim JSON Array (Liste) formatımıza dönüştürüp döndürür!
typedef struct cJSON Json;
Json* db_query(const char *sql);

#endif // COVA_DATABASE_H
