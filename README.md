<div align="center">
  <img width="2720" height="960" alt="cova_logo" src="https://github.com/user-attachments/assets/8f910354-76d5-49df-a432-57ec1368e330" />
  <h1>Cova Framework</h1>
  <p><strong>A high-performance, enterprise-grade Web Framework written in pure C.</strong></p>

  [![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](#)
  [![C11 Standard](https://img.shields.io/badge/Language-C11-00599C.svg)](#)
  [![Docker Ready](https://img.shields.io/badge/Docker-Ready-2496ED.svg)](#)
</div>

<br>

Cova Framework is an ultra-fast, lightweight, and incredibly feature-rich web framework designed for the C programming language. It brings modern web development paradigms (like ORMs, JWTs, and WebSockets) to the raw performance of C.

Whether you are building high-throughput microservices, IoT backends, or real-time applications, Cova provides the tools you need without the bloat.

---

## ✨ Features

- **⚡ ThreadPool Concurrency:** Efficiently handles thousands of concurrent requests using a custom, highly optimized ThreadPool.
- **🛡️ Custom ORM (Object-Relational Mapping):** Say goodbye to manual SQL strings! Map your C structs directly to SQLite tables using our powerful macro-based ORM.
- **🔐 JWT Authentication:** Built-in middleware for issuing and cryptographically verifying JSON Web Tokens.
- **⛔ Rate Limiting:** Protect your endpoints from DDoS and brute-force attacks with a robust Fixed Window rate limiter.
- **🗜️ GZIP Compression:** Automatic response compression to significantly reduce bandwidth usage.
- **🔌 WebSockets:** Native support for real-time bidirectional communication.
- **📁 Multipart File Uploads:** Easily parse and handle `multipart/form-data` file uploads.
- **🛠️ Memory Tracker:** A built-in, thread-safe memory leak detector ensures your application stays rock-solid in production.
- **🐳 Docker Native:** Containerize and deploy your application in seconds with the provided Docker setup.

---

## 🚀 Quick Start

### Option 1: Docker (Recommended)
The easiest way to get started is by using Docker. No dependencies required!

```bash
# Clone the repository
git clone https://github.com/TurkerAlbayrak/Cova.git
cd Cova

# Build and start the container
docker compose up --build
```
Your server is now running at `http://localhost:8080`.

### Option 2: Native Build
If you prefer to build natively, ensure you have GCC and OpenSSL installed.

```bash
# Windows (Requires MSYS2/MinGW-w64)
.\build.bat

# Linux / MacOS
make

# Run the server
./server
```

---

## 💻 Code Examples

### 1. Basic Routing & Responses
Setting up a simple JSON API endpoint is incredibly straightforward.

```c
void hello_handler(Request *req, Response *res) {
    response_json(res, "{\"message\": \"Hello from Cova Framework!\"}");
}

int main(void) {
    App app;
    app_init(&app);
    
    app_get(&app, "/api/hello", hello_handler);
    app_run(&app, 8080);
    return 0;
}
```

### 2. Using the ORM
Cova includes a powerful ORM that automatically converts your C structs into SQLite tables.

```c
typedef struct {
    int id;
    char username[50];
    int age;
} User;

// Define the fields and their SQL constraints
ORM_FIELD_MAP(User) = {
    ORM_INT_FIELD(User, id, "PRIMARY KEY AUTOINCREMENT"),
    ORM_STRING_FIELD(User, username, 50, "NOT NULL UNIQUE"),
    ORM_INT_FIELD(User, age, "DEFAULT 18"),
    ORM_END_FIELDS
};

// Register the model
ORM_MODEL(UserModel, "users", User, __orm_fields_User);

void create_user_handler(Request *req, Response *res) {
    User new_user = {0, "john_doe", 28};
    if (orm_insert(&UserModel, &new_user)) {
        printf("Inserted User ID: %d\n", new_user.id);
        response_text(res, "User created successfully!");
    }
}
```

### 3. JWT & Middlewares
Protect your sensitive endpoints easily using the built-in JWT middleware.

```c
void protected_handler(Request *req, Response *res) {
    // This route is now protected! Only valid JWTs can enter.
    if (!jwt_middleware(req, res)) return;
    
    response_text(res, "Welcome to the protected zone!");
}

int main(void) {
    App app;
    app_init(&app);
    app_set_jwt_secret(&app, "my_super_secret_key");
    
    app_get(&app, "/api/protected", protected_handler);
    app_run(&app, 8080);
}
```

### 4. WebSockets
Real-time chat? Data streaming? Cova handles WebSockets natively.

```c
void websocket_handler(Request *req, Response *res) {
    if (ws_handshake(req, res)) {
        printf("Client connected via WebSocket!\n");
        ws_send_text(res->client_socket, "Welcome to the Cova WS Server!");
        
        while (1) {
            char *msg = ws_read_frame(res->client_socket);
            if (!msg) break; // Client disconnected
            
            // Echo the message back
            ws_send_text(res->client_socket, msg);
            cova_free(msg);
        }
    }
}
```

---

## 🛠️ Usage in Your Own Projects

Want to use Cova Framework to build your own C projects? Follow these simple steps:

### 1. Project Structure
Create a new folder for your project and copy the essential Cova directories into it. Your project should look like this:

```
my_cova_project/
├── include/           # Copy from Cova Framework
├── src/               # Copy from Cova Framework (cova.c, server.c, orm.c, etc.)
├── main.c             # Your application entry point
└── Makefile           # Your build script
```

### 2. Basic `main.c` Template
Create a `main.c` file and include the main framework header:

```c
#include "cova.h"

void home_handler(Request *req, Response *res) {
    response_text(res, "Welcome to my Cova-powered app!");
}

int main(void) {
    App app;
    app_init(&app);
    
    app_get(&app, "/", home_handler);
    
    printf("Starting server on port 8080...\n");
    app_run(&app, 8080);
    return 0;
}
```

### 3. Compilation
When compiling your project, ensure you link the necessary libraries. 

**Linux / macOS:**
```bash
gcc -Wall -Wextra -Iinclude main.c src/*.c -o my_app -lssl -lcrypto -lz -lpthread
```

**Windows (MSYS2/MinGW):**
```bash
gcc -Wall -Wextra -Iinclude main.c src/*.c -o my_app.exe -lws2_32 -lssl -lcrypto -lz
```

---

## ⚡ Hot-Reloading (cova_watch.sh)

Tired of manually stopping the server, recompiling, and starting it again every time you change a line of code? 

Cova includes a built-in Hot-Reloading script: **`cova_watch.sh`**

This script constantly watches your `.c`, `.h`, and `.html` files for changes. When you save a file, it automatically recompiles your code and restarts the server instantly!

### How to use it:
1. Ensure you have `bash` and `cmake` installed.
2. Make the script executable (Linux/macOS):
   ```bash
   chmod +x cova_watch.sh
   ```
3. Run the script:
   ```bash
   ./cova_watch.sh
   ```
4. Start coding! Whenever you save a file, the watcher will print `[COVA WATCHER] Degisiklik algilandi! Yeniden baslatiliyor...` and refresh your server in milliseconds. To stop the watcher, simply press `CTRL+C`.

> **Note:** `cova_watch.sh` uses `cmake` to build the `cova_server` executable by default. Make sure your `CMakeLists.txt` is properly configured for your project if you modify the file structure.

---

## 📖 Documentation

For an in-depth guide on every feature, API reference, and advanced tutorials, please visit our **[Official Documentation Site](./docs/index.html)** (Open `docs/index.html` in your browser).

---

