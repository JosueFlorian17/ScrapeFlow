# Selenium
from selenium import webdriver
from webdriver_manager.chrome import ChromeDriverManager
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.common.exceptions import NoSuchElementException, ElementNotInteractableException
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

#Importar librerías
import requests
from bs4 import BeautifulSoup
import re
from urllib.parse import urlparse, urljoin
import time

def configurar_driver():
    options = webdriver.ChromeOptions()
    options.add_argument("--headless")
    options.add_argument("--no-sandbox")
    options.add_argument("--disable-gpu")
    service = Service(ChromeDriverManager().install())
    return webdriver.Chrome(service=service, options=options)

def cerrar_banner_cookies(driver):
    try:
        cookie_banner = driver.find_element(By.CLASS_NAME, "cookie-consent-banner-opt-out__container")
        if cookie_banner.is_displayed():
            cookie_banner.click()
    except (NoSuchElementException, ElementNotInteractableException):
        pass


class SearchScraper:
    def __init__(self):
        self.headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
        }
        self.price_patterns = [
            r'[$€S/]*\s*[\d.,]+',  # Captura precios con posibles símbolos y dígitos con punto o coma
            r'(?:precio|price)[^\d]*([\d.,]+)'  # Captura luego de palabra "precio"
        ]

    def clean_text(self, text):
        """Limpia espacios extra y caracteres no deseados"""
        return re.sub(r'\s+', ' ', text).strip()

    def extract_price(self, text):
        """Extrae el precio conservando solo números, puntos y comas."""
        for pattern in self.price_patterns:
            matches = re.findall(pattern, text, re.IGNORECASE)
            if matches:
                price_text = matches[0]
                # Conservar solo dígitos, puntos y comas
                price_clean = re.sub(r'[^\d.,]', '', price_text)
                if price_clean:
                    return price_clean
        return None

    def obtener_resultados(self, soup, url, selectors):
        #Nombre del item del producto
        tag, cls = selectors['container']
        class_regex = re.compile(re.escape(cls), re.I)
        products = soup.find_all(tag, class_=class_regex)
        results = []
        seen_urls = set()  # Para controlar duplicados
        
        for product in products:
            # Extraer título
            title = None
            if 'title' in selectors:
                tag, cls = selectors['title']
                title_regex = re.compile(re.escape(cls), re.I)
                title_elem = product.find(tag, class_=title_regex)
                title = self.clean_text(title_elem.text.strip()) if title_elem else None
                #print("titulo: ", title)

            # Extraer precio
            price_num = None
            price_text = ""
            if 'price' in selectors:
                tag, cls = selectors['price']
                price_regex= re.compile(re.escape(cls), re.I)
                price_elem = product.find(tag, class_=price_regex)
                price_text = self.clean_text(price_elem.get_text()) if price_elem else ""
                price_num = self.extract_price(price_text)
                #print("Precio: ", price_text)

            if price_text == "":
                continue

            # Extraer imagen
            image_url = None
            if 'image' in selectors:
                tag, cls = selectors['image']
                img_elem = product.find(tag, class_=cls) if cls else product.find(tag)
                #print("Imagen: ", img_elem)
                if img_elem:
                  image_url = img_elem.get('data-src') or img_elem.get('src')
                  # Manejar url relativas
                  parsed = urlparse(image_url)
                  if not parsed.scheme:  # No hay http:// o https:// → es relativa
                      base_url = f"https://{urlparse(url).netloc}"
                      image_url = urljoin(base_url, image_url)
                else:
                  image_url = None

            # Extraer URL del producto
            product_url = None
            if 'link' in selectors:
                tag, cls = selectors['link']
                link_elem = product.find(tag, class_=cls) if cls else product.find(tag, href=True)
                #print("Link: ", link_elem)
                if link_elem:
                    product_url = urljoin(url, link_elem.get('href'))

            # Si no hay URL o es un duplicado, saltar este producto
            if not product_url or product_url in seen_urls:
                continue

            seen_urls.add(product_url)  # Registrar URL para evitar duplicados

            nombre_tienda, _ = selectors['tienda']

            results.append({
                'titulo': title or "N/A",
                'precio': price_num or "N/A",
                'imagen': image_url or "N/A",
                'url': product_url or "N/A"
            })

        return results

    def scrape_search_results(self, url, selectors, umbral_minimo=3):
        """Extrae todos los productos de una página de búsqueda, eliminando duplicados y productos sin precio"""
        try:
            response = requests.get(url, headers=self.headers)
            response.raise_for_status()
            soup = BeautifulSoup(response.content, 'html.parser')
            results=self.obtener_resultados(soup, url, selectors)

            if len(results) < umbral_minimo:
                print("Pocos productos encontrados. Pasando a Selenium")
                return self.scrape_with_selenium(url, selectors)

            return results

        except Exception as e:
            print(f"Falló el request debido a {e}")
            return self.scrape_with_selenium(url, selectors)

    def scrape_with_selenium(self, url, selectors, max_paginas=3):
        driver = configurar_driver()
        driver.get(url)
        cerrar_banner_cookies(driver)

        resultados_totales = []

        tag, cls = selectors['container']
        WebDriverWait(driver, 10).until(
          EC.presence_of_element_located((By.CSS_SELECTOR, f"{tag}[class*='{cls}']"))
        )

        for i in range(max_paginas):
          # Simular scroll hacia abajo para cargar más productos
          scroll_pause = 2
          last_height = driver.execute_script("return document.body.scrollHeight")

          while True:
              driver.execute_script("window.scrollTo(0, document.body.scrollHeight);")
              time.sleep(scroll_pause)
              new_height = driver.execute_script("return document.body.scrollHeight")
              if new_height == last_height:
                  break
              last_height = new_height

          time.sleep(2)
          soup = BeautifulSoup(driver.page_source, 'html.parser')

          # Obtener resultados
          resultados_pagina = self.obtener_resultados(soup, url, selectors)
          resultados_totales.extend(resultados_pagina)

          # Intentar pasar a la siguiente página
          try:
              boton = driver.find_element(By.CSS_SELECTOR, ".vtex-pagination__next")
              boton.click()
          except NoSuchElementException:
              print("No hay más páginas.")
              break
        driver.quit()
        return resultados_totales
    
