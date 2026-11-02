#ifndef MESA_H
#define MESA_H

#include "tablero.h"
#include "config.h"
#include <pthread.h>
#include <vector>

struct ContextoPerro {
    class Mesa* mesa;
    int id_perro;
};

class Mesa {
private:
    int id_mesa;
    int jugador_puma;
    int jugador_perros;
    Configuracion config;

    Tablero tablero;
    pthread_mutex_t mutex_tablero;

    pthread_t hilo_juez;
    pthread_t hilo_puma;
    pthread_t hilos_perros[NUM_PERROS];
    ContextoPerro ctx_perros[NUM_PERROS];

    bool partida_activa;
    int ganador;
    int perros_capturados;
    int pos_puma;
    int pos_perros[NUM_PERROS];

    static void* rutina_puma(void* arg);
    static void* rutina_perro(void* arg);
    static void* rutina_juez(void* arg);

    void jugar_puma();
    void jugar_perro(int id_perro);
    void supervisar_juez();

public:
    Mesa();
    ~Mesa();

    void iniciar(int id, int j_puma, int j_perros, const Configuracion& cfg);
    void esperar();
    void abortar(int jugador_fallo);
    void detener();

    int obtener_ganador() const { return ganador; }
    int obtener_id() const { return id_mesa; }
    bool esta_activa() const { return partida_activa; }
};

#endif
