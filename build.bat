@echo off
setlocal

echo Building Cova Framework with HTTPS (OpenSSL) and Thread Pool...

:: Assuming GCC is in PATH.
:: On Windows, if OpenSSL is not available, remove -DUSE_OPENSSL, -lssl and -lcrypto to build without OpenSSL.

set SRC_FILES=src\server.c src\request.c src\response.c src\cJSON.c src\mime.c src\memtrack.c src\database.c src\sqlite3.c src\sha1.c src\base64.c src\websocket.c src\threadpool.c src\jwt.c src\rate_limiter.c src\multipart.c src\orm.c
set INC_DIRS=-Iinclude
set LIBS=-lws2_32 -lssl -lcrypto -lz

echo Compiling server.exe...
gcc -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -DUSE_OPENSSL %INC_DIRS% examples\main.c %SRC_FILES% -o server.exe %LIBS%

if %errorlevel% neq 0 (
    echo Build Failed! Please ensure OpenSSL and GCC are installed and in your MinGW system PATH.
    echo If OpenSSL is missing, try building without -DUSE_OPENSSL, -lssl and -lcrypto parameters.
    exit /b %errorlevel%
)

echo Build Success! server.exe generated.
endlocal
