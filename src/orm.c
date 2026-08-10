#include "orm.h"
#include "database.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Yardimci fonksiyon: SQLite turunu ORM turune cevirir
static const char* orm_type_to_sql(OrmFieldType type) {
    switch(type) {
        case ORM_TYPE_INT: return "INTEGER";
        case ORM_TYPE_STRING: return "TEXT";
        case ORM_TYPE_FLOAT: return "REAL";
        default: return "TEXT";
    }
}

bool orm_auto_migrate(OrmModel *model) {
    if (!model || !model->fields) return false;
    
    char sql[2048];
    int offset = snprintf(sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS %s (", model->table_name);
    
    for (int i = 0; model->fields[i].name != NULL; i++) {
        OrmField *f = &model->fields[i];
        offset += snprintf(sql + offset, sizeof(sql) - offset, 
                           "%s%s %s %s", 
                           (i == 0) ? "" : ", ",
                           f->name, 
                           orm_type_to_sql(f->type),
                           f->sql_opts ? f->sql_opts : "");
    }
    
    snprintf(sql + offset, sizeof(sql) - offset, ");");
    
    printf("[ORM] Auto-Migrate: %s\n", sql);
    return db_execute(sql);
}

bool orm_insert(OrmModel *model, void *struct_ptr) {
    if (!model || !struct_ptr) return false;
    sqlite3 *db = db_get_instance();
    if (!db) return false;
    
    char sql[2048];
    char cols[1024] = {0};
    char vals[1024] = {0};
    int col_offset = 0;
    int val_offset = 0;
    
    int field_count = 0;
    for (int i = 0; model->fields[i].name != NULL; i++) {
        if (strcmp(model->fields[i].name, "id") == 0) continue; 
        
        col_offset += snprintf(cols + col_offset, sizeof(cols) - col_offset, 
                              "%s%s", (field_count == 0) ? "" : ", ", model->fields[i].name);
        val_offset += snprintf(vals + val_offset, sizeof(vals) - val_offset, 
                              "%s?", (field_count == 0) ? "" : ", ");
        field_count++;
    }
    
    snprintf(sql, sizeof(sql), "INSERT INTO %s (%s) VALUES (%s);", model->table_name, cols, vals);
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[ORM] Hata: %s\n", sqlite3_errmsg(db));
        return false;
    }
    
    int bind_idx = 1;
    for (int i = 0; model->fields[i].name != NULL; i++) {
        OrmField *f = &model->fields[i];
        if (strcmp(f->name, "id") == 0) continue;
        
        void *field_ptr = (char*)struct_ptr + f->offset;
        
        switch (f->type) {
            case ORM_TYPE_INT:
                sqlite3_bind_int(stmt, bind_idx, *(int*)field_ptr);
                break;
            case ORM_TYPE_STRING:
                sqlite3_bind_text(stmt, bind_idx, (char*)field_ptr, -1, SQLITE_TRANSIENT);
                break;
            case ORM_TYPE_FLOAT:
                sqlite3_bind_double(stmt, bind_idx, *(float*)field_ptr);
                break;
        }
        bind_idx++;
    }
    
    bool success = true;
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("[ORM] Insert Hatasi: %s\n", sqlite3_errmsg(db));
        success = false;
    } else {
        for (int i = 0; model->fields[i].name != NULL; i++) {
            if (strcmp(model->fields[i].name, "id") == 0) {
                int *id_ptr = (int*)((char*)struct_ptr + model->fields[i].offset);
                *id_ptr = (int)sqlite3_last_insert_rowid(db);
                break;
            }
        }
    }
    
    sqlite3_finalize(stmt);
    return success;
}

bool orm_find_by_id(OrmModel *model, int id, void *out_struct_ptr) {
    if (!model || !out_struct_ptr) return false;
    sqlite3 *db = db_get_instance();
    if (!db) return false;
    
    char sql[256];
    // Aslinda prepared statement kullaniyoruz:
    snprintf(sql, sizeof(sql), "SELECT * FROM %s WHERE id = ?;", model->table_name);
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        for (int i = 0; model->fields[i].name != NULL; i++) {
            OrmField *f = &model->fields[i];
            void *field_ptr = (char*)out_struct_ptr + f->offset;
            
            int col_idx = -1;
            for (int j = 0; j < sqlite3_column_count(stmt); j++) {
                if (strcmp(sqlite3_column_name(stmt, j), f->name) == 0) {
                    col_idx = j;
                    break;
                }
            }
            
            if (col_idx != -1) {
                switch (f->type) {
                    case ORM_TYPE_INT:
                        *(int*)field_ptr = sqlite3_column_int(stmt, col_idx);
                        break;
                    case ORM_TYPE_STRING:
                        strncpy((char*)field_ptr, (const char*)sqlite3_column_text(stmt, col_idx), f->size - 1);
                        ((char*)field_ptr)[f->size - 1] = '\0';
                        break;
                    case ORM_TYPE_FLOAT:
                        *(float*)field_ptr = (float)sqlite3_column_double(stmt, col_idx);
                        break;
                }
            }
        }
    }
    
    sqlite3_finalize(stmt);
    return found;
}

bool orm_update(OrmModel *model, void *struct_ptr) {
    if (!model || !struct_ptr) return false;
    sqlite3 *db = db_get_instance();
    if (!db) return false;
    
    char sql[2048];
    char sets[1024] = {0};
    int set_offset = 0;
    int field_count = 0;
    
    int id_value = -1;
    
    for (int i = 0; model->fields[i].name != NULL; i++) {
        if (strcmp(model->fields[i].name, "id") == 0) {
            id_value = *(int*)((char*)struct_ptr + model->fields[i].offset);
            continue;
        }
        
        set_offset += snprintf(sets + set_offset, sizeof(sets) - set_offset, 
                              "%s%s = ?", (field_count == 0) ? "" : ", ", model->fields[i].name);
        field_count++;
    }
    
    if (id_value == -1) return false; // ID alani bulunamadi
    
    snprintf(sql, sizeof(sql), "UPDATE %s SET %s WHERE id = ?;", model->table_name, sets);
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }
    
    int bind_idx = 1;
    for (int i = 0; model->fields[i].name != NULL; i++) {
        OrmField *f = &model->fields[i];
        if (strcmp(f->name, "id") == 0) continue;
        
        void *field_ptr = (char*)struct_ptr + f->offset;
        
        switch (f->type) {
            case ORM_TYPE_INT:
                sqlite3_bind_int(stmt, bind_idx, *(int*)field_ptr);
                break;
            case ORM_TYPE_STRING:
                sqlite3_bind_text(stmt, bind_idx, (char*)field_ptr, -1, SQLITE_TRANSIENT);
                break;
            case ORM_TYPE_FLOAT:
                sqlite3_bind_double(stmt, bind_idx, *(float*)field_ptr);
                break;
        }
        bind_idx++;
    }
    
    // WHERE id = ?
    sqlite3_bind_int(stmt, bind_idx, id_value);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool orm_delete(OrmModel *model, int id) {
    if (!model) return false;
    sqlite3 *db = db_get_instance();
    if (!db) return false;
    
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM %s WHERE id = ?;", model->table_name);
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}
