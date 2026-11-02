# Torneo de Komikan Concurrente

Este proyecto implementa una simulación concurrente y multihilo en tiempo real del juego tradicional mapuche **Komikan**, desarrollada en **C++17** utilizando la biblioteca de hilos y sincronización **POSIX Threads (`pthreads`)**.

El sistema modela un servidor de torneo (*Matchmaker*) capaz de recibir $N$ jugadores y arbitrar múltiples mesas de juego de forma concurrente, con un límite máximo de $M$ mesas simultáneas administradas mediante semáforos POSIX. Cada partida se disputa en tiempo real y sin turnos tradicionales: las piezas (un Puma y 12 Perros) son entidades autónomas que deciden y compiten concurrentemente por las casillas del tablero.

---

## Requisitos de Compilación

* Sistema Operativo: Linux / UNIX compatible con estándares POSIX.
* Compilador: `g++` con soporte para C++17.
* Biblioteca de hilos: `pthread` (POSIX Threads).
* Utilidad de construcción: `make`.

---

## Compilación y Limpieza

El proyecto incluye un `Makefile` configurado con banderas de optimización y verificación estricta de advertencias (`-Wall -Wextra -std=c++17 -lpthread`):

* **Compilar el proyecto:**
  ```bash
  make
  ```
  Esto generará el ejecutable binario `torneo` a partir de los módulos fuente.

* **Limpiar binarios y archivos objeto:**
  ```bash
  make clean
  ```

---

## Instrucciones de Ejecución

El programa ejecutable recibe argumentos por línea de comandos:

```bash
./torneo N M [config.txt]
```

### Parámetros:
1. **`N` (obligatorio):** Número total de jugadores que ingresan al torneo ($N \ge 2$). Se emparejan en duplas de 2 jugadores por mesa (un jugador asume el Puma y el otro los 12 Perros). Si $N$ es impar, el último jugador queda en espera.
2. **`M` (obligatorio):** Número máximo de mesas que pueden jugarse de forma simultánea ($M \ge 1$).
3. **`config.txt` (opcional):** Ruta a un archivo de configuración para personalizar retardos, semilla aleatoria y posiciones iniciales. Si no se especifica o no existe, se utilizan los valores estándar por defecto.

### Ejemplos de uso:

* **Ejecutar 2 jugadores en 1 mesa (partida individual):**
  ```bash
  ./torneo 2 1 config.txt
  ```

* **Ejecutar 6 jugadores con un máximo de 2 mesas simultáneas:**
  ```bash
  ./torneo 6 2 config.txt
  ```

* **Ejecutar 10 jugadores con máximo 3 mesas simultáneas:**
  ```bash
  ./torneo 10 3 config.txt
  ```

---

## Archivo de Configuración (`config.txt`)

El archivo de configuración permite ajustar parámetros del juego mediante pares `clave = valor`. Las líneas que comienzan con `#` o vacías son ignoradas:

```ini
# Configuración del Komikan
retardo_min = 50           # Retardo mínimo de decisión por pieza (ms)
retardo_max = 300          # Retardo máximo de decisión por pieza (ms)
seed = 42                  # Semilla para números pseudoaleatorios (0 = time(NULL))
posicion_puma = 27         # Casilla inicial del Puma (0 a 27, 27 = Trono)
posiciones_perros = 0 1 2 3 4 5 6 7 8 9 10 14  # Casillas iniciales de los 12 perros
```

---

## Arquitectura del Sistema y Concurrencia

El diseño del proyecto se fundamenta en los siguientes mecanismos de sistemas operativos:

```
                  +-----------------------------------+
                  |  Servidor Matchmaker (torneo.cpp) |
                  |    N clientes | sem_mesas (cap M) |
                  +-----------------+-----------------+
                                    |
                    +---------------+---------------+
                    |                               |
              [Mesa Activa 1]                 [Mesa Activa 2]
         +-----------------------+       +-----------------------+
         | - Hilo Puma (1)       |       | - Hilo Puma (1)       |
         | - Hilos Perros (12)   |       | - Hilos Perros (12)   |
         | - Hilo Árbitro (1)    |       | - Hilo Árbitro (1)    |
         | - Mutex Tablero       |       | - Mutex Tablero       |
         +-----------------------+       +-----------------------+
```

### 1. Servidor Matchmaker y Control de Capacidad (`sem_t`)
* El hilo principal (`torneo.cpp`) empareja a los jugadores de 2 en 2.
* Para respetar el límite $M$, se utiliza un semáforo contador POSIX (`sem_mesas`) inicializado en $M$.
* Cada dupla solicita ingreso mediante `sem_wait(&sem_mesas)`. Si hay $M$ mesas ocupadas, el hilo espera pasivamente sin consumir CPU (*busy waiting*) hasta que una mesa finalice y libere un cupo con `sem_post(&sem_mesas)`.

### 2. Estructura Concurrente por Mesa (14 Hilos por Partida)
Cada mesa instanciada opera de forma completamente autónoma con:
* **1 hilo para el Puma:** Evalúa periódicamente saltos de captura válidos en línea recta sobre perros adyacentes y movimientos a casillas libres.
* **12 hilos independientes para los Perros:** Cada hilo controla un perro específico, intentando avanzar hacia casillas contiguas libres para cercar al Puma.
* **1 hilo Árbitro:** Supervisa periódicamente la partida, imprime el estado del tablero y declara al ganador cuando se cumple una condición de término.

### 3. Exclusión Mutua (`pthread_mutex_t`)
* **`mutex_tablero` (por mesa):** Protege atómicamente el estado del grafo del tablero (28 casillas). Cuando una pieza decide moverse o saltar, adquiere el mutex de su mesa, verifica la validez del movimiento en el estado actual, ejecuta el cambio y libera el cerrojo. Esto previene condiciones de carrera cuando dos o más piezas compiten por llegar a la misma casilla vacía.
* **`mutex_consola` (global):** Garantiza que los mensajes de log y las representaciones visuales del tablero se impriman de manera atómica, impidiendo que las salidas de mesas paralelas se mezclen o corten en la terminal.

### 4. Manejo Resiliente de Señales (`SIGINT` / `Ctrl+C`)
* Se instala una rutina para `SIGINT` mediante la llamada al sistema `sigaction`.
* El manejador de señal cumple estrictamente con el estándar *async-signal-safe*: únicamente marca la bandera atómica `terminar_programa = 1` y emite una notificación mediante `write()`, sin invocar funciones inseguras como `std::cout` o bloqueos de mutex en el contexto de la señal.
* Las llamadas bloqueantes (`sem_wait`) gestionan el retorno `EINTR`.
* Ante una interrupción, el servidor detiene todas las partidas en curso de forma ordenada, los hilos despiertan y finalizan en milisegundos, y se realiza un `pthread_join` exhaustivo de todos los hilos creados, garantizando la liberación de semáforos y memoria sin dejar hilos huérfanos ni fugas.

---

## Reglas y Condiciones de Término

El tablero consta de 28 casillas: una grilla ortogonal de $5 \times 5$ (casillas 0 a 24) más un trono triangular superior (casillas 25, 26 y 27).

* **Victoria del Puma:** Ocurre si el Puma logra capturar a **6 perros** mediante saltos en línea recta sobre casillas ocupadas hacia una casilla libre inmediatamente posterior.
* **Victoria de los Perros:** Ocurre si los perros logran bloquear completamente al Puma, dejándolo sin casillas adyacentes libres ni opciones válidas de captura.
