#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>

struct Configuracion {
    int N;                      // Total de jugadores
    int M;                      // Máximo de mesas concurrentes
    int retardo_min_ms;         // Retardo mínimo por movimiento (ms)
    int retardo_max_ms;         // Retardo máximo por movimiento (ms)
    unsigned int seed;          // Semilla aleatoria (0 = time(NULL))
    int pos_puma;               // Posición inicial del Puma (default: 27)
    std::vector<int> pos_perros;// Posiciones iniciales de los 12 Perros
    std::string archivo_config; // Ruta al archivo opcional

    Configuracion();
};

// Parsea argumentos de línea de comando: ./torneo N M [config.txt]
bool parsear_argumentos(int argc, char* argv[], Configuracion& config);

// Carga parámetros desde archivo de configuración
bool cargar_archivo_config(const std::string& ruta, Configuracion& config);

#endif // CONFIG_H
