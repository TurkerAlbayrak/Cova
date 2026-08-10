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
    # The server (server.exe) must be running for these tests.
    # If HTTPS is enabled, it checks BASE_URL_HTTPS, else BASE_URL_HTTP.
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
    # Detect if server is HTTPS
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
            
    # All requests should return 200, thread pool should not lock up
    assert results.count(200) == num_requests, f"Invalid results: {results}"

def test_keep_alive():
    """Test Connection Keep-Alive by sending multiple requests on the same socket"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    session = requests.Session()
    session.verify = False
    
    # Session sends Connection: keep-alive and keeps TCP socket open
    for i in range(5):
        r = session.get(url + "/")
        assert r.status_code == 200
        # Verify that server responds with keep-alive
        assert r.headers.get("Connection", "").lower() == "keep-alive"
        
    session.close()

def test_gzip_compression():
    """Test Gzip Compression support for text/html/json endpoints"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    # Requesting gzip compression
    headers = {"Accept-Encoding": "gzip"}
    r = requests.get(url + "/", headers=headers, verify=False)
    
    assert r.status_code == 200
    # The python requests library automatically decodes gzip, but we can check if it arrived in headers
    # Or, requests might hide the Content-Encoding transparently when parsed.
    # Checking raw urllib3 response is safer:
    assert r.raw.headers.get("Content-Encoding") == "gzip" or r.headers.get("Content-Encoding") == "gzip", "Gzip header was not returned!"
    
    # VERIFICATION: Confirm that the payload is not corrupted
    assert len(r.text) > 0

def test_jwt_middleware():
    """Test JWT Middleware token generation and validation"""
    url = BASE_URL_HTTP
    try:
        requests.get(url, verify=False)
    except:
        url = BASE_URL_HTTPS
        
    # 1. Attempt to access protected route without token (Expect 401)
    r1 = requests.get(url + "/protected", verify=False)
    assert r1.status_code == 401
    assert "Unauthorized" in r1.text
    
    # 2. Login and retrieve JWT Token
    r2 = requests.get(url + "/login", verify=False)
    assert r2.status_code == 200
    token = r2.json().get("token")
    assert token is not None
    assert len(token) > 20
    
    # 3. Attempt to access protected route with valid token (Expect 200)
    headers = {"Authorization": f"Bearer {token}"}
    r3 = requests.get(url + "/protected", headers=headers, verify=False)
    assert r3.status_code == 200
    assert "Welcome" in r3.text
    
    # 4. Attempt to access protected route with INVALID token (Expect 401)
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
        
    # Lower limit to 20 for this test only (to avoid breaking other tests)
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
    
    # Fire 45 rapid requests on same TCP session to avoid connection delays
    session = requests.Session()
    session.verify = False
    results = []
    for _ in range(45):
        results.append(fetch_url(session))
        
    # Restore limit to 1000 so other tests do not fail
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
