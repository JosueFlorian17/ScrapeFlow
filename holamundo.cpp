#include <iostream>
#include <omp.h>
#include <cstdlib>
#include <vector>

int main() {
    // Fijar número de hilos a 4 (puedes cambiar este valor)
    omp_set_num_threads(4);

    std::vector<std::string> urls = {
        "https://teleticket.com.pe/", // URL 1
        "https://www.facebook.com/",  // URL 2
        "https://teleticket.com.pe/", // URL 3
        "https://teleticket.com.pe/"  // URL 4
    };

#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
        int tid = omp_get_thread_num();
        std::string command = "python hola.py " + urls[i];
        std::cout << "[Hilo " << tid << "] Ejecutando: " << command << std::endl;

        int ret = system(command.c_str());
        if (ret != 0) {
            std::cerr << "[Hilo " << tid << "] Error ejecutando el comando: " << command << std::endl;
        }
    }

    return 0;
}
