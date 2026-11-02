#include "mesa.h"
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>

pthread_mutex_t mutex_consola = PTHREAD_MUTEX_INITIALIZER;

Mesa::Mesa() {
    pthread_mutex_init(&mutex_tablero, NULL);
    partida_activa = false;
    ganador = 0;
    perros_capturados = 0;
    pos_puma = TRONO_POS;
    for (int i = 0; i < NUM_PERROS; i++) {
        pos_perros[i] = -1;
    }
}

Mesa::~Mesa() {
    pthread_mutex_destroy(&mutex_tablero);
}

void Mesa::iniciar(int id, int j_puma, int j_perros, const Configuracion& cfg) {
    id_mesa = id;
    jugador_puma = j_puma;
    jugador_perros = j_perros;
    config = cfg;

    tablero.inicializar(config.pos_puma, config.pos_perros);
    pos_puma = config.pos_puma;
    for (size_t i = 0; i < config.pos_perros.size() && i < NUM_PERROS; i++) {
        pos_perros[i] = config.pos_perros[i];
    }

    perros_capturados = 0;
    ganador = 0;
    partida_activa = true;

    pthread_create(&hilo_juez, NULL, rutina_juez, this);
    pthread_create(&hilo_puma, NULL, rutina_puma, this);
    for (int i = 0; i < NUM_PERROS; i++) {
        ctx_perros[i].mesa = this;
        ctx_perros[i].id_perro = i;
        pthread_create(&hilos_perros[i], NULL, rutina_perro, &ctx_perros[i]);
    }
}

void Mesa::esperar() {
    pthread_join(hilo_juez, NULL);
    pthread_join(hilo_puma, NULL);
    for (int i = 0; i < NUM_PERROS; i++) {
        pthread_join(hilos_perros[i], NULL);
    }
}

void Mesa::detener() {
    pthread_mutex_lock(&mutex_tablero);
    partida_activa = false;
    pthread_mutex_unlock(&mutex_tablero);
}

void Mesa::abortar(int jugador_fallo) {
    pthread_mutex_lock(&mutex_tablero);
    if (partida_activa) {
        partida_activa = false;
        if (jugador_fallo == jugador_puma) {
            ganador = 2;
        } else {
            ganador = 1;
        }
        pthread_mutex_lock(&mutex_consola);
        std::cout << "[Mesa " << id_mesa << "] Abandono del Jugador " << jugador_fallo 
                  << ". Victoria por abandono para el oponente.\n";
        pthread_mutex_unlock(&mutex_consola);
    }
    pthread_mutex_unlock(&mutex_tablero);
}

void* Mesa::rutina_puma(void* arg) {
    Mesa* m = static_cast<Mesa*>(arg);
    m->jugar_puma();
    return NULL;
}

void* Mesa::rutina_perro(void* arg) {
    ContextoPerro* ctx = static_cast<ContextoPerro*>(arg);
    ctx->mesa->jugar_perro(ctx->id_perro);
    return NULL;
}

void* Mesa::rutina_juez(void* arg) {
    Mesa* m = static_cast<Mesa*>(arg);
    m->supervisar_juez();
    return NULL;
}

void Mesa::jugar_puma() {
    while (true) {
        if (terminar_programa) break;

        int r_min = config.retardo_min_ms;
        int r_max = config.retardo_max_ms;
        int retardo = r_min + (rand() % (r_max - r_min + 1));
        
        // Dormir en intervalos pequenos para responder rapido a senales
        int transcurrido = 0;
        while (transcurrido < retardo) {
            int trozo = std::min(20, retardo - transcurrido);
            usleep(trozo * 1000);
            transcurrido += trozo;
            if (terminar_programa || !partida_activa) break;
        }

        pthread_mutex_lock(&mutex_tablero);
        if (!partida_activa || terminar_programa) {
            pthread_mutex_unlock(&mutex_tablero);
            break;
        }

        const std::vector<SaltoCaptura>& saltos = tablero.obtener_saltos(pos_puma);
        std::vector<SaltoCaptura> saltos_validos;
        for (const auto& s : saltos) {
            if (tablero.obtener_casilla(s.victima) == CASILLA_PERRO &&
                tablero.obtener_casilla(s.destino) == CASILLA_VACIA) {
                saltos_validos.push_back(s);
            }
        }

        if (!saltos_validos.empty()) {
            SaltoCaptura elegido = saltos_validos[rand() % saltos_validos.size()];
            tablero.ejecutar_captura(elegido.origen, elegido.victima, elegido.destino);
            pos_puma = elegido.destino;

            for (int i = 0; i < NUM_PERROS; i++) {
                if (pos_perros[i] == elegido.victima) {
                    pos_perros[i] = -1;
                    break;
                }
            }

            perros_capturados++;
            pthread_mutex_lock(&mutex_consola);
            std::cout << "[Mesa " << id_mesa << "] PUMA comio perro en casilla " 
                      << elegido.victima << " saltando a " << elegido.destino 
                      << " (Capturas: " << perros_capturados << "/6)\n";
            pthread_mutex_unlock(&mutex_consola);

            if (perros_capturados >= 6) {
                ganador = 1;
                partida_activa = false;
                pthread_mutex_lock(&mutex_consola);
                std::cout << "[Mesa " << id_mesa << "] Puma alcanza 6 capturas. Victoria del Puma.\n";
                pthread_mutex_unlock(&mutex_consola);
            }
            pthread_mutex_unlock(&mutex_tablero);
            continue;
        }

        const std::vector<int>& vecinos = tablero.obtener_adyacentes(pos_puma);
        std::vector<int> libres;
        for (int v : vecinos) {
            if (tablero.obtener_casilla(v) == CASILLA_VACIA) {
                libres.push_back(v);
            }
        }

        if (!libres.empty()) {
            int dest = libres[rand() % libres.size()];
            tablero.mover_pieza(pos_puma, dest);
            pos_puma = dest;
            pthread_mutex_lock(&mutex_consola);
            std::cout << "[Mesa " << id_mesa << "] PUMA se movio a casilla " << dest << "\n";
            pthread_mutex_unlock(&mutex_consola);
        } else {
            ganador = 2;
            partida_activa = false;
            pthread_mutex_lock(&mutex_consola);
            std::cout << "[Mesa " << id_mesa << "] Puma acorralado sin movimientos. Victoria de los Perros.\n";
            pthread_mutex_unlock(&mutex_consola);
        }

        pthread_mutex_unlock(&mutex_tablero);
    }
}

void Mesa::jugar_perro(int id_perro) {
    while (true) {
        if (terminar_programa) break;

        int r_min = config.retardo_min_ms;
        int r_max = config.retardo_max_ms;
        int retardo = r_min + (rand() % (r_max - r_min + 1));

        int transcurrido = 0;
        while (transcurrido < retardo) {
            int trozo = std::min(20, retardo - transcurrido);
            usleep(trozo * 1000);
            transcurrido += trozo;
            if (terminar_programa || !partida_activa) break;
        }

        pthread_mutex_lock(&mutex_tablero);
        if (!partida_activa || terminar_programa || pos_perros[id_perro] == -1) {
            pthread_mutex_unlock(&mutex_tablero);
            break;
        }

        int pos_actual = pos_perros[id_perro];
        const std::vector<int>& vecinos = tablero.obtener_adyacentes(pos_actual);
        std::vector<int> libres;
        for (int v : vecinos) {
            if (tablero.obtener_casilla(v) == CASILLA_VACIA) {
                libres.push_back(v);
            }
        }

        if (!libres.empty()) {
            int dest = libres[rand() % libres.size()];
            tablero.mover_pieza(pos_actual, dest);
            pos_perros[id_perro] = dest;
            pthread_mutex_lock(&mutex_consola);
            std::cout << "[Mesa " << id_mesa << "] Perro " << id_perro + 1 
                      << " se movio a casilla " << dest << "\n";
            pthread_mutex_unlock(&mutex_consola);
        }

        pthread_mutex_unlock(&mutex_tablero);
    }
}

void Mesa::supervisar_juez() {
    pthread_mutex_lock(&mutex_consola);
    std::cout << "\n[Mesa " << id_mesa << "] Arbitro inicia partida (Puma: Jugador " 
              << jugador_puma << " vs Perros: Jugador " << jugador_perros << ")\n";
    tablero.imprimir(std::cout);
    pthread_mutex_unlock(&mutex_consola);

    while (true) {
        int tiempo = 0;
        while (tiempo < 300) {
            usleep(30000);
            tiempo += 30;
            if (terminar_programa || !partida_activa) break;
        }

        pthread_mutex_lock(&mutex_tablero);
        if (!partida_activa || terminar_programa) {
            pthread_mutex_unlock(&mutex_tablero);
            break;
        }
        pthread_mutex_unlock(&mutex_tablero);
    }

    pthread_mutex_lock(&mutex_consola);
    std::cout << "\n[Mesa " << id_mesa << "] Estado final del tablero:\n";
    tablero.imprimir(std::cout);
    std::cout << "[Mesa " << id_mesa << "] Resumen: ";
    if (ganador == 1) {
        std::cout << "Ganador Jugador " << jugador_puma << " (Puma)\n";
    } else if (ganador == 2) {
        std::cout << "Ganador Jugador " << jugador_perros << " (Perros)\n";
    } else {
        std::cout << "Partida finalizada por cancelacion o interrupcion\n";
    }
    pthread_mutex_unlock(&mutex_consola);
}
