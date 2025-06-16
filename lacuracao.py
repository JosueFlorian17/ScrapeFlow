import requests
from bs4 import BeautifulSoup
from urllib.parse import urlparse
import json

# Leer URL desde urls.txt
nombre_tienda = "La Curacao"
url = None

with open("urls.txt", "r", encoding="utf-8") as f:
    for linea in f:
        if linea.startswith(f"{nombre_tienda}:"):
            url = linea.split(":", 1)[1].strip()
            break

if not url:
    print(f"No se encontró la URL para {nombre_tienda} en urls.txt")
    exit()

# Realizar la solicitud HTTP
headers = {"User-Agent": "Mozilla/5.0"}
response = requests.get(url, headers=headers)

# Lista para guardar resultados
results = []

# Verificar si la solicitud fue exitosa
if response.status_code == 200:
    soup = BeautifulSoup(response.text, "html.parser")

    # Seleccionar todos los productos
    items = soup.select("li.item.product.product-item")

    # Limitar a los primeros 8 productos
    for item in items[:8]:
        try:
            title = item.select_one('strong.product.name a').text.strip()
        except:
            title = ""

        try:
            price = item.select_one('.special-price .price').text.strip()
        except:
            price = ""

        try:
            image_url = item.select_one('img.product-image-photo')['src']
        except:
            image_url = ""

        try:
            product_url = item.select_one('a.product-item-link')['href']
        except:
            product_url = ""

        # Extraer el dominio
        domain = urlparse(product_url).netloc if product_url else ""

        # Agregar al listado
        results.append({
            "titulo": title,
            "precio": price,
            "imagen": image_url,
            "url": product_url
        })

    # Imprimir el JSON en el formato solicitado
    output = {
        "success": True,
        "results": results
    }

    print(json.dumps(output, indent=2, ensure_ascii=False))

else:
    print(json.dumps({
        "success": False,
        "results": [],
        "error": f"Error al cargar la página: {response.status_code}"
    }, indent=2, ensure_ascii=False))
