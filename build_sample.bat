@echo off
echo Building Cova Framework Sample Project (Blog)...

:: Check for OpenSSL
gcc -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -DUSE_OPENSSL -Iinclude sample_project/main.c src/*.c -o sample_server.exe -lws2_32 -lssl -lcrypto -lz
if %errorlevel% neq 0 (
    echo [WARNING] OpenSSL not found or GCC failed. Building without OpenSSL...
    gcc -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Iinclude sample_project/main.c src/*.c -o sample_server.exe -lws2_32 -lz
)

if %errorlevel% equ 0 (
    echo Build Successful! Run sample_server.exe to start the blog.
) else (
    echo Build Failed!
)
