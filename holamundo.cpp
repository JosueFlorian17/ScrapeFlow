#include <iostream>
#include <vector>
#include <omp.h>   // OpenMP
#include <atomic> //std::atomic<int>
#include <sstream> // std::ostringstream                     para armar strings paso por paso
#include <fstream> // // std::ofstream                         I/O en C++
#include <cstdio> // // FILE*, _popen, _pclose, fgets                   manejo de archivos 
#include <nlohmann/json.hpp> // nlohmann::json              Para parsear JSON
#include <Windows.h>                 //to change, específico de Windows
#include <direct.h>       // _mkdir
#include <cstdlib>

using json = nlohmann::json;

//std::mutex mutex_json; //lock compartido para todos los threads
std::atomic<int> globalIndex{0};  
//atomic se encarga de sincronizar, no necesito mutex
//Conteo de producots procesados

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




// Helper function 5: Lee un archivo de texto, obtiene las URLs que contienen "http" y las retorna en un vector de strings
// Si una URL contiene un dominio excluido, se ignora esa URL (porque se ejecuta su scrapper aparte)
std::vector<std::string> obtenerURLsDesdeArchivo(const std::string& archivo, const std::vector<std::string>& dominios_excluidos) {
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

int main() {
    //Le indica a OpenMP que cree máximo 2 subprocesos
    int num_pro=4;
    omp_set_num_threads(num_pro);
    
    const std::string base_dir = "scrapers/";
    std::vector<std::string> scripts = {
        base_dir + "coolbox.py",
        base_dir + "efe.py",
        base_dir + "lacuracao.py",
        base_dir + "plazavea.py",
        base_dir + "ripley.py",
    };

    std::vector<std::string> dominios_excluidos = {
        "coolbox.pe", "efe.com.pe", "lacuracao.pe", "plazavea.com.pe","simple.ripley.com.pe","ripley.com.pe"
    };

    std::vector<std::string> urls = obtenerURLsDesdeArchivo("urls.txt", dominios_excluidos);

    int total_tareas = scripts.size() + urls.size();

    // todosProductos es un vector que en cada elemento tendra el JSON output de cada scraper
    std::vector<std::string> todosProductos;
    // Recordar que a partir del pragma todo se divide a lo largo de los threads determinados

    #pragma omp parallel for
    for (int i = 0; i < total_tareas; ++i) {

        //Remember: se castea explicitamente se size_t (unsigned) a int (signed) para qeu la comparacion sea exacta en tipo

        //Identifico el thread con tid
        int tid = omp_get_thread_num();
        std::cout << "[Hilo " << tid << "] Procesando URL: " << urls[i] << std::endl;

        //La url i se pasa a la funcion helper ejecutarScrapper, que devuelve el json de ese scraper python
        if (i < static_cast<int>(scripts.size())) {
            std::cout << "[🧵 Hilo " << tid << "] Ejecutando script: " << scripts[i] << std::endl;
            resultadoJson = ejecutarScraper(scripts[i]);
        } else {
            std::string url = urls[i - scripts.size()];
            std::cout << "[🧵 Hilo " << tid << "] Ejecutando main_scraper.py con URL: " << url << std::endl;
            resultadoJson = ejecutarMainScraperConURL(url);
        }
        
        if (resultadoJson.empty()) {
            std::cerr << "[Url " <<i<< "] Error: no se obtuvo JSON\n";
            continue;
        }

        // Guardo el JSON crudo obtenido del url asociado al proceso tid (no deberia ser mejor el asociado al url i??): sí!! ya lo cambié
        std::ofstream debug("debug_json_" + std::to_string(i) + ".txt");
        debug << resultadoJson;
        debug.close();

        try {

            // Proceso con nlohmann::json el JSON crudo 
            json datos = json::parse(resultadoJson);
            
            if (datos.contains("results")) {
             //itera sobre cada producto, se asegura que sus resultados sean válidos, y los asigna a variables que luego pasa a la helper generarProductoJSON 
                    //y genera un mini JSON por cada producto
                //Vecotr local (vive en el hilo)
                std::vector<std::string> productosLocal;

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

                    int idx=globalIndex.fetch_add(1, std::memory_order_relaxed);
                    productosLocal.emplace_back(generarProductoJSON(
                        idx, 
                        titulo, 
                        imagen, 
                        precio, 
                        urlProducto
                    ));   
                }
                //el pragma omp critical crea una sección protegida automáticamente, SOLO UN THREAD a la vez ejecuta este bloque    
                #pragma omp critical
                {
                    todosProductos.insert(todosProductos.end(),productosLocal.begin(),productosLocal.end());
                }
            }

            //Si el json tiene False en success o no tiene results, entonces se genera un archivo denominado failed_json con el numero de proceso que lo genero (no seri abueno usar el i del url mejor?)
            else {
                std::cerr << "[Url " <<i<< "] El scraper no reportó éxito\n";
                std::ofstream fail("failed_json_" + std::to_string(i) + ".txt");
                fail << resultadoJson;
                fail.close();
            }
        }
        //Si el try de nolhmann json falla, cualquier error se reporta y el output al file error_json_ con el id del proceso
        catch (const std::exception& e) {
            std::cerr << "[Url " <<i<< "] Error: " << e.what() << "\n";
            std::ofstream error("error_json_" + std::to_string(i) + ".txt");
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