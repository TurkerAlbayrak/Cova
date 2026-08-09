cmake_minimum_required(VERSION 3.10)
project(CovaFramework VERSION 1.0.0 LANGUAGES C)

# C Standardı (C11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Uyarıları (Warnings) açalım, ancak sqlite3'ün uyarılarını kapatalım
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-sign-compare")

# Cova Framework Kaynak Dosyaları
set(COVA_SOURCES
    src/server.c
    src/request.c
    src/response.c
    src/cJSON.c
    src/mime.c
    src/memtrack.c
    src/database.c
    src/sqlite3.c
    src/sha1.c
    src/base64.c
    src/websocket.c
)

# 1. Cova'yı Statik Bir Kütüphane (Library) olarak derle
add_library(cova STATIC ${COVA_SOURCES})
target_include_directories(cova PUBLIC include)

# Windows için Winsock (ws2_32) kütüphanesini bağla
if(WIN32)
    target_link_libraries(cova ws2_32)
endif()

# Linux/Mac için pthreads bağla
if(UNIX)
    target_link_libraries(cova pthread)
endif()

# 2. Ana Uygulama (Executable)
add_executable(cova_server examples/main.c)

# Ana uygulamaya Cova kütüphanesini bağla
target_link_libraries(cova_server cova)
