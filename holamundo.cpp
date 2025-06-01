#include <iostream>
#include <omp.h>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <mutex>
#include <cstdio>

std::mutex mutex_json;

std::string escaparComillas(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (c == '"') {
            result += "\\\"";
        } else {
            result += c;
        }
    }
    return result;
}

std::string generarProductoJSON(const std::string& titulo, const std::string& url, int index) {
    std::ostringstream producto;
    std::string tituloEscapado = escaparComillas(titulo);
    producto << "    {\n"
             << "        \"id\": \"web-" << index << "\",\n"
             << "        \"titulo\": \"" << tituloEscapado << "\",\n"
             << "        \"imagen\": \"./img/abrigos/01.jpg\",\n"
             << "        \"categoria\": {\n"
             << "            \"nombre\": \"Web\",\n"
             << "            \"id\": \"web\"\n"
             << "        },\n"
             << "        \"precio\": 1234,\n"
             << "        \"url\": \"" << url << "\"\n" 
             << "    }";
    return producto.str();
}


std::string ejecutarPythonYObtenerTitulo(const std::string& url) {
    std::string comando = "python3 hola.py \"" + url + "\"";
    std::string resultado;
    FILE* pipe = popen(comando.c_str(), "r");

    if (!pipe) {
        std::cerr << "Error al ejecutar el comando Python." << std::endl;
        return "";
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        resultado += buffer;
    }

    pclose(pipe);

    // Supone que el script devuelve: {"titulo": "Nombre del sitio"}
    size_t pos1 = resultado.find(":");
    size_t pos2 = resultado.rfind("\"");
    if (pos1 != std::string::npos && pos2 != std::string::npos && pos2 > pos1 + 2) {
        return resultado.substr(pos1 + 2, pos2 - pos1 - 2);
    }

    return "";
}

int main() {
    omp_set_num_threads(4);

    std::vector<std::string> urls = {
        "https://teleticket.com.pe/",
        "https://www.facebook.com/",
        "https://www.google.com/",
        "https://www.wikipedia.org/"
    };

    std::vector<std::string> productos(urls.size());

#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
        int tid = omp_get_thread_num();
        std::cout << "[Hilo " << tid << "] Procesando URL: " << urls[i] << std::endl;

        std::string titulo = ejecutarPythonYObtenerTitulo(urls[i]);

        if (!titulo.empty()) {
            productos[i] = generarProductoJSON(titulo, urls[i], i);
        } else {
            productos[i] = "    // Error obteniendo título";
        }
    }

    // Guardar todos los productos en un solo archivo JSON
    std::ofstream salida("js/productos_generados.json");
    salida << "[\n";
    for (size_t i = 0; i < productos.size(); ++i) {
        salida << productos[i];
        if (i != productos.size() - 1) salida << ",";
        salida << "\n";
    }
    salida << "]\n";
    salida.close();

    std::cout << "Productos generados guardados en productos_generados.json" << std::endl;

    return 0;
}
