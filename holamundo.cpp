#include <iostream>
#include <omp.h>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <stdio.h>
#include <Windows.h>
#include <direct.h>
#include <nlohmann/json.hpp> // Para parsear JSON

using json = nlohmann::json;

std::mutex mutex_json;
int globalIndex = 0;

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
    char buffer[1024];
    GetCurrentDirectoryA(sizeof(buffer), buffer);
    std::string currentDir(buffer);

    if (url.find("plazavea.com.pe") != std::string::npos) {
        comando = "python \"" + currentDir + "\\scrapper_plaza_vea.py\" \"" + url + "\"";
    } else if (url.find("ripley.com.pe") != std::string::npos) {
        comando = "python \"" + currentDir + "\\scrapper_ripley.py\" \"" + url + "\"";
    } else {
        std::cerr << "URL no reconocida: " << url << std::endl;
        return "";
    }

    std::string resultado;
    char pipeBuffer[4096];
    FILE* pipe = _popen(comando.c_str(), "r");

    if (!pipe) {
        std::cerr << "Error al ejecutar: " << comando << std::endl;
        return "";
    }

    while (fgets(pipeBuffer, sizeof(pipeBuffer), pipe)) {
        resultado += pipeBuffer;
    }

    int exitCode = _pclose(pipe);
    if (exitCode != 0) {
        std::cerr << "Scrapper falló con código: " << exitCode << std::endl;
    }
    
    return resultado;
}

int main() {
    omp_set_num_threads(2);

    std::vector<std::string> urls = {
        "https://www.plazavea.com.pe/tecnologia/computo/laptops",
        "https://simple.ripley.com.pe/tecnologia/computacion/laptops"
    };

    std::vector<std::string> todosProductos;

    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
        int tid = omp_get_thread_num();
        std::cout << "[Hilo " << tid << "] Procesando URL: " << urls[i] << std::endl;

        std::string resultadoJson = ejecutarScrapper(urls[i]);
        
        if (resultadoJson.empty()) {
            std::cerr << "[Hilo " << tid << "] Error: no se obtuvo JSON\n";
            continue;
        }

        // Guardar JSON crudo para depuración
        std::ofstream debug("debug_json_" + std::to_string(tid) + ".txt");
        debug << resultadoJson;
        debug.close();

        try {
            json datos = json::parse(resultadoJson);
            
            if (datos["success"] && datos.contains("results")) {
                #pragma omp critical
                {
                    for (auto& producto : datos["results"]) {
                        // Manejo seguro de campos nulos
                        std::string titulo = "Producto sin nombre";
                        if (producto.contains("title") && !producto["title"].is_null()) {
                            titulo = producto["title"].get<std::string>();
                        }

                        double precio = 0.0;
                        if (producto.contains("price") && !producto["price"].is_null()) {
                            if (producto["price"].is_number()) {
                                precio = producto["price"].get<double>();
                            }
                            // Manejar precios como strings
                            else if (producto["price"].is_string()) {
                                try {
                                    precio = std::stod(producto["price"].get<std::string>());
                                } catch (...) {
                                    precio = 0.0;
                                }
                            }
                        }

                        std::string imagen = "";
                        if (producto.contains("image_url") && !producto["image_url"].is_null()) {
                            imagen = producto["image_url"].get<std::string>();
                        }

                        std::string urlProducto = "";
                        if (producto.contains("product_url") && !producto["product_url"].is_null()) {
                            urlProducto = producto["product_url"].get<std::string>();
                        }

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
            }
            else {
                std::cerr << "[Hilo " << tid << "] El scraper no reportó éxito\n";
                std::ofstream fail("failed_json_" + std::to_string(tid) + ".txt");
                fail << resultadoJson;
                fail.close();
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[Hilo " << tid << "] Error: " << e.what() << "\n";
            std::ofstream error("error_json_" + std::to_string(tid) + ".txt");
            error << resultadoJson;
            error.close();
        }
    }

    // Crear directorio
    if (_mkdir("js") == -1) {
    if (errno != EEXIST) {
        std::cerr << "Error creando directorio js" << std::endl;
    }
}

    // Guardar productos
    std::ofstream salida("js/productos_generados.json");
    salida << "[\n";
    for (size_t i = 0; i < todosProductos.size(); ++i) {
        salida << todosProductos[i];
        if (i != todosProductos.size() - 1) salida << ",";
        salida << "\n";
    }
    salida << "]\n";
    salida.close();

    std::cout << "Productos generados: " << todosProductos.size() << std::endl;
    return 0;
}