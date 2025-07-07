#!/usr/bin/env python3
import json
from urllib.parse import urlparse
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.chrome.options import Options
from webdriver_manager.chrome import ChromeDriverManager
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import sys
import io

# Redirigir salida en UTF-8
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')

# Leer URL desde urls.txt
nombre_tienda = "Plaza Vea"
url = None

with open("urls.txt", "r", encoding="utf-8") as f:
    for linea in f:
        if linea.startswith(f"{nombre_tienda}:"):
            url = linea.split(":", 1)[1].strip()
            break

if not url:
    print(f"No se encontró la URL para {nombre_tienda} en urls.txt")
    exit()

def init_driver():
    options = Options()
    options.add_argument('--headless')
    options.add_argument('--no-sandbox')
    options.add_argument('--disable-dev-shm-usage')
    options.add_argument(
        '--user-agent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) '
        'AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.93 Safari/537.36'
    )
    service = Service(ChromeDriverManager().install())
    return webdriver.Chrome(service=service, options=options)

def scrape_plazavea(url):
    driver = init_driver()
    driver.get(url)
    wait = WebDriverWait(driver, 20)
    containers = wait.until(
        EC.presence_of_all_elements_located(
            (By.CSS_SELECTOR, 'div.HA.Showcase.Showcase--non-food.ga-product-item')
        )
    )
    results = []
    for c in containers:
        try:
            title = c.find_element(By.CSS_SELECTOR, 'strong.product.name a').text.strip().replace("\"", " ")
            price_elem = c.find_element(By.CSS_SELECTOR, '.Showcase__salePrice')
            price = float(price_elem.get_attribute('data-price').replace(',', ''))
            img_el = c.find_element(By.CSS_SELECTOR, 'figure.Showcase__photo img')
            image_url = img_el.get_attribute('src')
            link_el = c.find_element(By.CSS_SELECTOR, 'a.Showcase__link')
            product_url = link_el.get_attribute('href')

            results.append({
                'titulo': title,
                'precio': str(price),
                'imagen': image_url,
                'url': product_url
            })
        except Exception:
            continue
    driver.quit()
    return results

def main():
    items = scrape_plazavea(url)
    for i, item in enumerate(items):
        print(json.dumps(item, ensure_ascii=False))
    # Nota: No se imprime lista, solo objetos JSON individuales

if __name__ == '__main__':
    main()
