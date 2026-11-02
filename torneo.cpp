#include "tablero.h"
#include "config.h"
#include "mesa.h"
#include <iostream>
#include <vector>
#include <semaphore.h>
#include <csignal>
#include <unistd.h>
#include <ctime>

static volatile sig_atomic_t terminar_programa = 0;
static sem_t sem_mesas;
static std::vector<Mesa*> mesas_activas;
static pthread_mutex_t mutex_mesas_activas = PTHREAD_MUTEX_INITIALIZER;

void manejador_sigint(int sig) {
    (void)sig;
    terminar_programa = 1;
    std::cout << "\n[SERVIDOR] Señal SIGINT recibida (Ctrl+C). Deteniendo el torneo ordenadamente...\n";

    pthread_mutex_lock(&mutex_mesas_activas);
    for (Mesa* m : mesas_activas) {
        if (m && m->esta_activa()) {
            m->detener();
        }
    }
    pthread_mutex_unlock(&mutex_mesas_activas);
}

struct DatosHiloPartida {
    int id_mesa;
    int j_puma;
    int j_perros;
    Configuracion config;
};

void* ejecutar_partida(void* arg) {
    DatosHiloPartida* datos = static_cast<DatosHiloPartida*>(arg);

    Mesa* mesa = new Mesa();

    pthread_mutex_lock(&mutex_mesas_activas);
    mesas_activas.push_back(mesa);
    pthread_mutex_unlock(&mutex_mesas_activas);

    mesa->iniciar(datos->id_mesa, datos->j_puma, datos->j_perros, datos->config);
    mesa->esperar();

    pthread_mutex_lock(&mutex_mesas_activas);
    for (auto it = mesas_activas.begin(); it != mesas_activas.end(); ++it) {
        if (*it == mesa) {
            mesas_activas.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_mesas_activas);

    delete mesa;
    delete datos;

    sem_post(&sem_mesas);
    return NULL;
}

int main(int argc, char* argv[]) {
    Configuracion config;
    if (!parsear_argumentos(argc, argv, config)) {
        return 1;
    }

    if (config.seed != 0) {
        srand(config.seed);
    } else {
        srand(time(NULL));
    }

    struct sigaction sa;
    sa.sa_handler = manejador_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sem_init(&sem_mesas, 0, config.M);

    std::cout << "=================================================\n";
    std::cout << "      TORNEO DE KOMIKAN CONCURRENTE\n";
    std::cout << "=================================================\n";
    std::cout << "Jugadores totales (N): " << config.N << "\n";
    std::cout << "Capacidad de mesas simultaneas (M): " << config.M << "\n";
    std::cout << "Retardos: [" << config.retardo_min_ms << " ms - " 
              << config.retardo_max_ms << " ms]\n";
    std::cout << "=================================================\n\n";

    int total_parejas = config.N / 2;
    if (config.N % 2 != 0) {
        std::cout << "[SERVIDOR] Aviso: N=" << config.N 
                  << " es impar. El ultimo jugador quedara en espera.\n";
    }

    std::vector<pthread_t> hilos_gestores;
    int id_mesa_actual = 1;

    for (int p = 0; p < total_parejas; p++) {
        if (terminar_programa) {
            std::cout << "[SERVIDOR] No se aceptan nuevos jugadores por detencion del torneo.\n";
            break;
        }

        int j_puma = 2 * p + 1;
        int j_perros = 2 * p + 2;

        std::cout << "[MATCHMAKER] Emparejando Jugador " << j_puma 
                  << " (Puma) y Jugador " << j_perros << " (Perros). Esperando mesa disponible...\n";

        sem_wait(&sem_mesas);

        if (terminar_programa) {
            sem_post(&sem_mesas);
            break;
        }

        DatosHiloPartida* datos = new DatosHiloPartida();
        datos->id_mesa = id_mesa_actual++;
        datos->j_puma = j_puma;
        datos->j_perros = j_perros;
        datos->config = config;

        pthread_t tid;
        pthread_create(&tid, NULL, ejecutar_partida, datos);
        hilos_gestores.push_back(tid);
    }

    for (pthread_t tid : hilos_gestores) {
        pthread_join(tid, NULL);
    }

    sem_destroy(&sem_mesas);
    pthread_mutex_destroy(&mutex_mesas_activas);

    std::cout << "\n=================================================\n";
    std::cout << "      TORNEO FINALIZADO EXITOSAMENTE\n";
    std::cout << "=================================================\n";
    return 0;
}
