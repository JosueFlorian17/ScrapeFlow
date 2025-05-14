import sys
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
from selenium.webdriver.common.by import By
import time

url = sys.argv[1]

chrome_path = "C:/Program Files/Google/Chrome/Application/chrome.exe"

options = webdriver.ChromeOptions()
options.binary_location = chrome_path
options.add_argument("--headless")

driver = webdriver.Chrome(service=Service(ChromeDriverManager().install()), options=options)

driver.get(url)

time.sleep(5)

titulo = driver.title
print("Título de la página:", titulo)

driver.quit()
