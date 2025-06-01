# hola.py
import sys
import time
import json
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
from selenium.webdriver.common.by import By

if len(sys.argv) < 2:
    print("Uso: python script.py <URL>")
    sys.exit(1)

url = sys.argv[1]

chrome_path = "/usr/bin/google-chrome-stable"

options = webdriver.ChromeOptions()
options.binary_location = chrome_path
options.add_argument("--headless")
options.add_argument("--no-sandbox")
options.add_argument("--disable-dev-shm-usage")

driver = webdriver.Chrome(service=Service(ChromeDriverManager().install()), options=options)
driver.get(url)
time.sleep(5)

titulo = driver.title
driver.quit()

# Imprimir JSON
print(json.dumps({"titulo": titulo}))
