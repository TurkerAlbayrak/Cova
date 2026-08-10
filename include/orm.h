#ifndef COVA_ORM_H
#define COVA_ORM_H

#include <stddef.h>
#include <stdbool.h>

// Desteklenen veri tipleri
typedef enum {
    ORM_TYPE_INT,
    ORM_TYPE_STRING,
    ORM_TYPE_FLOAT
} OrmFieldType;

// Bir tablo sutununu (C struct alani) temsil eden yapi
typedef struct {
    const char *name;      // Sutun adi (Struct alani adi)
    OrmFieldType type;     // Veri tipi
    size_t offset;         // Struct icindeki hafiza ofseti (offsetof)
    size_t size;           // Veri boyutu (stringler icin maksimum uzunluk vb.)
    const char *sql_opts;  // "PRIMARY KEY AUTOINCREMENT" gibi SQL kisitlamalari
} OrmField;

// Bir veritabani tablosunu (C struct) temsil eden model yapisi
typedef struct {
    const char *table_name;
    size_t struct_size;
    OrmField *fields;      // Son elemani name=NULL olan dizi
} OrmModel;

// ==========================================
// ORM MAKROLARI (GELİŞTİRİCİ DENEYİMİ İÇİN)
// ==========================================

#define ORM_FIELD_MAP(StructName) \
    OrmField __orm_fields_##StructName[]

#define ORM_INT_FIELD(Struct, Field, Options) \
    {#Field, ORM_TYPE_INT, offsetof(Struct, Field), sizeof(int), Options}

#define ORM_STRING_FIELD(Struct, Field, MaxLen, Options) \
    {#Field, ORM_TYPE_STRING, offsetof(Struct, Field), MaxLen, Options}
    
#define ORM_FLOAT_FIELD(Struct, Field, Options) \
    {#Field, ORM_TYPE_FLOAT, offsetof(Struct, Field), sizeof(float), Options}

#define ORM_END_FIELDS {NULL, ORM_TYPE_INT, 0, 0, NULL}

#define ORM_MODEL(ModelName, TableName, StructName, FieldArray) \
    OrmModel ModelName = { \
        TableName, \
        sizeof(StructName), \
        FieldArray \
    }

// ==========================================
// ORM VERITABANI ISLEMLERI
// ==========================================

// Tabloyu eger yoksa otomatik olusturur
bool orm_auto_migrate(OrmModel *model);

// Yeni bir kayit ekler (id degeri autoincrement ise, struct icine yeni ID'yi geri yazar)
bool orm_insert(OrmModel *model, void *struct_ptr);

// ID'ye gore arama yapar ve sonucu out_struct_ptr icerisine doldurur
bool orm_find_by_id(OrmModel *model, int id, void *out_struct_ptr);

// Kaydi gunceller (ID alani uzerinden)
bool orm_update(OrmModel *model, void *struct_ptr);

// Kaydi siler (ID alani uzerinden)
bool orm_delete(OrmModel *model, int id);

#endif // COVA_ORM_H
