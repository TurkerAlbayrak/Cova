import pytest
import requests
import time
import socket

BASE_URL = "http://127.0.0.1:8080"

def wait_for_server():
    for _ in range(10):
        try:
            r = requests.get(BASE_URL + "/")
            if r.status_code == 200:
                return True
        except requests.ConnectionError:
            time.sleep(0.5)
    return False

@pytest.fixture(scope="module", autouse=True)
def setup_server():
    if not wait_for_server():
        pytest.fail("Server did not start in time.")

def test_hello_endpoint():
    response = requests.get(BASE_URL + "/")
    assert response.status_code == 200
    assert response.text == "Hello World"

def test_not_found_endpoint():
    response = requests.get(BASE_URL + "/not_found_path")
    assert response.status_code == 404
    assert response.text == "Ooops! Aradiginiz sayfa uzay boslugunda kayboldu."
