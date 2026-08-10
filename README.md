# Cova Framework

<p align="center">
  <img width="1440" height="480" alt="image" src="https://github.com/user-attachments/assets/340957cd-2573-4a63-86f0-0584d18da096" />
</p>

## Screenshots

<img width="677" height="297" alt="1" src="https://github.com/user-attachments/assets/6ae0fb19-441e-4711-afb8-3d5165fef5d2" />
<img width="1562" height="687" alt="2" src="https://github.com/user-attachments/assets/599685a9-f440-4a6a-8166-ab7e4f9e0b4a" />
<img width="595" height="181" alt="3" src="https://github.com/user-attachments/assets/28ea28f1-2ae3-4f32-bc70-4f4b31fca93b" />
<img width="722" height="255" alt="4" src="https://github.com/user-attachments/assets/514988db-e15b-4ba8-a6c9-aae6872cdc71" />
<img width="592" height="207" alt="5" src="https://github.com/user-attachments/assets/6c51d62a-c1e5-48eb-9790-a3e38f649fd2" />
<img width="1665" height="895" alt="6" src="https://github.com/user-attachments/assets/bd746b28-3117-4083-9918-2902dd790cb6" />
<img width="1027" height="1031" alt="7" src="https://github.com/user-attachments/assets/81f933eb-fbc3-4b52-85b9-ef298e7c48a0" />
<img width="1027" height="1031" alt="7" src="https://github.com/user-attachments/assets/e854b14a-c68f-431e-9611-828cca8d8075" />
<img width="1452" height="567" alt="8" src="https://github.com/user-attachments/assets/72058f32-3cda-4f6a-b5b2-04ecf649c0e6" />

Cova is a high-performance, lightweight, and robust C Web Framework designed for building modern web applications, RESTful APIs, and real-time services. It handles everything from socket management to database operations internally, without relying on bloated external dependencies.

## Key Features

*   **Zero Memory Leak Guarantee:** Features a built-in memory tracker that intercepts all memory allocations and deallocations, ensuring that every allocated byte is freed.
*   **Routing System:** Dynamic routing mechanism supporting URL parameters (e.g., `/users/:id`) and query parameters (e.g., `?q=search`).
*   **Middleware Architecture:** Easily intercept incoming HTTP requests to handle logging, authentication, and authorization before reaching the endpoint handlers.
*   **Built-in SQLite Integration:** Ships with an embedded SQLite engine and provides seamless, automated conversion between SQL queries and JSON objects.
*   **WebSockets (Real-Time):** Native implementation of the RFC 6455 WebSocket protocol. Handles Handshake, SHA-1 hashing, Base64 encoding, and Masking internally.
*   **Static File & Template Server:** Deliver static assets or use the built-in HTML template engine to inject context variables dynamically.
*   **Hot-Reloading:** A dedicated Bash script that automatically detects changes in source code, recompiles the framework, and restarts the server instantly.

---

## Installation and Build Instructions

Cova uses CMake as its primary build system to ensure cross-platform compatibility and professional library structuring.

### Requirements
*   CMake (Version 3.10 or higher)
*   GCC or MSVC Compiler

### Build Steps

1.  Clone the repository and navigate to the root directory.
2.  Generate the build files using CMake:
    ```bash
    cmake -B build
    ```
3.  Compile the project:
    ```bash
    cmake --build build
    ```
4.  Run the executable:
    ```bash
    ./build/cova_server
    ```

### Hot-Reloading (Development Mode)

When developing your application, you do not need to manually recompile every time you change a file. Execute the provided watcher script:

```bash
./cova_watch.sh
```

This script monitors all `.c`, `.h`, and `.html` files. Upon detecting a save action, it instantly terminates the active server, recompiles the codebase, and relaunches the application.

---

## Integrating Cova into Your Own Project

If you want to use the Cova Framework to build your own application, the most modern and robust method is to include it as a CMake subdirectory.

### Step 1: Add Cova as a Submodule
Inside your own project directory, run:
```bash
git submodule add https://github.com/TurkerAlbayrak/Cova.git vendor/cova
```

### Step 2: Update Your CMakeLists.txt
Add the Cova directory and link the library to your main executable:

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyAwesomeApp VERSION 1.0.0 LANGUAGES C)

# Add Cova Framework
add_subdirectory(vendor/cova)

# Your application
add_executable(my_app src/main.c)

# Link Cova to your application
target_link_libraries(my_app cova)
```

### Step 3: Include the Header
Now, you can include Cova anywhere in your source code:
```c
#include "cova.h"

App app;
// Build your handlers...
```

---

## Detailed Usage Guide

The `examples/main.c` file serves as the core entry point for testing capabilities. Below are detailed examples of how to utilize the various components of the Cova Framework.

### 1. Application Initialization

To start a server, you must initialize an `App` instance and invoke `app_run`.

```c
#include "cova.h"

App app;

int main(void) {
    app_init(&app);
    app_run(&app, 8080); // Listens on port 8080
    return 0;
}
```

### 2. Basic Routing and Handlers

Routes are defined using methods like `app_get` and `app_post`. A handler function always takes `Request` and `Response` pointers.

```c
void hello_handler(Request *req, Response *res) {
    response_header(res, "Content-Type", "text/plain");
    response_text(res, "Hello, World!");
}

int main(void) {
    app_init(&app);
    app_get(&app, "/hello", hello_handler);
    app_run(&app, 8080);
    return 0;
}
```

### 3. Middleware Implementation

Middlewares execute sequentially before the request reaches the target handler. Returning `1` allows the chain to continue; returning `0` halts the execution (useful for unauthorized requests).

```c
int logger_middleware(Request *req, Response *res) {
    printf("[LOGGER] Incoming request to: %s\n", req->path);
    return 1;
}

int main(void) {
    app_init(&app);
    app_use(&app, logger_middleware);
    // ... routes
}
```

### 4. Serving Static Files

You can map a URL prefix directly to a local file system directory.

```c
int main(void) {
    app_init(&app);
    // Any request to /public/* will serve files from the ./public directory
    app_static(&app, "/public", "./public"); 
    app_run(&app, 8080);
}
```

### 5. Returning JSON Responses

Cova utilizes the cJSON library internally, which is deeply integrated with the framework's memory tracker.

```c
void api_handler(Request *req, Response *res) {
    Json *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "status", "success");
    
    response_status(res, 200);
    response_json_object(res, json);
    
    cJSON_Delete(json); // Necessary to prevent memory leaks
}
```

### 6. SQLite Database Integration

The `db_query` function automatically connects to the database and maps the resulting rows into a JSON array, making it extremely easy to build REST APIs.

```c
void get_users_handler(Request *req, Response *res) {
    const char *sql = "SELECT id, name FROM users;";
    Json *rows = db_query(sql);
    
    if (rows) {
        response_json_object(res, rows);
        cJSON_Delete(rows);
    }
}

int main(void) {
    db_init("application.db"); // Initialize the database file
    app_init(&app);
    app_get(&app, "/users", get_users_handler);
    app_run(&app, 8080);
}
```

### 7. WebSockets

Cova handles the intricate details of WebSocket handshakes and framing. You can interact with the client using a blocking loop.

```c
void chat_handler(Request *req, Response *res) {
    if (!ws_handshake(req, res)) {
        response_status(res, 400);
        return;
    }
    
    while (1) {
        char *msg = ws_read_frame(res->client_socket);
        if (!msg) break; // Client disconnected
        
        ws_send_text(res->client_socket, "Message Received!");
        cova_free(msg);
    }
}
```

### 8. Custom Error Handlers

You can override the default 404 (Not Found) or 500 (Internal Server Error) behaviors.

```c
void custom_404(Request *req, Response *res) {
    response_status(res, 404);
    response_text(res, "Resource could not be found.");
}

int main(void) {
    app_init(&app);
    app_on_404(&app, custom_404);
    app_run(&app, 8080);
}
```

---

## Memory Tracker Report

When the server process receives an interrupt signal (SIGINT / CTRL+C), the framework safely shuts down all active threads and prints a final memory analysis report. If the number of `malloc` operations perfectly matches the number of `free` operations, you will see a `0 Bellek Sizintisi` (0 Memory Leak) validation on the terminal. Ensure you always use `cova_malloc` and `cova_free` within your handler logic to utilize this feature.
