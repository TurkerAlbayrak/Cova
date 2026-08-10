import pytest
import requests
import time
import socket
import concurrent.futures

BASE_URL_HTTP = "http://127.0.0.1:8080"
BASE_URL_HTTPS = "https://127.0.0.1:8080"

def wait_for_server(url, timeout=5):
    start_time = time.time()
    while time.time() - start_time < timeout:
        try:
            r = requests.get(url + "/", verify=False, timeout=1)
            if r.status_code == 200:
                return True
        except requests.ConnectionError:
            time.sleep(0.5)
    return False

@pytest.fixture(scope="module", autouse=True)
def setup_server():
    # Bu testin calismasi icin server'in (server.exe) calisiyor olmasi gerekir.
    # Eger HTTPS aciksa BASE_URL_HTTPS, degilse BASE_URL_HTTP kontrol edilir.
    if not (wait_for_server(BASE_URL_HTTP) or wait_for_server(BASE_URL_HTTPS)):
        pytest.fail("Server did not start or respond in time.")

def test_hello_endpoint():
    try:
        response = requests.get(BASE_URL_HTTP + "/", verify=False)
    except requests.ConnectionError:
        response = requests.get(BASE_URL_HTTPS + "/", verify=False)
        
    assert response.status_code == 200
    assert len(response.text) > 0

def test_not_found_endpoint():
    try:
        response = requests.get(BASE_URL_HTTP + "/invalid_path_404", verify=False)
    except requests.ConnectionError:
        response = requests.get(BASE_URL_HTTPS + "/invalid_path_404", verify=False)
        
    assert response.status_code == 404

def fetch_url(url):
    try:
        return requests.get(url, verify=False, timeout=2).status_code
    except Exception as e:
        return str(e)

def test_thread_pool_concurrency():
    """Test Thread Pool by sending 50 simultaneous requests"""
    # Sunucunun HTTPS olup olmadigini anla
    url = BASE_URL_HTTP + "/"
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS + "/"
        
    num_requests = 50
    results = []
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=50) as executor:
        futures = [executor.submit(fetch_url, url) for _ in range(num_requests)]
        for future in concurrent.futures.as_completed(futures):
            results.append(future.result())
            
    # Tum istekler 200 donmeli, thread pool kitlenmemeli
    assert results.count(200) == num_requests, f"Gecersiz sonuclar: {results}"

def test_keep_alive():
    """Test Connection Keep-Alive by sending multiple requests on the same socket"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    session = requests.Session()
    session.verify = False
    
    # Session, Connection: keep-alive gonderir ve TCP soketini acik tutar
    for i in range(5):
        r = session.get(url + "/")
        assert r.status_code == 200
        # Sunucunun keep-alive olarak yanit verdigini dogrula
        assert r.headers.get("Connection", "").lower() == "keep-alive"
        
    session.close()

def test_gzip_compression():
    """Test Gzip Compression support for text/html/json endpoints"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    # Gzip istedigimizi belirtiyoruz
    headers = {"Accept-Encoding": "gzip"}
    r = requests.get(url + "/", headers=headers, verify=False)
    
    assert r.status_code == 200
    # Python requests kutuphanesi gzip'i otomatik olarak cozer, ama header'da geldi mi kontrol edebiliriz
    # Veya requests otomatik parse edince Content-Encoding header'ini seffafca gizleyebilir,
    # raw urllib3 response uzerinden kontrol etmek daha saglikli:
    assert r.raw.headers.get("Content-Encoding") == "gzip" or r.headers.get("Content-Encoding") == "gzip", "Gzip header was not returned!"
    
    # SIKISTIRMA DOGRULAMASI: Gelen verinin bozulmadigini teyit et
    assert len(r.text) > 0

def test_jwt_middleware():
    """Test JWT Middleware token generation and validation"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    # 1. Korumali sayfaya tokensiz girmeyi dene (401 Bekleniyor)
    r1 = requests.get(url + "/protected", verify=False)
    assert r1.status_code == 401
    assert "Unauthorized" in r1.text
    
    # 2. Login ol ve JWT Token al
    r2 = requests.get(url + "/login", verify=False)
    assert r2.status_code == 200
    token = r2.json().get("token")
    assert token is not None
    assert len(token) > 20
    
    # 3. Korumali sayfaya token ile girmeyi dene (200 Bekleniyor)
    headers = {"Authorization": f"Bearer {token}"}
    r3 = requests.get(url + "/protected", headers=headers, verify=False)
    assert r3.status_code == 200
    assert "Welcome" in r3.text
    
    # 4. Korumali sayfaya GECERSIZ (Bozuk) token ile girmeyi dene (401 Bekleniyor)
    bad_token = token[:-1] + ("A" if token[-1] != "A" else "B")
    bad_headers = {"Authorization": f"Bearer {bad_token}"}
    r4 = requests.get(url + "/protected", headers=bad_headers, verify=False)
    assert r4.status_code == 401
    assert "Unauthorized" in r4.text

def test_rate_limiter():
    """Test Rate Limiting (max 20 requests per second)"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    # Sadece bu test icin limiti 20'ye dusuruyoruz (Diger testleri etkilememek adina)
    requests.get(url + "/set_rate_limit?limit=20", verify=False)
        
    import time
    from concurrent.futures import ThreadPoolExecutor
    
    # Wait for the beginning of the next second to avoid fixed-window reset
    t = time.time()
    time.sleep((int(t) + 1) - t + 0.1)
    
    success_count = 0
    too_many_requests_count = 0
    
    def fetch_url(session):
        return session.get(url + "/")
    
    # TCP baglanti gecikmesi yasamamak icin ayni oturum uzerinden hizlica 45 istek atiyoruz
    session = requests.Session()
    session.verify = False
    results = []
    for _ in range(45):
        results.append(fetch_url(session))
        
    # Diger testlerin bozulmamasi icin limiti geri 1000 yapiyoruz
    time.sleep(1.1)
    requests.get(url + "/set_rate_limit?limit=1000", verify=False)
        
    for r in results:
        if r.status_code == 429:
            too_many_requests_count += 1
            assert "Too Many Requests" in r.text
        else:
            success_count += 1
            
    assert success_count <= 20, f"Expected max 20 successful requests, but got {success_count}"
    assert too_many_requests_count >= 5, f"Expected at least 5 rate limited requests, but got {too_many_requests_count}"

def test_file_upload():
    """Test Multipart/form-data File Upload"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    # Simulate a file upload
    file_content = b"This is a test file content for Cova Framework!"
    files = {
        'profile_pic': ('avatar.txt', file_content, 'text/plain')
    }
    
    r = requests.post(url + "/upload", files=files, verify=False)
    assert r.status_code == 200
    assert "success" in r.text
    assert "avatar.txt" in r.text
    assert str(len(file_content)) in r.text

def test_orm():
    """Test ORM capabilities via query parameters"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    r = requests.get(url + "/api/users?id=1", verify=False)
    assert r.status_code == 200
    assert "cova_admin" in r.text
    
    # Test for non-existent user
    r_not_found = requests.get(url + "/api/users?id=9999", verify=False)
    assert r_not_found.status_code == 404
