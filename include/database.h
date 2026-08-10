#ifndef COVA_DATABASE_H
#define COVA_DATABASE_H

#include "cJSON.h"
#include <stdbool.h>

// Connects to the database (Creates file if it doesn't exist)
bool db_init(const char *filename);

// Closes the database connection
void db_close(void);

// Executes statements that don't return results (INSERT, UPDATE, DELETE, CREATE TABLE, etc.)
bool db_execute(const char *sql);

// Executes a SELECT query, returning the results as a JSON Array
typedef struct cJSON Json;
Json* db_query(const char *sql);

// Returns the raw SQLite connection instance (for advanced modules like ORM)
struct sqlite3;
struct sqlite3* db_get_instance(void);

#endif // COVA_DATABASE_H
