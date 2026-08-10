# Cova Projesi Test Raporu

## Yapılan İşlemler
Cova projesinin derlenmiş hali olan `server.exe` çalıştırılmış ve Python'un `pytest` ile `requests` kütüphaneleri kullanılarak uç noktaları (endpoints) test edilmiştir. Çalışmalar, ana repoyu kirletmemek adına `C:\Capi\tests_pytest` adında yeni bir klasörde yapılmıştır.

## Test Bulguları
`examples/main.c` dosyasında yer alan uç noktalar referans alınarak testler yazıldığında, mevcut `server.exe` uygulamasının beklenen uç noktalara ve yanıtlara sahip olmadığı görülmüştür. 

*   **Ana Sayfa (GET `/`)**: Örneklerde "Welcome to Cova Framework!" beklenirken sunucu `"Hello World"` yanıtı dönmektedir.
*   **API Durum (GET `/api/status`)**: JSON yanıt beklenirken sunucu bu uç noktada `404 Not Found` hatası dönmektedir.
*   **API Kullanıcılar (GET `/api/users`)**: Veritabanından kullanıcı verisi beklenirken sunucu bu uç noktada `404 Not Found` hatası dönmektedir.
*   **Sayfa Bulunamadı (404)**: Geçersiz bir uç noktaya istek atıldığında sunucu Türkçe olarak `"Ooops! Aradiginiz sayfa uzay boslugunda kayboldu."` hatası dönmektedir.

## Sonuç
`test_server.py` test kodları, mevcut `server.exe`'nin davranışına (mevcut çalışan sürüme) uyarlanarak güncellenmiş ve testlerin başarıyla geçmesi sağlanmıştır. Eğer projenin `examples/main.c`'deki haline göre çalışması hedefleniyorsa, ortamınızda `cmake` yüklü olmadığından dolayı C kodunun yeniden derlenmesi (build) gerçekleştirilememiştir. `server.exe`'nin eski bir derleme olduğu tespit edilmiştir.
