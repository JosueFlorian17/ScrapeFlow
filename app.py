from flask import Flask, request, render_template_string
from urllib.parse import quote

app = Flask(__name__)

HTML = """
<!doctype html>
<title>Buscador de URLs</title>
<h1>Generador de URLs de búsqueda</h1>
<form action="/generar" method="POST">
  <input type="text" name="search_term" placeholder="Ej: iphone 16" required>
  <button type="submit">Buscar</button>
</form>
<p>{{ mensaje }}</p>
"""

@app.route("/", methods=["GET"])
def index():
    return render_template_string(HTML)

@app.route("/generar", methods=["POST"])
def generar():
    search_term = request.form["search_term"]
    encoded_term = quote(search_term)
    dash_term = search_term.replace(" ", "-")

    urls = {
        "Ripley": f"https://simple.ripley.com.pe/search/{encoded_term}?sort=relevance_desc&page=1",
        "Promart": f"https://www.promart.pe/{dash_term}?ft={encoded_term}",
        "Metro": f"https://www.metro.pe/{encoded_term}?_q={encoded_term}&map=ft",
        "Falabella": f"https://www.falabella.com.pe/falabella-pe/search?Ntt={encoded_term}",
        "Efe": f"https://www.efe.com.pe/catalogsearch/result/?q={encoded_term}",
        "La Curacao": f"https://www.lacuracao.pe/catalogsearch/result/?q={encoded_term}",
        "Coolbox": f"https://www.coolbox.pe/{encoded_term}?_q={encoded_term}&map=ft",
        "Hiraoka": f"https://hiraoka.com.pe/gpsearch/?q={encoded_term}",
        "Plaza Vea": f"https://www.plazavea.com.pe/busca/?fq=C:679/686/917&ft={encoded_term}"
    }

    # ✅ Actualiza el archivo local urls.txt
    with open("urls.txt", "w", encoding="utf-8") as f:
        f.write(f"🔍 URLs generadas para: {search_term}\n\n")
        for tienda, url in urls.items():
            f.write(f"{tienda}: {url}\n")

    return render_template_string(HTML, mensaje="✅ URLs actualizadas correctamente en 'urls.txt'.")

if __name__ == "__main__":
    app.run(debug=True)
