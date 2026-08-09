#!/bin/bash

echo "[COVA WATCHER] Hot-Reload sistemi baslatildi. Dosyalar izleniyor..."
echo "[COVA WATCHER] Kapatmak icin CTRL+C'ye basin."

# Tüm .c, .h ve .html dosyalarının en son değiştirilme zamanını saniye cinsinden bulur
get_mtime() {
    find src include examples public -type f \( -name "*.c" -o -name "*.h" -o -name "*.html" \) -exec stat -c %Y {} + 2>/dev/null | sort -nr | head -n 1
}

SERVER_PID=""

start_server() {
    echo "[COVA WATCHER] Kod derleniyor..."
    cmake --build build
    if [ $? -eq 0 ]; then
        echo -e "[COVA WATCHER] Sunucu baslatiliyor...\n"
        ./build/cova_server.exe &
        SERVER_PID=$!
    else
        echo "[COVA WATCHER] Derleme hatasi! Lutfen kodu duzeltip kaydedin."
        SERVER_PID=""
    fi
}

# CTRL+C basıldığında arkaplandaki sunucuyu da kapat
trap 'echo -e "\n[COVA WATCHER] Kapatiliyor..."; [ -n "$SERVER_PID" ] && kill $SERVER_PID 2>/dev/null; exit 0' SIGINT SIGTERM

start_server
LAST_MTIME=$(get_mtime)

while true; do
    sleep 1
    CURRENT_MTIME=$(get_mtime)
    
    if [ -n "$CURRENT_MTIME" ] && [ "$CURRENT_MTIME" != "$LAST_MTIME" ]; then
        echo -e "\n[COVA WATCHER] Degisiklik algilandi! Yeniden baslatiliyor..."
        LAST_MTIME=$CURRENT_MTIME
        
        # Eski sunucuyu kapat
        if [ -n "$SERVER_PID" ]; then
            kill $SERVER_PID 2>/dev/null
            wait $SERVER_PID 2>/dev/null
        fi
        
        start_server
    fi
done
