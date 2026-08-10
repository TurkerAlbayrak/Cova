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
