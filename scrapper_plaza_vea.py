#!/usr/bin/env python3
import sys
import json
from urllib.parse import urlparse
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.chrome.options import Options
from webdriver_manager.chrome import ChromeDriverManager
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import argparse

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
            title = c.get_attribute('data-ga-name')
            price_elem = c.find_element(By.CSS_SELECTOR, '.Showcase__salePrice')
            price = float(price_elem.get_attribute('data-price').replace(',', ''))
            img_el = c.find_element(By.CSS_SELECTOR, 'figure.Showcase__photo img')
            image_url = img_el.get_attribute('src')
            link_el = c.find_element(By.CSS_SELECTOR, 'a.Showcase__link')
            product_url = link_el.get_attribute('href')
            domain = urlparse(product_url).netloc
            results.append({
                'title': title,
                'price': price,
                'image_url': image_url,
                'product_url': product_url,
                'domain': domain
            })
        except Exception:
            continue
    driver.quit()
    return results


def main():
    parser = argparse.ArgumentParser(description='Scrape Plaza Vea laptops')
    parser.add_argument('url', help='URL de categoría a scrapear')
    args = parser.parse_args()
    items = scrape_plazavea(args.url)
    output = {
        'success': len(items) > 0,
        'results': items
    }
    print(json.dumps(output, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
