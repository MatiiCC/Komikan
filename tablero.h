#ifndef TABLERO_H
#define TABLERO_H

#include <vector>
#include <string>
#include <iostream>

constexpr int NUM_CASILLAS = 28;
constexpr int NUM_PERROS = 12;
constexpr int TRONO_POS = 27;

enum TipoCasilla {
    CASILLA_VACIA = 0,
    CASILLA_PUMA = 1,
    CASILLA_PERRO = 2
};

struct SaltoCaptura {
    int origen;
    int victima;
    int destino;
};

class Tablero {
private:
    int casillas[NUM_CASILLAS];
    std::vector<int> adyacencias[NUM_CASILLAS];
    std::vector<SaltoCaptura> saltos_posibles[NUM_CASILLAS];

    void construir_grafo();

public:
    Tablero();

    void inicializar(int pos_puma, const std::vector<int>& pos_perros);

    int obtener_casilla(int pos) const;

    void poner_pieza(int pos, int tipo);

    void mover_pieza(int origen, int destino);

    void ejecutar_captura(int origen, int victima, int destino);

    const std::vector<int>& obtener_adyacentes(int pos) const;
    const std::vector<SaltoCaptura>& obtener_saltos(int pos) const;

    void imprimir(std::ostream& os = std::cout) const;
    std::string to_string() const;
};

#endif 
