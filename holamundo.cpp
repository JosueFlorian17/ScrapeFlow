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
    auto obtener = [&](const std::string& clave) -> std::string {
        auto it = datos.find(clave);
        return (it != datos.end()) ? escaparComillas(it->second) : "N/A";
    };

    std::ostringstream oss;
    oss << "  {\n";
    oss << "    \"titulo\": \"" << obtener("titulo") << "\",\n";
    oss << "    \"precio\": \"" << obtener("precio") << "\",\n";
    oss << "    \"imagen\": \"" << obtener("imagen") << "\",\n";
    oss << "    \"url\": \"" << obtener("url") << "\"\n";
    oss << "  }";
    return oss.str();
}

std::vector<std::map<std::string, std::string>> ejecutarScraper(const std::string& script) {
    std::vector<std::map<std::string, std::string>> productos;
    std::string comando = "python3 " + script;
    FILE* pipe = popen(comando.c_str(), "r");

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

int main() {
    omp_set_num_threads(4); // Puedes ajustar esto según tu CPU

    // Añade aquí todos los scrapers Python disponibles
    const std::string base_dir = "scrapers/";
    std::vector<std::string> scripts = {
        base_dir + "coolbox.py",
        base_dir + "efe.py",
        base_dir + "lacuracao.py",
        base_dir + "plazavea.py"
    };

    std::vector<std::string> productosTotales;

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < static_cast<int>(scripts.size()); ++i) {
        int tid = omp_get_thread_num();
        std::cout << "[🧵 Hilo " << tid << "] Ejecutando: " << scripts[i] << std::endl;

        std::vector<std::map<std::string, std::string>> productos = ejecutarScraper(scripts[i]);

        for (const auto& datos : productos) {
            std::string jsonProducto = generarProductoJSON(datos);
            std::lock_guard<std::mutex> lock(mutex_json);
            productosTotales.push_back(jsonProducto);
        }
    }

    // Guardar todos los productos en un solo archivo JSON
    std::ofstream salida("main/js/productos_generados.json");
    salida << "[\n";
    for (size_t i = 0; i < productosTotales.size(); ++i) {
        salida << productosTotales[i];
        if (i != productosTotales.size() - 1) salida << ",";
        salida << "\n";
    }
    salida << "]\n";
    salida.close();

    std::cout << "✅ Productos generados guardados en js/productos_generados.json" << std::endl;

    return 0;
}
