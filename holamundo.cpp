#include <iostream>
#include <omp.h>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <stdio.h>
#include <nlohmann/json.hpp> // Para parsear JSON

using json = nlohmann::json;

std::mutex mutex_json;
int globalIndex = 0; // Contador global para IDs únicos

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

std::string generarProductoJSON(int index, const std::string& titulo, 
                                const std::string& imagen, double precio, 
                                const std::string& url) {
    std::ostringstream producto;
    std::string tituloEscapado = escaparComillas(titulo);
    std::string imagenEscapada = escaparComillas(imagen);
    std::string urlEscapada = escaparComillas(url);

    // Valores por defecto
    std::string imagenFinal = imagenEscapada.empty() ? "./img/abrigos/01.jpg" : imagenEscapada;
    double precioFinal = (precio <= 0) ? 1234 : precio;

    producto << "    {\n"
             << "        \"id\": \"web-" << index << "\",\n"
             << "        \"titulo\": \"" << tituloEscapado << "\",\n"
             << "        \"imagen\": \"" << imagenFinal << "\",\n"
             << "        \"categoria\": {\n"
             << "            \"nombre\": \"Web\",\n"
             << "            \"id\": \"web\"\n"
             << "        },\n"
             << "        \"precio\": " << precioFinal << ",\n"
             << "        \"url\": \"" << urlEscapada << "\"\n" 
             << "    }";
    return producto.str();
}

std::string ejecutarScrapper(const std::string& url) {
    std::string comando;
    
    // Determinar qué scraper usar según la URL
    if (url.find("plazavea.com.pe") != std::string::npos) {
        comando = "python3 scrapper_plaza_vea.py \"" + url + "\"";
    } else if (url.find("ripley.com.pe") != std::string::npos) {
        comando = "python3 scrapper_ripley.py \"" + url + "\"";
    } else {
        std::cerr << "URL no reconocida: " << url << std::endl;
        return "";
    }

    std::string resultado;
    char buffer[4096];
    FILE* pipe = popen(comando.c_str(), "r");
    
    if (!pipe) {
        std::cerr << "Error al ejecutar: " << comando << std::endl;
        return "";
    }
    
    while (fgets(buffer, sizeof(buffer), pipe) {
        resultado += buffer;
    }
    
    pclose(pipe);
    return resultado;
}

int main() {
    omp_set_num_threads(2);

    std::vector<std::string> urls = {
        "https://www.plazavea.com.pe/tecnologia/computo/laptops",
        "https://simple.ripley.com.pe/tecnologia/computacion/laptops"
    };

    std::vector<std::string> todosProductos; // Almacenará todos los productos

    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
        int tid = omp_get_thread_num();
        std::cout << "[Hilo " << tid << "] Procesando URL: " << urls[i] << std::endl;

        std::string resultadoJson = ejecutarScrapper(urls[i]);
        
        if (resultadoJson.empty()) {
            std::cerr << "[Hilo " << tid << "] Error: no se obtuvo JSON para " << urls[i] << std::endl;
            continue;
        }

        try {
            json datos = json::parse(resultadoJson);
            
            if (datos["success"] && datos.contains("results")) {
                // Sección crítica para evitar mezcla de productos
                #pragma omp critical
                {
                    for (auto& producto : datos["results"]) {
                        // Extraer campos con valores por defecto
                        std::string titulo = producto.value("title", "Producto sin nombre");
                        double precio = producto.value("price", 0.0);
                        std::string imagen = producto.value("image_url", "");
                        std::string urlProducto = producto.value("product_url", "");

                        // Generar JSON con estructura deseada
                        std::string productoJSON = generarProductoJSON(
                            globalIndex, 
                            titulo, 
                            imagen, 
                            precio, 
                            urlProducto
                        );
                        
                        todosProductos.push_back(productoJSON);
                        globalIndex++;
                    }
                }
            } else {
                std::cerr << "[Hilo " << tid << "] El scraper no reportó éxito o no tiene resultados" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Hilo " << tid << "] Error parseando JSON: " << e.what() << std::endl;
        }
    }

    // Guardar todos los productos en un solo archivo JSON
    std::ofstream salida("js/productos_generados2.json");
    salida << "[\n";
    for (size_t i = 0; i < todosProductos.size(); ++i) {
        salida << todosProductos[i];
        if (i != todosProductos.size() - 1) salida << ",";
        salida << "\n";
    }
    salida << "]\n";
    salida.close();

    std::cout << "Productos generados guardados en js/productos_generados.json" << std::endl;
    std::cout << "Total de productos: " << todosProductos.size() << std::endl;

    return 0;
}