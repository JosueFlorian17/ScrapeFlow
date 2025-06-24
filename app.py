from flask import Flask, request, send_from_directory
from urllib.parse import quote
import os
import subprocess  # ✅ Importamos subprocess para ejecutar el binario

app = Flask(__name__, static_folder="buscador", static_url_path="/buscador")

# Página de inicio: buscador visual
@app.route("/")
def home():
    return send_from_directory("buscador", "index.html")

# Página principal de resultados (Temulianos)
@app.route("/main/index.html")
def pagina_resultado():
    return send_from_directory("main", "index.html")

# Recursos estáticos desde /main (CSS, JS, imágenes)
@app.route("/main/<path:filename>")
def main_static(filename):
    return send_from_directory("main", filename)

# Guardar las URLs en el archivo urls.txt y ejecutar el binario scraperfinal
@app.route("/guardar", methods=["POST"])
def guardar():
    data = request.get_json()
    search_term = data.get("termino", "").strip()

    if not search_term:
        return "❌ No se recibió término de búsqueda", 400

    encoded_term = quote(search_term)
    dash_term = search_term.replace(" ", "-")

    urls = {
        "Ripley": f"https://simple.ripley.com.pe/search/{encoded_term}?sort=relevance_desc&page=1",
        "Metro": f"https://www.metro.pe/{encoded_term}?_q={encoded_term}&map=ft",
        "Falabella": f"https://www.falabella.com.pe/falabella-pe/collection/{dash_term}",
        "Efe": f"https://www.efe.com.pe/catalogsearch/result/?q={encoded_term}",
        "La Curacao": f"https://www.lacuracao.pe/catalogsearch/result/?q={encoded_term}",
        "Coolbox": f"https://www.coolbox.pe/{encoded_term}?_q={encoded_term}&map=ft",
        "Hiraoka": f"https://hiraoka.com.pe/gpsearch/?q={encoded_term}",
        "Plaza Vea": f"https://www.plazavea.com.pe/busca/?fq=C:679/686/917&ft={encoded_term}",
        "Mercado Libre": f"https://listado.mercadolibre.com.pe/{dash_term}?sb=all_mercadolibre#D[A:{encoded_term}]",
        "PcFactory": f"https://www.pcfactory.com.pe/busqueda_avanzada?query={encoded_term}",
        "Linio": f"https://linio.falabella.com.pe/linio-pe/search?Ntt={encoded_term}"
    }

    with open("urls.txt", "w", encoding="utf-8") as f:
        f.write(f"🔍 URLs generadas para: {search_term}\n\n")
        for tienda, url in urls.items():
            f.write(f"{tienda}: {url}\n")

    # ✅ Ejecutar el binario scraperfinal
    try:
        subprocess.run(["./scraperfinal3"], check=True)
    except Exception as e:
        return f"❌ Error al ejecutar scraperfinal: {str(e)}", 500

    return "✅ URLs guardadas y scraper ejecutado"

# Otros archivos sueltos como /carrito.html
@app.route("/<path:filename>")
def serve_misc(filename):
    return send_from_directory(".", filename)

if __name__ == "__main__":
    app.run(debug=True)
