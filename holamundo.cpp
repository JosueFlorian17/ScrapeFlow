#include <iostream>
#include <omp.h>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <string>
#include <map>
#include <set>

std::mutex mutex_json;

std::string escaparComillas(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (c == '"') result += "\\\"";
        else result += c;
    }
    return result;
}

std::vector<std::map<std::string, std::string>> parsearJSONdePython(const std::string& json) {
    std::vector<std::map<std::string, std::string>> productos;
    size_t pos = 0;

    while ((pos = json.find('{', pos)) != std::string::npos) {
        size_t end = json.find('}', pos);
        if (end == std::string::npos) break;

        std::string objeto = json.substr(pos, end - pos + 1);
        std::map<std::string, std::string> producto;

        size_t kpos = 0;
        while ((kpos = objeto.find("\"", kpos)) != std::string::npos) {
            size_t kend = objeto.find("\"", kpos + 1);
            if (kend == std::string::npos) break;

            std::string clave = objeto.substr(kpos + 1, kend - kpos - 1);
            size_t cpos = objeto.find(":", kend);
            size_t vstart = objeto.find("\"", cpos);
            size_t vend = objeto.find("\"", vstart + 1);
            if (vstart == std::string::npos || vend == std::string::npos) break;

            std::string valor = objeto.substr(vstart + 1, vend - vstart - 1);
            producto[clave] = valor;

            kpos = vend + 1;
        }

        productos.push_back(producto);
        pos = end + 1;
    }

    return productos;
}

std::string generarProductoJSON(const std::map<std::string, std::string>& datos) {
    auto obtener = [&](const std::vector<std::string>& claves) -> std::string {
        for (const auto& clave : claves) {
            auto it = datos.find(clave);
            if (it != datos.end()) return escaparComillas(it->second);
        }
        return "N/A";
    };

    std::ostringstream oss;
    oss << "  {\n";
    oss << "    \"titulo\": \"" << obtener({"titulo", "title"}) << "\",\n";
    oss << "    \"precio\": \"" << obtener({"precio", "price"}) << "\",\n";
    oss << "    \"imagen\": \"" << obtener({"imagen", "image_url"}) << "\",\n";
    oss << "    \"url\": \"" << obtener({"url", "product_url"}) << "\"\n";
    oss << "  }";
    return oss.str();
}

std::vector<std::map<std::string, std::string>> ejecutarScraper(const std::string& script) {
    std::string comando = "python3 " + script;
    FILE* pipe = popen(comando.c_str(), "r");
    std::vector<std::map<std::string, std::string>> productos;

    if (!pipe) {
        std::cerr << "❌ Error al ejecutar: " << script << std::endl;
        return productos;
    }

    char buffer[2048];
    std::string salidaTotal;

    while (fgets(buffer, sizeof(buffer), pipe)) {
        salidaTotal += buffer;
    }
    pclose(pipe);

    return parsearJSONdePython(salidaTotal);
}

std::vector<std::string> obtenerURLsDesdeArchivo(const std::string& archivo, const std::set<std::string>& dominios_excluidos) {
    std::vector<std::string> urls;
    std::ifstream file(archivo);
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("http") != std::string::npos) {
            size_t pos = line.find("http");
            std::string url = line.substr(pos);
            for (const std::string& dominio : dominios_excluidos) {
                if (url.find(dominio) != std::string::npos) {
                    url = "";
                    break;
                }
            }
            if (!url.empty()) urls.push_back(url);
        }
    }
    return urls;
}

std::vector<std::map<std::string, std::string>> ejecutarMainScraperConURL(const std::string& url) {
    std::vector<std::map<std::string, std::string>> productos;
    std::string comando = "python3 scrapers/main_scraper.py \"" + url + "\"";
    FILE* pipe = popen(comando.c_str(), "r");

    if (!pipe) {
        std::cerr << "❌ Error al ejecutar main_scraper.py con URL: " << url << std::endl;
        return productos;
    }

    char buffer[2048];
    std::string salidaTotal;

    while (fgets(buffer, sizeof(buffer), pipe)) {
        salidaTotal += buffer;
    }
    pclose(pipe);

    return parsearJSONdePython(salidaTotal);
}

int main() {
    omp_set_num_threads(4);

    const std::string base_dir = "scrapers/";
    std::vector<std::string> scripts = {
        base_dir + "coolbox.py",
        base_dir + "efe.py",
        base_dir + "lacuracao.py",
        base_dir + "plazavea.py"
    };

    std::set<std::string> dominios_excluidos = {
        "coolbox.pe", "efe.com.pe", "lacuracao.pe", "plazavea.com.pe"
    };

    std::vector<std::string> urls = obtenerURLsDesdeArchivo("urls.txt", dominios_excluidos);
    std::vector<std::string> productosTotales;

    int total_tareas = scripts.size() + urls.size();

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < total_tareas; ++i) {
        int tid = omp_get_thread_num();
        std::vector<std::map<std::string, std::string>> productos;

        if (i < static_cast<int>(scripts.size())) {
            std::cout << "[🧵 Hilo " << tid << "] Ejecutando script: " << scripts[i] << std::endl;
            productos = ejecutarScraper(scripts[i]);
        } else {
            std::string url = urls[i - scripts.size()];
            std::cout << "[🧵 Hilo " << tid << "] Ejecutando main_scraper.py con URL: " << url << std::endl;
            productos = ejecutarMainScraperConURL(url);
        }

        for (const auto& datos : productos) {
            std::string jsonProducto = generarProductoJSON(datos);
            std::lock_guard<std::mutex> lock(mutex_json);
            productosTotales.push_back(jsonProducto);
        }
    }

    std::ofstream salida("main/js/productos_generados.json");
    salida << "[\n";
    for (size_t i = 0; i < productosTotales.size(); ++i) {
        salida << productosTotales[i];
        if (i != productosTotales.size() - 1) salida << ",";
        salida << "\n";
    }
    salida << "]\n";
    salida.close();

    std::cout << "✅ Productos generados guardados en main/js/productos_generados.json" << std::endl;
    return 0;
}
