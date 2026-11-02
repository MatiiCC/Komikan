#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>

Configuracion::Configuracion()
    : N(2),
      M(1),
      retardo_min_ms(50),
      retardo_max_ms(300),
      seed(0),
      pos_puma(TRONO_POS),
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
        std::cerr << "[ADVERTENCIA] No se pudo abrir el archivo de configuracion: " 
                  << ruta << ". Usando valores por defecto.\n";
        return false;
    }

    std::string linea;
    int numero_linea = 0;
    while (std::getline(file, linea)) {
        numero_linea++;
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

        try {
            if (clave == "retardo_min" || clave == "retardo_min_ms") {
                config.retardo_min_ms = std::stoi(valor);
            } else if (clave == "retardo_max" || clave == "retardo_max_ms") {
                config.retardo_max_ms = std::stoi(valor);
            } else if (clave == "seed" || clave == "semilla") {
                config.seed = static_cast<unsigned int>(std::stoul(valor));
            } else if (clave == "posicion_puma" || clave == "pos_puma") {
                int p = std::stoi(valor);
                if (p >= 0 && p < NUM_CASILLAS) {
                    config.pos_puma = p;
                } else {
                    std::cerr << "[ADVERTENCIA] Posicion del puma fuera de rango (0-" 
                              << (NUM_CASILLAS - 1) << "): " << p << ". Se mantiene por defecto.\n";
                }
            } else if (clave == "posiciones_perros" || clave == "pos_perros") {
                std::stringstream ss(valor);
                std::string token;
                std::vector<int> perros;
                std::set<int> vistos;
                bool formato_valido = true;

                while (ss >> token) {
                    try {
                        int pos = std::stoi(token);
                        if (pos < 0 || pos >= NUM_CASILLAS) {
                            std::cerr << "[ADVERTENCIA] Posicion de perro fuera de rango: " << pos << "\n";
                            formato_valido = false;
                            break;
                        }
                        if (vistos.count(pos)) {
                            std::cerr << "[ADVERTENCIA] Posicion de perro duplicada: " << pos << "\n";
                            formato_valido = false;
                            break;
                        }
                        vistos.insert(pos);
                        perros.push_back(pos);
                    } catch (...) {
                        formato_valido = false;
                        break;
                    }
                }

                if (formato_valido && perros.size() == NUM_PERROS) {
                    // Verificar que ningun perro coincida con el puma
                    bool solapado = false;
                    for (int p : perros) {
                        if (p == config.pos_puma) {
                            solapado = true;
                            break;
                        }
                    }
                    if (!solapado) {
                        config.pos_perros = perros;
                    } else {
                        std::cerr << "[ADVERTENCIA] Un perro coincide con la casilla del puma. Se mantienen posiciones por defecto.\n";
                    }
                } else if (!formato_valido) {
                    std::cerr << "[ADVERTENCIA] Formato invalido en posiciones de perros (linea " 
                              << numero_linea << "). Se mantienen por defecto.\n";
                } else if (perros.size() != NUM_PERROS) {
                    std::cerr << "[ADVERTENCIA] Se requieren exactamente " << NUM_PERROS 
                              << " perros (se encontraron " << perros.size() 
                              << "). Se mantienen por defecto.\n";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[ADVERTENCIA] Error de parseo en linea " << numero_linea 
                      << " ('" << clave << "'): valor invalido '" << valor 
                      << "'. Se ignora esta clave.\n";
        } catch (...) {
            std::cerr << "[ADVERTENCIA] Error desconocido en linea " << numero_linea << ". Se ignora.\n";
        }
    }

    // Validacion y saneamiento de rangos de retardo
    if (config.retardo_min_ms < 1) {
        std::cerr << "[ADVERTENCIA] retardo_min debe ser >= 1 ms. Ajustado a 10 ms.\n";
        config.retardo_min_ms = 10;
    }
    if (config.retardo_max_ms < config.retardo_min_ms) {
        std::cerr << "[ADVERTENCIA] retardo_max menor que retardo_min. Ajustado a " 
                  << (config.retardo_min_ms + 100) << " ms.\n";
        config.retardo_max_ms = config.retardo_min_ms + 100;
    }

    return true;
}

bool parsear_argumentos(int argc, char* argv[], Configuracion& config) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " N M [config.txt]\n";
        std::cerr << "  N: Numero total de clientes/jugadores a procesar (N >= 2)\n";
        std::cerr << "  M: Numero maximo de mesas concurrentes (M >= 1)\n";
        std::cerr << "  config.txt: Archivo opcional de configuracion inicial\n";
        return false;
    }

    try {
        config.N = std::stoi(argv[1]);
        config.M = std::stoi(argv[2]);
    } catch (const std::exception&) {
        std::cerr << "[ERROR] Los parametros N y M deben ser numeros enteros validos.\n";
        return false;
    } catch (...) {
        std::cerr << "[ERROR] Error al leer los parametros N y M.\n";
        return false;
    }

    if (config.N < 2) {
        std::cerr << "[ERROR] Se necesitan al menos 2 jugadores (N >= 2) para formar una partida.\n";
        return false;
    }

    if (config.M < 1) {
        std::cerr << "[ERROR] El numero maximo de mesas concurrentes debe ser al menos 1 (M >= 1).\n";
        return false;
    }

    if (argc >= 4) {
        config.archivo_config = argv[3];
        cargar_archivo_config(config.archivo_config, config);
    }

    return true;
}
