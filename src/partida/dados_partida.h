// Dados da partida BR (data/partida.json do Ventania → dados/partida.bdat, BPAR).
#pragma once
#include <stdio.h>
#include <string.h>

#include "nucleo/tipos.h"

struct FaseTormenta { f32 espera_s, fechamento_s, raio, dano; };

struct ConfigPartida {
    static const int MAX_FASES = 16;
    f32 contagem_s = 10, fim_s = 15, protecao_inicio_s = 3;
    f32 raio_inicial = 380, margem_terra = 230;
    int n_fases = 0;
    FaseTormenta fases[MAX_FASES];
    bool carregar(const u8* d, u32 t, char* erro, int ne) {
        struct Cab {
            char magia[4];
            u32 versao, n_fases;
            f32 contagem, fim, protecao, raio, margem;
            u32 off_fases;
        };
        static_assert(sizeof(Cab) == 36, "Cab BPAR");
        if (!d || t < 64 || memcmp(d, "BPAR", 4) != 0) { snprintf(erro, ne, "dados da partida sem assinatura BPAR"); return false; }
        Cab c;
        memcpy(&c, d, sizeof c);
        if (c.versao != 1 || c.n_fases == 0 || c.n_fases > MAX_FASES || c.off_fases + c.n_fases * sizeof(FaseTormenta) > t) {
            snprintf(erro, ne, "dados da partida inválidos");
            return false;
        }
        contagem_s = c.contagem; fim_s = c.fim; protecao_inicio_s = c.protecao;
        raio_inicial = c.raio; margem_terra = c.margem;
        n_fases = static_cast<int>(c.n_fases);
        memcpy(fases, d + c.off_fases, n_fases * sizeof(FaseTormenta));
        return true;
    }
};
