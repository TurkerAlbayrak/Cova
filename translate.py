import os

replacements = {
    # database.c / database.h / orm.c / orm.h (Already done mostly, but just in case)
    "Hata: Veritabani acilamadi": "Error: Database could not be opened",
    "SQLite veritabani (%s) basariyla acildi!": "SQLite database (%s) successfully opened!",
    "Sorgu Hatasi (Execute):": "Query Error (Execute):",
    "Sorgu Hatasi (Query):": "Query Error (Query):",
    "Hata:": "Error:",
    "Insert Hatasi:": "Insert Error:",

    # request.c / request.h (Already done mostly)

    # server.c
    "// Router Eşleştirme Motoru (YENİ - V9) - İleri Tanımlama (Forward Declaration)": "// Router Matching Engine (NEW - V9) - Forward Declaration",
    "// Thread'e gönderilecek argümanları taşıyan yapı": "// Structure carrying arguments to be sent to Thread",
    "// --- THREAD FONKSİYONU ---": "// --- THREAD FUNCTION ---",
    "// Her gelen bağlantı (client) kendi bağımsız kanalında (thread) bu fonksiyonu çalıştırır.": "// Every incoming connection (client) runs this function in its own independent thread.",
    "// Argümanlar kopyalandı, belleği temizle (V16 - Kendi free fonksiyonumuz)": "// Arguments copied, clear memory (V16 - Our custom free function)",
    "// Zaman Aşımı (Timeout) ayarı (5 saniye)": "// Timeout setting (5 seconds)",
    "// Max header boyutu": "// Max header size",
    "// Bağlantı kapandı veya Timeout oldu veya geçersiz istek": "// Connection closed, Timeout occurred, or invalid request",
    "// Content-Length'i bul": "// Find Content-Length",
    "// V22: Limit Kontrolü": "// V22: Limit Control",
    "// Buffer'ı Request struct'ına çevir": "// Convert Buffer to Request struct",
    "// Eski yapi icin (JSON)": "// For old structure (JSON)",
    "// V22: Multipart Parse": "// V22: Multipart Parse",
    "// Eğer parse edilemeyen anlamsız bir istekse (V13 - 500 Hatası)": "// If it is a meaningless request that cannot be parsed (V13 - 500 Error)",
    "// Gelen isteği Router'dan geçirip uygun Handler'ı (Fonksiyonu) bulma": "// Passing the incoming request through the Router to find the appropriate Handler",
    "// 1. Önce Middleware'leri (Ara Yazılımları) Çalıştır": "// 1. First Run Middlewares",
    "// Ara yazılım 0 dönerse, isteği kes (Örn: Yetki Yok)": "// If middleware returns 0, abort the request (E.g: Unauthorized)",
    "// 2. Statik Dosya (Public Folder) Kontrolü (V14)": "// 2. Static File (Public Folder) Check (V14)",
    "// 3. Normal Rota Eşleştirme (V9 Matcher kullanıyoruz)": "// 3. Normal Route Matching (Using V9 Matcher)",
    "// Doğru rotayı bulduk, fonksiyona yolla!": "// Found the correct route, dispatch to function!",
    "// 4. Hiçbir rota eşleşmezse 404 Sayfa Bulunamadı Hatası (V13)": "// 4. If no route matches, 404 Page Not Found Error (V13)",
    "// İstek işlendikten sonra Keep-Alive kontrolü": "// Keep-Alive check after request is processed",
    "// İstemci (Client) kapatmak istiyor": "// Client wants to close",
    "// Sunucu veya Handler kapatmak istiyor": "// Server or Handler wants to close",
    "// Dosyayı Binary (Rb) modunda açıyoruz": "// Opening file in Binary (Rb) mode",
    "// Dosya yoksa 404 döndür": "// Return 404 if file does not exist",
    "// Güvenlik: Dizin dışına çıkmayı engelle (Directory Traversal Attack)": "// Security: Prevent Directory Traversal Attack",
    "// V9 - Dinamik Rota (Path Parameter) Eşleştirme Motoru": "// V9 - Dynamic Route (Path Parameter) Matching Engine",
    "// Tam eşleşme (Örn: \"/api/status\" == \"/api/status\")": "// Exact match (E.g: \"/api/status\" == \"/api/status\")",
    "// Parametreli rotaları ayır (Örn: \"/users/:id\" ve \"/users/5\")": "// Separate parameterized routes (E.g: \"/users/:id\" and \"/users/5\")",
    "// Güvenlik için dizileri null ile sonlandır": "// Null-terminate arrays for security",
    "// Aynı anda ilerle": "// Advance simultaneously",
    "// Parametre (:) bulduk, demek ki bu kısmı eşleşmiş sayacağız!": "// Found parameter (:), which means we'll consider this part matched!",
    "// Değerini kopyalayalım": "// Let's copy its value",
    "// Parametre bitene kadar (veya string bitene kadar) req_ptr'yi ilerlet": "// Advance req_ptr until parameter ends (or string ends)",
    "// Son olarak kopyaladığımız parametre string'ini null terminate edelim": "// Finally, null terminate the copied parameter string",
    "// Eşleşme bozuldu": "// Match broken",
    "// Her iki string de aynı anda bittiyse tam eşleşmiştir!": "// If both strings end at the same time, it's an exact match!",
    "// Windows için WSAStartup (Socket kütüphanesini başlat)": "// WSAStartup for Windows (Initialize Socket library)",
    "// Socket Oluşturma": "// Socket Creation",
    "Soket olusturulamadi!": "Socket creation failed!",
    "// Port Bağlama (Bind)": "// Port Binding (Bind)",
    "// Adresi hemen yeniden kullanabilmek için SO_REUSEADDR ayarı": "// SO_REUSEADDR setting to reuse the address immediately",
    "Bind hatasi! Port kullanimda olabilir.": "Bind error! Port might be in use.",
    "// Dinlemeye Başla (Listen)": "// Start Listening (Listen)",
    "Listen hatasi!": "Listen error!",
    "// V17: Thread Pool Başlatılıyor (Örn: 10 worker thread)": "// V17: Thread Pool Starting (E.g: 10 worker threads)",
    "// CTRL+C sinyalini yakalamak için": "// To catch CTRL+C signal",
    "// Ana Sunucu Döngüsü": "// Main Server Loop",
    "// Yeni İstemci Kabul Et (Accept)": "// Accept New Client",
    "// V17: İsteği (Client'i) Thread Pool'a gönder": "// V17: Send Request (Client) to Thread Pool",
    "// Kuyruğa Ekle": "// Add to Queue",
    "// Sinyal Yönetimi (CTRL+C) ve Kapanış": "// Signal Handling (CTRL+C) and Shutdown",
    "// Bellek sızıntısı raporunu göster": "// Show memory leak report",
    "// Tüm bağlantıları ve socketleri temizler, memory leak raporunu basar": "// Cleans up all connections and sockets, prints memory leak report",
    "// Global degisken baslangiclari": "// Global variables initialization",
    "printf(\"Shutting down server gracefully...\\n\");": "printf(\"Shutting down server gracefully...\\n\");", # Just keep it
    "printf(\"\\n[INFO] Shutting down server gracefully...\\n\\n\");": "printf(\"\\n[INFO] Shutting down server gracefully...\\n\\n\");",
    "printf(\"[INFO] Server listening on port %d...\\n\", port);": "printf(\"[INFO] Server listening on port %d...\\n\", port);",
    
    # main.c
    "Veritabani baglantisi kurulamadi!": "Database connection failed!",
    "Basariyla test kullanicisi eklendi.": "Successfully added test user.",
    "// V23: Tabloyu ORM uzerinden otomatik olustur (Auto-Migrate)": "// V23: Automatically create table via ORM (Auto-Migrate)",
    "// Eger tablo bossa, ORM kullanarak test verisi ekleyelim": "// If table is empty, add test data using ORM",
    "// 2. Initialize the Framework App": "// 2. Initialize the Framework App",
    "// 3. Register Middlewares": "// 3. Register Middlewares",
    "// 4. Register Static Files Directory": "// 4. Register Static Files Directory",
    "// 5. Register Custom Error Handlers": "// 5. Register Custom Error Handlers",
    "// V20: JWT Secret Ayari": "// V20: JWT Secret Configuration",
    "// V21: Rate Limiter Ayari (Saniyede max 1000 istek, diger testleri bloklamamasi icin)": "// V21: Rate Limiter Settings (Max 1000 requests per second, to not block other tests)",
    "// 6. Define Routes": "// 6. Define Routes",
    "// V22: File Upload route": "// V22: File Upload route",
    "// 7. Start the Server": "// 7. Start the Server",
    "// Route seviyesinde JWT Middleware cagirimi": "// JWT Middleware call at route level",
    "// Orijinal dosya adini kullanarak public klasorune kaydet": "// Save to public folder using original filename",
    "Dosya %s basariyla yuklendi!": "File %s uploaded successfully!",
    "Dosya alinamadi!": "File could not be received!",
    "\"{\\\"message\\\": \\\"Basariyla eklendi!\\\"}\"": "\"{\\\"message\\\": \\\"Successfully added!\\\"}\"",
    "Kullanici bulunamadi!": "User not found!",
    "Eksik JSON verisi!": "Missing JSON data!",
    
    # memtrack.c
    "// Toplam tahsis edilen bellek miktarı": "// Total allocated memory amount",
    "// Bellek bloğunun tahsis edildiği fonksiyon/dosya (İleride geliştirilebilir)": "// Function/file where memory block was allocated (Can be improved in future)",
    "// Tahsis edilen blok boyutu": "// Allocated block size",
    "// V16: Kendi Malloc Fonksiyonumuz (Sızıntıları Yakalamak İçin)": "// V16: Our Custom Malloc Function (To Catch Leaks)",
    "// Gerçek malloc'u çağır": "// Call real malloc",
    "// Hafıza yoksa direkt dön": "// Return directly if out of memory",
    "// Takip objesini oluştur": "// Create tracking object",
    "// Zincire (Linked List) ekle": "// Add to Linked List",
    "// Mutex ile kilitliyoruz (Thread Safe)": "// Lock with Mutex (Thread Safe)",
    "// V16: Kendi Free Fonksiyonumuz": "// V16: Our Custom Free Function",
    "// Zincirden (Linked List) çıkart": "// Remove from Linked List",
    "// Mutex kilidini açıyoruz": "// Unlock Mutex",
    "// V16: Bellek Sızıntı Raporu": "// V16: Memory Leak Report",
    "[V16] MEMORY LEAK (BELLEK SIZINTISI) RAPORU": "[V16] MEMORY LEAK REPORT",
    "Toplam Tahsis (Malloc) :": "Total Allocations (Malloc) :",
    "adet": "count",
    "Toplam Temizleme (Free):": "Total Frees (Free):",
    ">> SONUC: KUSURSUZ! (0 Bellek Sizintisi) <<": ">> RESULT: FLAWLESS! (0 Memory Leaks) <<",
    ">> DIKKAT: %d ADET BELLEK SIZINTISI TESPIT EDILDI! <<": ">> WARNING: %d MEMORY LEAKS DETECTED! <<",
    "-> Sızan Blok: %p, Boyut: %zu bytes": "-> Leaked Block: %p, Size: %zu bytes",
    "// Thread safety için kritik bölge": "// Critical section for thread safety",

    # response.c
    "// Header listesine content-type ekle": "// Add content-type to header list",
    "// HTTP Yanıtını oluşturur ve gönderir": "// Creates and sends the HTTP Response",
    "// 1. Status Line": "// 1. Status Line",
    "// 2. Headers": "// 2. Headers",
    "// 3. Body": "// 3. Body",
    "// Metin Yanıt": "// Text Response",
    "// Dosya yoksa 404 döndür": "// Return 404 if file not found",
    "// Dosya boyutunu öğren": "// Get file size",
    "// Response gönder (Headers)": "// Send Response (Headers)",

    # threadpool.c
    "// Kuyrukta kalan ve islenmeyen gorevleri (ve argumanlarini) temizle": "// Clean up unprocessed tasks in the queue (and their arguments)"
}

def translate_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
            
        original_content = content
        for turkish, english in replacements.items():
            content = content.replace(turkish, english)
            
        if content != original_content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Translated: {filepath}")
    except Exception as e:
        pass

src_dir = "src"
include_dir = "include"

for root, _, files in os.walk(src_dir):
    for file in files:
        if file.endswith(".c") or file.endswith(".h"):
            translate_file(os.path.join(root, file))
            
for root, _, files in os.walk(include_dir):
    for file in files:
        if file.endswith(".c") or file.endswith(".h"):
            translate_file(os.path.join(root, file))
            
translate_file("examples/main.c")
