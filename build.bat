@echo off
setlocal

echo Building Cova Framework with HTTPS (OpenSSL) and Thread Pool...

:: GCC'nin PATH'de oldugunu varsayiyoruz.
:: Windows'ta eger OpenSSL yoksa, OpenSSL olmadan derlemek icin -DUSE_OPENSSL ve -lssl -lcrypto parametrelerini siliniz.

set SRC_FILES=src\server.c src\request.c src\response.c src\cJSON.c src\mime.c src\memtrack.c src\database.c src\sqlite3.c src\sha1.c src\base64.c src\websocket.c src\threadpool.c
set INC_DIRS=-Iinclude
set LIBS=-lws2_32 -lssl -lcrypto

echo Compiling server.exe...
gcc -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -DUSE_OPENSSL %INC_DIRS% examples\main.c %SRC_FILES% -o server.exe %LIBS%

if %errorlevel% neq 0 (
    echo Build Failed! Lutfen OpenSSL ve GCC'nin (MinGW) sisteminizde yuklu ve PATH icerisinde oldugundan emin olun.
    echo Eger OpenSSL yoksa, build.bat dosyasindan -DUSE_OPENSSL, -lssl ve -lcrypto parametrelerini cikararak tekrar deneyin.
    exit /b %errorlevel%
)

echo Build Success! server.exe olusturuldu.
endlocal
