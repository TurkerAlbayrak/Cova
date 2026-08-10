# Kapsamlı Sistem Testi Raporu (Cova Framework)

Yeni geliştirdiğimiz **HTTPS (OpenSSL)** ve **Thread Pool (İş Parçacığı Havuzu)** özelliklerini sınamak adına detaylı bir test senaryosu hazırlandı. 

## 1. Hazırlanan Test Senaryoları (`test_server.py`)
Mevcut Python `pytest` dosyamız aşağıdaki stres ve güvenlik testlerini kapsayacak şekilde baştan yazıldı:
* **Eşzamanlılık (Concurrency) Stres Testi:** Yeni yazdığımız Thread Pool mimarisinin kilitlenip kilitlenmediğini anlamak için sunucuya aynı anda 50 paralel HTTP/HTTPS isteği (request) gönderen bir asenkron yük testi eklendi (`test_thread_pool_concurrency`).
* **Otomatik HTTPS Doğrulaması:** Sunucunun açık olan portlarını tarayarak bağlantının güvenli (`https://`) mi yoksa güvensiz (`http://`) mi olduğunu tespit eden ve SSL sertifika iletişimini doğrulayan (TLS Handshake) testler entegre edildi.

## 2. Test Yürütme Bulguları (ÖNEMLİ)
Sistem ortamınızda gerçekleştirdiğimiz derinlemesine taramalar sonucunda **hiçbir C derleyicisi (GCC, Clang, MinGW, MSVC) veya derleme aracı (CMake)** bulunamadı. (Hatta kurulu olan Windows Subsystem for Linux (WSL) üzerinde bile GCC mevcut değil).

C kodlarında (Thread Pool ve OpenSSL entegrasyonu) yaptığımız değişikliklerin çalıştırılabilir bir `server.exe` dosyasına dönüştürülmesi (build) için derleyici şarttır. Bu sebeple yazdığımız yeni ve güçlü Cova mimarisi, bilgisayarınızda **fiziksel olarak derlenip çalıştırılamadığı için test süreçleri simülasyondan öteye geçememiştir.** Eski `server.exe` dosyası, eklediğimiz yeni özellikleri barındırmadığı için test edilmesi anlamsız olacaktır.

## 3. Sonuç ve Aksiyon Planı
Kapsamlı test betiklerini (`test_server.py`) Github deponuza yükledim. 
Sistemi gerçek anlamda test edebilmemiz için:
1. Bilgisayarınıza **MSYS2 (MinGW-w64)** veya **Visual Studio C++ Build Tools** kurmalısınız.
2. Kurulumun ardından Cova dizini içerisindeki hazırladığım `build.bat` dosyasını çalıştırarak yeni `server.exe`'yi elde edebilirsiniz.
3. Sonrasında `pytest C:\Capi\tests_pytest\test_server.py` komutu ile havuz (thread pool) ve SSL dayanıklılığını test edebilirsiniz.
