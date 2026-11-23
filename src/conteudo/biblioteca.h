// Modelos carregados do pacote por id (sem cópia), com cache simples.
#pragma once
#include "modelo.h"
#include "nucleo/arena.h"
#include "pak.h"

class BibliotecaModelos {
public:
    // Reserva `capacidade` modelos na arena (cerca de 1,5 KB cada).
    bool iniciar(const Pacote* pak, Arena& arena, int capacidade);
    // Índice do modelo "modelos/<id>.bmod", ou −1 (erro em `erro`).
    int carregar(const char* id, char* erro, int n_erro);
    const ModeloBrisa& operator[](int i) const { return m_[i]; }
    int n() const { return n_; }
    int capacidade() const { return max_; }
    u32 bytes() const { return bytes_; }
private:
    const Pacote* pak_ = nullptr;
    ModeloBrisa* m_ = nullptr;
    char (*id_)[32] = nullptr;
    int n_ = 0, max_ = 0;
    u32 bytes_ = 0;
};
