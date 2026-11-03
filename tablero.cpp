#include "tablero.h"
#include <sstream>

Tablero::Tablero() {
    for (int i = 0; i < NUM_CASILLAS; i++) {
        casillas[i] = CASILLA_VACIA;
    }
    construir_grafo();
}

void Tablero::construir_grafo() {
        for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 5; c++) {
            int u = r * 5 + c;
            if (c + 1 < 5) {
                int v = r * 5 + (c + 1);
                adyacencias[u].push_back(v);
                adyacencias[v].push_back(u);
            }
            if (r + 1 < 5) {
                int v = (r + 1) * 5 + c;
                adyacencias[u].push_back(v);
                adyacencias[v].push_back(u);
            }
        }
    }

    adyacencias[20].push_back(25);
    adyacencias[25].push_back(20);

    adyacencias[21].push_back(25);
    adyacencias[25].push_back(21);

    adyacencias[22].push_back(27);
    adyacencias[27].push_back(22);

    adyacencias[23].push_back(26);
    adyacencias[26].push_back(23);

    adyacencias[24].push_back(26);
    adyacencias[26].push_back(24);

    adyacencias[25].push_back(26);
    adyacencias[26].push_back(25);

    adyacencias[25].push_back(27);
    adyacencias[27].push_back(25);

    adyacencias[26].push_back(27);
    adyacencias[27].push_back(26);

    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 5; c++) {
            if (c + 2 < 5) {
                int u = r * 5 + c;
                int v = r * 5 + c + 1;
                int w = r * 5 + c + 2;
                saltos_posibles[u].push_back({u, v, w});
                saltos_posibles[w].push_back({w, v, u});
            }
        }
    }

    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 5; c++) {
            if (r + 2 < 5) {
                int u = r * 5 + c;
                int v = (r + 1) * 5 + c;
                int w = (r + 2) * 5 + c;
                saltos_posibles[u].push_back({u, v, w});
                saltos_posibles[w].push_back({w, v, u});
            }
        }
    }

    saltos_posibles[16].push_back({16, 21, 25});
    saltos_posibles[25].push_back({25, 21, 16});

    saltos_posibles[18].push_back({18, 23, 26});
    saltos_posibles[26].push_back({26, 23, 18});

    saltos_posibles[17].push_back({17, 22, 27});
    saltos_posibles[27].push_back({27, 22, 17});

    saltos_posibles[20].push_back({20, 25, 27});
    saltos_posibles[27].push_back({27, 25, 20});

    saltos_posibles[24].push_back({24, 26, 27});
    saltos_posibles[27].push_back({27, 26, 24});
}

void Tablero::inicializar(int pos_puma, const std::vector<int>& pos_perros) {
    for (int i = 0; i < NUM_CASILLAS; i++) {
        casillas[i] = CASILLA_VACIA;
    }
    if (pos_puma >= 0 && pos_puma < NUM_CASILLAS) {
        casillas[pos_puma] = CASILLA_PUMA;
    }
    for (int p : pos_perros) {
        if (p >= 0 && p < NUM_CASILLAS) {
            casillas[p] = CASILLA_PERRO;
        }
    }
}

int Tablero::obtener_casilla(int pos) const {
    if (pos >= 0 && pos < NUM_CASILLAS) {
        return casillas[pos];
    }
    return CASILLA_VACIA;
}

void Tablero::poner_pieza(int pos, int tipo) {
    if (pos >= 0 && pos < NUM_CASILLAS) {
        casillas[pos] = tipo;
    }
}

void Tablero::mover_pieza(int origen, int destino) {
    if (origen >= 0 && origen < NUM_CASILLAS && destino >= 0 && destino < NUM_CASILLAS) {
        casillas[destino] = casillas[origen];
        casillas[origen] = CASILLA_VACIA;
    }
}

void Tablero::ejecutar_captura(int origen, int victima, int destino) {
    mover_pieza(origen, destino);
    if (victima >= 0 && victima < NUM_CASILLAS) {
        casillas[victima] = CASILLA_VACIA;
    }
}

const std::vector<int>& Tablero::obtener_adyacentes(int pos) const {
    return adyacencias[pos];
}

const std::vector<SaltoCaptura>& Tablero::obtener_saltos(int pos) const {
    return saltos_posibles[pos];
}

static char simbolo_casilla(int val, int idx) {
    if (val == CASILLA_PUMA) return 'P';
    if (val == CASILLA_PERRO) return 'D';
    if (idx == TRONO_POS) return 'T';
    return '.';
}

std::string Tablero::to_string() const {
    std::ostringstream ss;
    char c[NUM_CASILLAS];
    for (int i = 0; i < NUM_CASILLAS; i++) {
        c[i] = simbolo_casilla(casillas[i], i);
    }

    ss << "\n";
    ss << "            (" << c[27] << ")\n";
    ss << "           / | \\\n";
    ss << "        (" << c[25] << ")--|--(" << c[26] << ")\n";
    ss << "       / |   |   | \\\n";
    ss << "    (" << c[20] << ")-(" << c[21] << ")-(" << c[22] << ")-(" << c[23] << ")-(" << c[24] << ")\n";
    ss << "     |   |   |   |   |\n";
    ss << "    (" << c[15] << ")-(" << c[16] << ")-(" << c[17] << ")-(" << c[18] << ")-(" << c[19] << ")\n";
    ss << "     |   |   |   |   |\n";
    ss << "    (" << c[10] << ")-(" << c[11] << ")-(" << c[12] << ")-(" << c[13] << ")-(" << c[14] << ")\n";
    ss << "     |   |   |   |   |\n";
    ss << "    (" << c[5]  << ")-(" << c[6]  << ")-(" << c[7]  << ")-(" << c[8]  << ")-(" << c[9]  << ")\n";
    ss << "     |   |   |   |   |\n";
    ss << "    (" << c[0]  << ")-(" << c[1]  << ")-(" << c[2]  << ")-(" << c[3]  << ")-(" << c[4]  << ")\n";

    return ss.str();
}

void Tablero::imprimir(std::ostream& os) const {
    os << to_string();
}
