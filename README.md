# Cova Framework

![Cova Logo](https://img.shields.io/badge/Cova-Framework-blue?style=for-the-badge&logo=c)
![Build](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-blue)

<img width="1440" height="480" alt="image" src="https://github.com/user-attachments/assets/94fec5a4-54fc-432d-a1be-254f62be45b8" />


Cova is a blazing fast, lightweight, and modern web framework written purely in C. Designed with zero external dependencies (other than OpenSSL for HTTPS), Cova brings the elegance of modern web frameworks to the raw performance of C.

## 🚀 Features

Cova is packed with enterprise-grade features, built from scratch to guarantee maximum performance and zero memory leaks.

- **⚡ ThreadPool Concurrency:** Efficiently handles thousands of concurrent requests using a custom ThreadPool.
- **🛡️ Custom ORM (Object-Relational Mapping):** Say goodbye to manual SQL queries! Map your C structs directly to SQLite tables using our powerful macro-based ORM.
- **🔐 JWT Authentication:** Built-in middleware for issuing and verifying JSON Web Tokens.
- **⛔ Rate Limiting:** Protect your endpoints from DDoS attacks with a robust Fixed Window rate limiter.
- **🗜️ GZIP Compression:** Automatic response compression to save bandwidth.
- **🔌 WebSockets:** Native support for real-time bidirectional communication.
- **📁 Multipart File Uploads:** Easily handle `multipart/form-data` requests.
- **🛠️ Memory Tracker:** A built-in memory leak detector ensures your application stays rock-solid in production.

---

## 🛠️ Quick Start

### 1. Requirements
- **Windows:** MSYS2 / MinGW-w64 (GCC)
- **Linux/Mac:** GCC and Make
- **OpenSSL:** Optional, for HTTPS support.

### 2. Build the Framework
Use the provided build script to compile the framework and the example application.

```bash
# Windows
.\build.bat

# Linux / MacOS
make
```

### 3. Run the Server
```bash
./server.exe
```

The server will start listening on `http://localhost:8080`.

---

## 💻 Code Examples

### Defining a Route & JSON Response

```c
void hello_handler(Request *req, Response *res) {
    response_json(res, "{\"message\": \"Welcome to Cova Framework!\"}");
}

int main(void) {
    App app;
    app_init(&app);
    
    app_get(&app, "/", hello_handler);
    app_run(&app, 8080);
    return 0;
}
```

### Using the Cova ORM

Define your C struct and map it to a database table:

```c
typedef struct {
    int id;
    char username[50];
    int age;
} User;

// ORM Metadata Mapping
ORM_FIELD_MAP(User) = {
    ORM_INT_FIELD(User, id, "PRIMARY KEY AUTOINCREMENT"),
    ORM_STRING_FIELD(User, username, 50, "NOT NULL UNIQUE"),
    ORM_INT_FIELD(User, age, "DEFAULT 18"),
    ORM_END_FIELDS
};
ORM_MODEL(UserModel, "users", User, __orm_fields_User);
```

Inserting and Querying data:
```c
// Auto-migrate (Creates table if it doesn't exist)
orm_auto_migrate(&UserModel);

// Insert a new record
User new_user = {0, "cova_admin", 35};
orm_insert(&UserModel, &new_user);

// Find by ID (directly into a struct!)
User u;
if (orm_find_by_id(&UserModel, 1, &u)) {
    printf("Found User: %s (Age: %d)\n", u.username, u.age);
}
```

### WebSocket Echo Server
```c
void websocket_chat_handler(Request *req, Response *res) {
    if (!ws_handshake(req, res)) return;
    
    while (1) {
        char *msg = ws_read_frame(res->client_socket);
        if (!msg) break;
        
        char reply[512];
        snprintf(reply, sizeof(reply), "Server Echo: %s", msg);
        ws_send_text(res->client_socket, reply);
        
        cova_free(msg);
    }
}
```

---

## 🏗️ Architecture
Cova is built around a non-blocking network socket architecture. When a client connects, the request is dispatched to a **ThreadPool** worker. This ensures the main server loop is never blocked, allowing for massive concurrency. 

All memory allocations are routed through `cova_malloc` and `cova_free`. Upon graceful shutdown (CTRL+C), the **Memory Tracker** generates a detailed report, guaranteeing a leak-free environment.

## 📄 License
This project is licensed under the MIT License.
