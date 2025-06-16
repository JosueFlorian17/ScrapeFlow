from flask import Flask, request, send_from_directory
from urllib.parse import quote

app = Flask(__name__)

@app.route('/')
def index():
    return send_from_directory('.', 'probando.html')

@app.route('/ver')
def ver_archivo():
    try:
        with open("urls.txt", "r", encoding="utf-8") as f:
            contenido = f.read().replace("\n", "<br>")
        return f"<h2>Contenido de urls.txt</h2><p>{contenido}</p>"
    except FileNotFoundError:
        return "Archivo urls.txt no encontrado."

@app.route('/guardar', methods=['POST'])
def guardar():
    data = request.get_json()
    term = data.get("termino", "").strip()
    if not term:
        return "Término vacío", 400

    encoded = quote(term)
    dashed = term.replace(" ", "-")

    urls = {
        "Ripley": f"https://simple.ripley.com.pe/search/{encoded}?sort=relevance_desc&page=1",
        "Promart": f"https://www.promart.pe/{dashed}?ft={encoded}",
        "Metro": f"https://www.metro.pe/{encoded}?_q={encoded}&map=ft",
        "Falabella": f"https://www.falabella.com.pe/falabella-pe/search?Ntt={encoded}",
        "Efe": f"https://www.efe.com.pe/catalogsearch/result/?q={encoded}",
        "La Curacao": f"https://www.lacuracao.pe/catalogsearch/result/?q={encoded}",
        "Coolbox": f"https://www.coolbox.pe/{encoded}?_q={encoded}&map=ft",
        "Hiraoka": f"https://hiraoka.com.pe/gpsearch/?q={encoded}",
        "Plaza Vea": f"https://www.plazavea.com.pe/busca/?ft={encoded}"
    }

    with open("urls.txt", "w", encoding="utf-8") as f:
        f.write(f"🔍 URLs generadas para: {term}\n\n")
        for tienda, url in urls.items():
            f.write(f"{tienda}: {url}\n")

    return "Archivo urls.txt actualizado exitosamente."

if __name__ == "__main__":
    app.run(debug=True)
