#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

Configuracion::Configuracion()
    : N(2),
      M(1),
      retardo_min_ms(50),
      retardo_max_ms(300),
      seed(0),
      pos_puma(27),
      pos_perros({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 14}),
      archivo_config("") {}

static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool cargar_archivo_config(const std::string& ruta, Configuracion& config) {
    std::ifstream file(ruta);
    if (!file.is_open()) {
        std::cerr << "[ADVERTENCIA] No se pudo abrir el archivo de configuración: " 
                  << ruta << ". Usando valores por defecto.\n";
        return false;
    }

    std::string linea;
    while (std::getline(file, linea)) {
        linea = trim(linea);
        if (linea.empty() || linea[0] == '#') {
            continue; 
        }

        size_t pos_igual = linea.find('=');
        if (pos_igual == std::string::npos) {
            continue;
        }

        std::string clave = trim(linea.substr(0, pos_igual));
        std::string valor = trim(linea.substr(pos_igual + 1));

        std::transform(clave.begin(), clave.end(), clave.begin(), ::tolower);

        if (clave == "retardo_min" || clave == "retardo_min_ms") {
            config.retardo_min_ms = std::stoi(valor);
        } else if (clave == "retardo_max" || clave == "retardo_max_ms") {
            config.retardo_max_ms = std::stoi(valor);
        } else if (clave == "seed" || clave == "semilla") {
            config.seed = static_cast<unsigned int>(std::stoul(valor));
        } else if (clave == "posicion_puma" || clave == "pos_puma") {
            config.pos_puma = std::stoi(valor);
        } else if (clave == "posiciones_perros" || clave == "pos_perros") {
            std::stringstream ss(valor);
            std::vector<int> perros;
            int p;
            while (ss >> p) {
                perros.push_back(p);
            }
            if (!perros.empty()) {
                config.pos_perros = perros;
            }
        }
    }

    
    if (config.retardo_min_ms < 0) config.retardo_min_ms = 10;
    if (config.retardo_max_ms < config.retardo_min_ms) {
        config.retardo_max_ms = config.retardo_min_ms + 100;
    }

    return true;
}

bool parsear_argumentos(int argc, char* argv[], Configuracion& config) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " N M [config.txt]\n";
        std::cerr << "  N: Número total de clientes/jugadores a procesar (N >= 2)\n";
        std::cerr << "  M: Número máximo de mesas concurrentes (M >= 1)\n";
        std::cerr << "  config.txt: Archivo opcional de configuración inicial\n";
        return false;
    }

    try {
        config.N = std::stoi(argv[1]);
        config.M = std::stoi(argv[2]);
    } catch (...) {
        std::cerr << "[ERROR] Los parámetros N y M deben ser números enteros válidos.\n";
        return false;
    }

    if (config.N < 2) {
        std::cerr << "[ERROR] Se necesitan al menos 2 jugadores (N >= 2) para formar una partida.\n";
        return false;
    }

    if (config.M < 1) {
        std::cerr << "[ERROR] El número máximo de mesas concurrentes debe ser al menos 1 (M >= 1).\n";
        return false;
    }

    if (argc >= 4) {
        config.archivo_config = argv[3];
        cargar_archivo_config(config.archivo_config, config);
    }

    return true;
}
