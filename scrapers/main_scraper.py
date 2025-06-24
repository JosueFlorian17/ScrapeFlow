import sys
import json
from scraper import SearchScraper
from utils.selectores import get_selectors_by_domain

def main():
    # Verificar que se proporcione una URL
    if len(sys.argv) != 2:
        print("❌ Debes proporcionar una URL como argumento", file=sys.stderr)
        sys.exit(1)

    url = sys.argv[1]
    selectors = get_selectors_by_domain(url)

    if not selectors:
        print(f"❌ No se encontró configuración para el dominio de la URL: {url}", file=sys.stderr)
        sys.exit(1)

    scraper = SearchScraper()
    try:
        results = scraper.scrape_search_results(url, selectors)

        # Imprimir solo la lista de productos como JSON puro
        json.dump(results, sys.stdout, ensure_ascii=False, indent=2)

    except Exception as e:
        print(f"❌ Error al procesar la URL: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
