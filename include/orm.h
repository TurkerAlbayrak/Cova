#ifndef COVA_ORM_H
#define COVA_ORM_H

#include <stddef.h>
#include <stdbool.h>

// Supported data types
typedef enum {
    ORM_TYPE_INT,
    ORM_TYPE_STRING,
    ORM_TYPE_FLOAT
} OrmFieldType;

// Represents a database table column (C struct field)
typedef struct {
    const char *name;      // Column name (Struct field name)
    OrmFieldType type;     // Data type
    size_t offset;         // Memory offset in struct (offsetof)
    size_t size;           // Data size (max length for strings, etc.)
    const char *sql_opts;  // SQL constraints like "PRIMARY KEY AUTOINCREMENT"
} OrmField;

// Represents a database table (C struct model)
typedef struct {
    const char *table_name;
    size_t struct_size;
    OrmField *fields;      // Array ending with name=NULL
} OrmModel;

// ==========================================
// ORM MACROS (DEVELOPER EXPERIENCE)
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
// ORM DATABASE OPERATIONS
// ==========================================

// Automatically creates the table if it does not exist
bool orm_auto_migrate(OrmModel *model);

// Inserts a new record (if id is autoincrement, writes the new ID back into the struct)
bool orm_insert(OrmModel *model, void *struct_ptr);

// Searches by ID and populates out_struct_ptr with the result
bool orm_find_by_id(OrmModel *model, int id, void *out_struct_ptr);

// Updates the record (using the ID field)
bool orm_update(OrmModel *model, void *struct_ptr);

// Deletes the record (using the ID field)
bool orm_delete(OrmModel *model, int id);

#endif // COVA_ORM_H
