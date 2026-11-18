// Modelos carregados do pacote por id (sem cópia), com cache simples.
#pragma once
#include "modelo.h"
#include "pak.h"

class BibliotecaModelos {
public:
    static const int MAX = 64;
    void iniciar(const Pacote* pak) { pak_ = pak; n_ = 0; bytes_ = 0; }
    // Índice do modelo "modelos/<id>.bmod", ou −1 (erro em `erro`).
    int carregar(const char* id, char* erro, int n_erro);
    const ModeloBrisa& operator[](int i) const { return m_[i]; }
    int n() const { return n_; }
    u32 bytes() const { return bytes_; }
private:
    const Pacote* pak_ = nullptr;
    ModeloBrisa m_[MAX];
    char id_[MAX][32];
    int n_ = 0;
    u32 bytes_ = 0;
};
