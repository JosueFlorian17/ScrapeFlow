from urllib.parse import urlparse, urljoin

def get_selectors_by_domain(url):
      domain = urlparse(url).netloc.lower()
      print("Dominio consultado: ", domain)
      selectors_config = {
          'hiraoka.com.pe': {
              'container': ('li', 'item product product-item'),
              'title': ('a', 'product-item-link'),
              'price': ('span', 'price'),
              'image': ('img', None),
              'link': ('a', None),
              'tienda':('Hiraoka',None),
          },
          'listado.mercadolibre.com.pe': {
              'container': ('li', 'ui-search-layout'),
              'title': ('h3', 'poly-component'),
              'price': ('span', 'money-amount__fraction'),
              'image': ('img', None),
              'link': ('a', None),
              'tienda':('Mercado Libre',None),
          },
          'www.falabella.com.pe': {
              'container': ('div', 'jsx-3752256814'),
              'title': ('b', 'pod-subTitle'),
              'price': ('li', 'prices-0'),
              'image': ('img', None),
              'link': ('a', None),
              'tienda': ('Falabella', None),
          },
          'www.pcfactory.com.pe': {
              'container': ('li', 'ais-Hits-item'),
              'title': ('h4', 'product__card-title'),
              'price': ('div', 'texto-tachado'),
              'image': ('img', None),
              'link': ('a', None),
              'tienda': ('PcFactory', None),
          },
          'linio.falabella.com.pe': {
              'container': ('div', 'search-results'),
              'title': ('b', 'pod-subTitle'),
              'price': ('span', 'jsx-233704000'),
              'image': ('img', None),
              'link': ('a', None),
              'tienda': ('Linio', None),
          },
          'www.metro.pe': {
              'container': ('div', 'vtex-search-result'),
              'title': ('span', 'vtex-product-summary'),
              'price': ('span', 'currencyContainer'),
              'image': ('img', None),
              'link': ('a', None),
              'tienda': ('Metro', None),
          }
      }

      # Retorna los selectores si el dominio es conocido
      if domain in selectors_config:
          return selectors_config[domain]
      else:
          print(f"No hay selectores definidos para el dominio: {domain}")
          return None
