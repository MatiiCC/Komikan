#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>

struct Configuracion {
    int N;                      
    int M;                      
    int retardo_min_ms;         
    int retardo_max_ms;         
    unsigned int seed;          
    int pos_puma;               
    std::vector<int> pos_perros;
    std::string archivo_config;

    Configuracion();
};
bool parsear_argumentos(int argc, char* argv[], Configuracion& config);
bool cargar_archivo_config(const std::string& ruta, Configuracion& config);

#endif
