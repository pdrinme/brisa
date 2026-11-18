// Estatísticas de tempo de quadro (janela móvel), em microssegundos.
#pragma once
#include "tipos.h"

class EstatisticasQuadro {
public:
    static const int JANELA = 60;
    void registrar(u32 quadro_us, u32 trabalho_us);
    u32 media_quadro_us() const;
    u32 media_trabalho_us() const;
    u32 pior_quadro_us() const;     // pior da janela
    u32 pior_trabalho_us() const;
    u32 pior_trabalho_total_us() const { return pior_trabalho_total_; }  // desde o início
    u32 amostras() const { return n_ < JANELA ? n_ : JANELA; }
    u32 total() const { return n_; }
    // FPS × 10 (ex.: 599 = 59,9), da média da janela.
    u32 fps_x10() const;
private:
    u32 quadro_[JANELA] = {};
    u32 trabalho_[JANELA] = {};
    u32 n_ = 0;
    u32 pior_trabalho_total_ = 0;
};
