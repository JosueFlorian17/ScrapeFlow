from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
import time
import json

# Leer URL desde urls.txt
nombre_tienda = "Coolbox"
url = None

with open("urls.txt", "r", encoding="utf-8") as f:
    for linea in f:
        if linea.startswith(f"{nombre_tienda}:"):
            url = linea.split(":", 1)[1].strip()
            break

if not url:
    print(f"No se encontró la URL para {nombre_tienda} en urls.txt")
    exit()

# Configuración del navegador
options = Options()
options.add_argument("--headless=new")  # Usa 'new' si tienes Chrome reciente
options.add_argument("--disable-gpu")
options.add_argument("--no-sandbox")
options.add_argument("--disable-dev-shm-usage")

driver = webdriver.Chrome(options=options)
driver.get(url)
time.sleep(5)  # Esperar que cargue todo

productos = []

# Iterar sobre los primeros 8 artículos
for i in range(1, 9):
    try:
        selector_article = f"div.coolboxpe-store-search-0-x-galleryItem:nth-child({i}) > section:nth-child(1) > a:nth-child(1) > article:nth-child(1)"
        article = driver.find_element(By.CSS_SELECTOR, selector_article)

        try:
            nombre = article.find_element(By.CSS_SELECTOR, "h3 span").text.strip()
        except:
            nombre = "N/A"

        try:
            precio_actual_element = article.find_element(By.CSS_SELECTOR, "span.vtex-product-price-1-x-sellingPriceValue")
            precio_actual = precio_actual_element.text.replace('\n', '').replace(' ', '').strip()
        except:
            precio_actual = "N/A"

        try:
            precio_anterior_element = article.find_element(By.CSS_SELECTOR, "span.vtex-product-price-1-x-listPriceValue")
            precio_anterior = precio_anterior_element.text.replace('\n', '').replace(' ', '').strip()
        except:
            precio_anterior = "N/A"

        try:
            imagen = article.find_element(By.CSS_SELECTOR, "img").get_attribute("src")
        except:
            imagen = "N/A"

        try:
            url_producto = article.find_element(By.XPATH, "..").get_attribute("href")
        except:
            url_producto = "N/A"

        productos.append({
            "titulo": nombre,
            "precio": precio_anterior,
            "imagen": imagen,
            "url": url_producto
        })

    except Exception as e:
        continue

driver.quit()

# Imprimir productos como JSON
print(json.dumps(productos, ensure_ascii=False, indent=2))
