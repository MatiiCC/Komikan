#include "tablero.h"
#include "config.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "=== PRUEBA DE TABLERO Y CONFIGURACIÓN ===\n";

    Configuracion config;
    if (argc >= 3) {
        parsear_argumentos(argc, argv, config);
    } else {
        std::cout << "Usando valores por defecto (ejecuta './test_tablero N M config.txt' para probar parseo)\n";
        cargar_archivo_config("config.txt", config);
    }

    std::cout << "Jugadores N: " << config.N << "\n";
    std::cout << "Mesas M: " << config.M << "\n";
    std::cout << "Retardo min: " << config.retardo_min_ms << " ms, max: " << config.retardo_max_ms << " ms\n";
    std::cout << "Seed: " << config.seed << "\n";
    std::cout << "Posición inicial Puma: " << config.pos_puma << "\n";
    std::cout << "Posiciones iniciales Perros (" << config.pos_perros.size() << "): ";
    for (int p : config.pos_perros) std::cout << p << " ";
    std::cout << "\n\n";

    Tablero tab;
    tab.inicializar(config.pos_puma, config.pos_perros);

    std::cout << "Visualización del Tablero Inicial:";
    tab.imprimir(std::cout);

    std::cout << "\nVerificación de adyacencias clave:\n";
    std::cout << "Adyacentes al Trono (27): ";
    for (int v : tab.obtener_adyacentes(27)) std::cout << v << " ";
    std::cout << "\n";

    std::cout << "Adyacentes a casilla 25 (triángulo izq): ";
    for (int v : tab.obtener_adyacentes(25)) std::cout << v << " ";
    std::cout << "\n";

    std::cout << "Adyacentes a casilla 22 (centro superior grilla): ";
    for (int v : tab.obtener_adyacentes(22)) std::cout << v << " ";
    std::cout << "\n";

    std::cout << "\nSaltos de captura disponibles desde el Trono (27):\n";
    for (const auto& s : tab.obtener_saltos(27)) {
        std::cout << "  Saltar sobre perro en " << s.victima << " para aterrizar en " << s.destino << "\n";
    }

    std::cout << "\nPruebas completadas correctamente.\n";
    return 0;
}
