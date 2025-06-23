#include <iostream>
#include <omp.h>
#include <cstdlib>
#include <vector>
#include <sstream> //para armar strings paso por paso
#include <fstream> //I/O en C++
#include <mutex>  //para secciones críticas, aseguradas de los threads
#include <cstdio> //manejo de archivos 
#include <stdio.h>
#include <Windows.h> //to change, específico de Windows
#include <direct.h>
#include <nlohmann/json.hpp> // Para parsear JSON

using json = nlohmann::json;

std::mutex mutex_json; //lock compartido para todos los threads
int globalIndex = 0;  //Conteo de producots procesados

// Helper function 1: Añade un backslash a las comillas dobles. Servirá para los JSON, donde se deben escapar las comillas



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

// Helper function 2: Recibe el indice global del producto, el titulo, el link a la imagen, el precio y la url a la pagina del producto
// Escapa comillas en cada variable y si es que no es válido el valor de la variable (porque el scraper no obtuvo el valor), aplica los valores default
// Devuelve una string con forma interna de json


std::string generarProductoJSON(int index, const std::string& titulo, 
                                const std::string& imagen, double precio, 
                                const std::string& url) {
    std::ostringstream producto; // un buffer de memoria al que puedes tratar exactamente como sifuera cout y luego llamar str para obtener una string grande
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

// Helper function 3: Recibe una url, y en base a su dominio, elije el scraper de python a ejecutar
// Obtiene el output del scraper python (un JSON de forma {"success":true,"results":[ … ]}) y lo retorna
std::string ejecutarScrapper(const std::string& url) {
    std::string comando;
    char buffer[1024];
    GetCurrentDirectoryA(sizeof(buffer), buffer);
    std::string currentDir(buffer);

    if (url.find("plazavea.com.pe") != std::string::npos) {
        // se colocan entre comillas por si acaso haya espacios en el path
        comando = "python \"" + currentDir + "\\scrapper_plaza_vea.py\" \"" + url + "\"";
    } else if (url.find("ripley.com.pe") != std::string::npos) {
        comando = "python \"" + currentDir + "\\scrapper_ripley.py\" \"" + url + "\"";
    } else {
        std::cerr << "URL no reconocida: " << url << std::endl;
        return "";
    }

    std::string resultado;
    // definir el tamaño de chunk a leer
    char pipeBuffer[4096];
    // Inicia el comando, y retorna un pipe que permite que c++ lea todo lo que el script de python imprima
    FILE* pipe = _popen(comando.c_str(), "r");

    if (!pipe) {
        std::cerr << "Error al ejecutar: " << comando << std::endl;
        return "";
    }
    // Lee chunks de 4kb hasta que el programa de python temrina de imprimir y los pega en la variable resultado
    while (fgets(pipeBuffer, sizeof(pipeBuffer), pipe)) {
        resultado += pipeBuffer;
    }
    // Cierra el pipe y obtiene su status de exito. Debe ser 0, sino es porque el Python script falló
    int exitCode = _pclose(pipe);
    if (exitCode != 0) {
        std::cerr << "Scrapper falló con código: " << exitCode << std::endl;
    }
    //Retorna el json string de forma {"success":true,"results":[ … ]} que produce el scraper python
    return resultado;
}

int main() {
    //Le indica a OpenMP que cree máximo 2 subprocesos
    std::int num_pro=2;
    omp_set_num_threads(num_pro);
    std::vector<std::string> urls = {
        "https://www.plazavea.com.pe/tecnologia/computo/laptops",
        "https://simple.ripley.com.pe/tecnologia/computacion/laptops"
    };

    // todosProductos es un vector que en cada elemento tendra el JSON output de cada scraper
    std::vector<std::string> todosProductos;
    // Recordar que a partir del pragma todo se divide a lo largo de los threads determinados

    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(urls.size()); ++i) {

        //Remember: se castea explicitamente se size_t (unsigned) a int (signed) para qeu la comparacion sea exacta en tipo

        //Identifico el thread con tid
        int tid = omp_get_thread_num();
        std::cout << "[Hilo " << tid << "] Procesando URL: " << urls[i] << std::endl;

        //La url i se pasa a la funcion helper ejecutarScrapper, que devuelve el json de ese scraper python
        std::string resultadoJson = ejecutarScrapper(urls[i]);
        
        if (resultadoJson.empty()) {
            std::cerr << "[Url " << id << "] Error: no se obtuvo JSON\n";
            continue;
        }

        // Guardo el JSON crudo obtenido del url asociado al proceso tid (no deberia ser mejor el asociado al url i??): sí!! ya lo cambié
        std::ofstream debug("debug_json_" + std::to_string(id) + ".txt");
        debug << resultadoJson;
        debug.close();

        try {

            // Proceso con nlohmann::json el JSON crudo 
            json datos = json::parse(resultadoJson);
            
            if (datos["success"] && datos.contains("results")) {

                #pragma omp critical
                //el pragma omp critical crea una sección protegida automáticamente, SOLO UN THREAD a la vez ejecuta este bloque    

                { //itera sobre cada producto, se asegura que sus resultados sean válidos, y los asigna a variables que luego pasa a la helper generarProductoJSON 
                    //y genera un mini JSON por cada producto
                   
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
            //Si el json tiene False en success o no tiene results, entonces se genera un archivo denominado failed_json con el numero de proceso que lo genero (no seri abueno usar el i del url mejor?)
            else {
                std::cerr << "[Url " << id << "] El scraper no reportó éxito\n";
                std::ofstream fail("failed_json_" + std::to_string(id) + ".txt");
                fail << resultadoJson;
                fail.close();
            }
        }
        //Si el try de nolhmann json falla, cualquier error se reporta y el output al file error_json_ con el id del proceso
        catch (const std::exception& e) {
            std::cerr << "[Url " << id << "] Error: " << e.what() << "\n";
            std::ofstream error("error_json_" + std::to_string(id) + ".txt");
            error << resultadoJson;
            error.close();
        }
    }

    // Crear directorio si no existe
    if (_mkdir("js") == -1) {
    if (errno != EEXIST) {
        // Si el mkdir falla y no es porque el directorio exista, entonces hubo un error creandolo
        std::cerr << "Error creando directorio js" << std::endl;}
    }

    // Guardar productos usando ofstream y la variable salida para acumular cada producto en todosProductos
    
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