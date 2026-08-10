#include "cova.h"
#include "cJSON.h"
#include "database.h"
#include "orm.h"
#include "jwt.h"
#include "sqlite3.h"
#include "memtrack.h"
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------
// 1. DATABASE MODELS (ORM)
// ---------------------------------------------------------
typedef struct {
    int id;
    char title[100];
    char content[1024];
    char created_at[50];
} Post;

ORM_FIELD_MAP(Post) = {
    ORM_INT_FIELD(Post, id, "PRIMARY KEY AUTOINCREMENT"),
    ORM_STRING_FIELD(Post, title, 100, "NOT NULL"),
    ORM_STRING_FIELD(Post, content, 1024, "NOT NULL"),
    ORM_STRING_FIELD(Post, created_at, 50, "NOT NULL"),
    ORM_END_FIELDS
};
ORM_MODEL(PostModel, "posts", Post, __orm_fields_Post);

typedef struct {
    int id;
    char username[50];
    char password[50];
} User;

ORM_FIELD_MAP(User) = {
    ORM_INT_FIELD(User, id, "PRIMARY KEY AUTOINCREMENT"),
    ORM_STRING_FIELD(User, username, 50, "NOT NULL UNIQUE"),
    ORM_STRING_FIELD(User, password, 50, "NOT NULL"),
    ORM_END_FIELDS
};
ORM_MODEL(UserModel, "users", User, __orm_fields_User);


// ---------------------------------------------------------
// 2. API HANDLERS
// ---------------------------------------------------------

// GET /api/posts - Get all blog posts
void get_posts_handler(Request *req, Response *res) {
    sqlite3_stmt *stmt;
    char *sql = "SELECT id, title, content, created_at FROM posts ORDER BY id DESC";
    
    if (sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, 0) != SQLITE_OK) {
        response_json(res, "[]");
        return;
    }

    // Build a JSON array manually for simplicity, or use cJSON. We use cJSON.
    cJSON *json_array = cJSON_CreateArray();
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *post = cJSON_CreateObject();
        cJSON_AddNumberToObject(post, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(post, "title", (const char *)sqlite3_column_text(stmt, 1));
        cJSON_AddStringToObject(post, "content", (const char *)sqlite3_column_text(stmt, 2));
        cJSON_AddStringToObject(post, "created_at", (const char *)sqlite3_column_text(stmt, 3));
        cJSON_AddItemToArray(json_array, post);
    }
    sqlite3_finalize(stmt);

    char *json_str = cJSON_PrintUnformatted(json_array);
    response_json(res, json_str);
    
    cova_free(json_str);
    cJSON_Delete(json_array);
}

// POST /api/login - Authenticate admin
void login_handler(Request *req, Response *res) {
    cJSON *body = cJSON_Parse(req->body);
    if (!body) {
        response_json(res, "{\"error\": \"Invalid JSON\"}");
        return;
    }

    cJSON *user = cJSON_GetObjectItem(body, "username");
    cJSON *pass = cJSON_GetObjectItem(body, "password");

    if (user && pass && strcmp(user->valuestring, "admin") == 0 && strcmp(pass->valuestring, "123456") == 0) {
        // Correct credentials -> Generate JWT
        char *token = jwt_generate("{\"role\": \"admin\"}", "cova_super_secret");
        
        char response_buf[512];
        snprintf(response_buf, sizeof(response_buf), "{\"token\": \"%s\"}", token);
        response_json(res, response_buf);
        cova_free(token);
    } else {
        res->status_code = 401; // Unauthorized
        response_json(res, "{\"error\": \"Invalid credentials\"}");
    }
    cJSON_Delete(body);
}

// POST /api/posts - Create a new post (Protected)
void create_post_handler(Request *req, Response *res) {
    // 1. Validate JWT! Only admins can create posts.
    if (!jwt_middleware(req, res)) return;

    // 2. Parse body
    cJSON *body = cJSON_Parse(req->body);
    if (!body) {
        response_json(res, "{\"error\": \"Invalid JSON\"}");
        return;
    }

    cJSON *title = cJSON_GetObjectItem(body, "title");
    cJSON *content = cJSON_GetObjectItem(body, "content");

    if (title && content) {
        Post new_post = {0};
        strncpy(new_post.title, title->valuestring, 99);
        strncpy(new_post.content, content->valuestring, 1023);
        strncpy(new_post.created_at, "Today", 49); // Simplified date

        if (orm_insert(&PostModel, &new_post)) {
            response_json(res, "{\"success\": true}");
        } else {
            res->status_code = 500;
            response_json(res, "{\"error\": \"Database error\"}");
        }
    } else {
        res->status_code = 400;
        response_json(res, "{\"error\": \"Missing title or content\"}");
    }
    
    cJSON_Delete(body);
}


// ---------------------------------------------------------
// 3. MAIN APP
// ---------------------------------------------------------
int main(void) {
    // 1. Initialize Database & Models
    if (!db_init("blog.db")) {
        printf("[DATABASE] Connection failed!\n");
        return 1;
    }
    orm_auto_migrate(&UserModel);
    orm_auto_migrate(&PostModel);

    // Create default admin user if not exists
    User admin = {0, "admin", "123456"};
    orm_insert(&UserModel, &admin); // Will silently fail if username exists due to UNIQUE constraint. That's fine!

    // 2. Initialize App
    App app;
    app_init(&app);
    
    // Set secret for JWT
    app_set_jwt_secret(&app, "cova_super_secret");

    // 3. Register Routes
    app_get(&app, "/api/posts", get_posts_handler);
    app_post(&app, "/api/posts", create_post_handler);
    app_post(&app, "/api/login", login_handler);

    // 4. Serve Static Frontend Files
    app_static(&app, "/", "./sample_project/public/");

    // 5. Start Server
    printf("\n[SYSTEM] Starting Cova Blog on port 8080...\n");
    printf("[SYSTEM] Visit http://localhost:8080/\n\n");
    app_run(&app, 8080);
    
    return 0;
}
