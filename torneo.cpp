#include "tablero.h"
#include "config.h"
#include "mesa.h"
#include <iostream>
#include <vector>
#include <semaphore.h>
#include <csignal>
#include <unistd.h>
#include <ctime>
#include <cerrno>

volatile sig_atomic_t terminar_programa = 0;
static sem_t sem_mesas;
static std::vector<Mesa*> mesas_activas;
static pthread_mutex_t mutex_mesas_activas = PTHREAD_MUTEX_INITIALIZER;

void manejador_sigint(int sig) {
    (void)sig;
    terminar_programa = 1;
    const char msg[] = "\n[SERVIDOR] Senal SIGINT recibida (Ctrl+C). Iniciando apagado ordenado...\n";
    ssize_t ret = write(STDERR_FILENO, msg, sizeof(msg) - 1);
    (void)ret;
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

    pthread_mutex_lock(&mutex_consola);
    std::cout << "=================================================\n";
    std::cout << "      TORNEO DE KOMIKAN CONCURRENTE\n";
    std::cout << "=================================================\n";
    std::cout << "Jugadores totales (N): " << config.N << "\n";
    std::cout << "Capacidad de mesas simultaneas (M): " << config.M << "\n";
    std::cout << "Retardos: [" << config.retardo_min_ms << " ms - " 
              << config.retardo_max_ms << " ms]\n";
    std::cout << "=================================================\n\n";
    pthread_mutex_unlock(&mutex_consola);

    int total_parejas = config.N / 2;
    if (config.N % 2 != 0) {
        pthread_mutex_lock(&mutex_consola);
        std::cout << "[SERVIDOR] Aviso: N=" << config.N 
                  << " es impar. El ultimo jugador quedara en espera.\n";
        pthread_mutex_unlock(&mutex_consola);
    }

    std::vector<pthread_t> hilos_gestores;
    int id_mesa_actual = 1;

    for (int p = 0; p < total_parejas; p++) {
        if (terminar_programa) {
            break;
        }

        int j_puma = 2 * p + 1;
        int j_perros = 2 * p + 2;

        pthread_mutex_lock(&mutex_consola);
        std::cout << "[MATCHMAKER] Emparejando Jugador " << j_puma 
                  << " (Puma) y Jugador " << j_perros << " (Perros). Esperando mesa disponible...\n";
        pthread_mutex_unlock(&mutex_consola);

        int s_res;
        while ((s_res = sem_wait(&sem_mesas)) != 0 && errno == EINTR) {
            if (terminar_programa) {
                break;
            }
        }

        if (terminar_programa || s_res != 0) {
            if (s_res == 0) {
                sem_post(&sem_mesas);
            }
            break;
        }

        DatosHiloPartida* datos = new DatosHiloPartida();
        datos->id_mesa = id_mesa_actual++;
        datos->j_puma = j_puma;
        datos->j_perros = j_perros;
        datos->config = config;

        pthread_t tid;
        if (pthread_create(&tid, NULL, ejecutar_partida, datos) == 0) {
            hilos_gestores.push_back(tid);
        } else {
            std::cerr << "[ERROR] No se pudo crear hilo para mesa.\n";
            delete datos;
            sem_post(&sem_mesas);
        }
    }

    if (terminar_programa) {
        pthread_mutex_lock(&mutex_consola);
        std::cout << "[SERVIDOR] Deteniendo mesas activas...\n";
        pthread_mutex_unlock(&mutex_consola);

        pthread_mutex_lock(&mutex_mesas_activas);
        for (Mesa* m : mesas_activas) {
            if (m) {
                m->detener();
            }
        }
        pthread_mutex_unlock(&mutex_mesas_activas);
    }

    // Esperar a que terminen todos los hilos gestores y las mesas
    for (pthread_t tid : hilos_gestores) {
        pthread_join(tid, NULL);
    }

    sem_destroy(&sem_mesas);
    pthread_mutex_destroy(&mutex_mesas_activas);
    pthread_mutex_destroy(&mutex_consola);

    std::cout << "\n=================================================\n";
    if (terminar_programa) {
        std::cout << "  TORNEO INTERRUMPIDO Y CERRADO CORRECTAMENTE\n";
    } else {
        std::cout << "      TORNEO FINALIZADO EXITOSAMENTE\n";
    }
    std::cout << "=================================================\n";
    return 0;
}
