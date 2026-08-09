#include "database.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>

static sqlite3 *db = NULL;

bool db_init(const char *filename) {
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        printf("[DATABASE] Hata: Veritabani acilamadi -> %s\n", sqlite3_errmsg(db));
        return false;
    }
    printf("[DATABASE] SQLite veritabani (%s) basariyla acildi!\n", filename);
    return true;
}

void db_close(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

bool db_execute(const char *sql) {
    if (!db) return false;
    
    char *err_msg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        printf("[DATABASE] Sorgu Hatasi (Execute): %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

// SQLite'ın her bir satırı okuduğunda tetiklediği geri çağırma (callback) fonksiyonu
static int query_callback(void *data, int argc, char **argv, char **azColName) {
    Json *json_array = (Json*)data;
    Json *row = cJSON_CreateObject();
    
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            // Güvenlik ve basitlik adına tüm değerleri string olarak alıyoruz
            cJSON_AddStringToObject(row, azColName[i], argv[i]);
        } else {
            cJSON_AddNullToObject(row, azColName[i]);
        }
    }
    
    cJSON_AddItemToArray(json_array, row);
    return 0; // 0: İşleme devam et
}

Json* db_query(const char *sql) {
    if (!db) return NULL;
    
    Json *json_array = cJSON_CreateArray();
    char *err_msg = NULL;
    
    // query_callback fonksiyonu, her satır için json_array'e yeni obje ekleyecek
    if (sqlite3_exec(db, sql, query_callback, (void*)json_array, &err_msg) != SQLITE_OK) {
        printf("[DATABASE] Sorgu Hatasi (Query): %s\n", err_msg);
        sqlite3_free(err_msg);
        cJSON_Delete(json_array);
        return NULL;
    }
    
    return json_array;
}
