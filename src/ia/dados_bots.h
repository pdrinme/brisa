// Dados dos bots (data/bots.json + data/nomes_jogadores.txt do Ventania,
// compilados em dados/bots.bdat). Os campos têm os nomes do JSON.
#pragma once
#include <stdio.h>
#include <string.h>

#include "nucleo/tipos.h"

struct DificuldadeBot {
    char nome[16];
    f32 reacao_s, erro_mira_graus, alcance_visao_m, campo_visao_graus, engaja_m, cura_abaixo, constroi, coleta, salto_cedo,
        decisao_hz, mira_assenta_s, memoria_s, investiga, usa_cobertura, cobertura_vida, recua_vida, atira_andando,
        rajada_ticks, pausa_ticks, ritmo_lateral, explora_predios;
};
static_assert(sizeof(DificuldadeBot) == 100, "DificuldadeBot");

class BancoBots {
public:
    bool carregar(const u8* d, u32 t, char* erro, int ne);
    int preencher_ate = 16;
    f32 decisao_hz = 5, decisao_longe_hz = 1, lod_distancia_m = 120, audicao_tiro_m = 70;
    int raios_visao_por_tick = 24;
    int n_dificuldades() const { return n_dif_; }
    const DificuldadeBot& dificuldade(int i) const { return dif_[i]; }
    int n_mistura() const { return n_mis_; }
    int mistura(int i) const { return mis_[i % n_mis_]; }   // índice da dificuldade
    int n_nomes() const { return n_nomes_; }
    const char* nome(int i) const { return nomes_ + (i % n_nomes_) * 24; }
    int n_trajes() const { return n_trajes_; }
    const char* traje(int i) const { return trajes_ + (i % n_trajes_) * 24; }
private:
    const DificuldadeBot* dif_ = nullptr;
    const u8* mis_ = nullptr;
    const char* nomes_ = nullptr;
    const char* trajes_ = nullptr;
    int n_dif_ = 0, n_mis_ = 0, n_nomes_ = 0, n_trajes_ = 0;
};

inline bool BancoBots::carregar(const u8* d, u32 t, char* erro, int ne) {
    struct Cab {
        char magia[4];
        u32 versao, preencher, n_dif, n_mis, n_nomes, n_trajes;
        f32 dec, dec_longe, lod;
        u32 raios;
        f32 audicao;
        u32 off_dif, off_mis, off_nomes, off_trajes;
    };
    static_assert(sizeof(Cab) == 64, "Cab BBOT");
    if (!d || t < 64 || memcmp(d, "BBOT", 4) != 0) { snprintf(erro, ne, "dados dos bots sem assinatura BBOT"); return false; }
    const Cab* c = reinterpret_cast<const Cab*>(d);
    if (c->versao != 1 || c->n_dif == 0 || c->n_mis == 0 || c->n_nomes == 0 || c->n_trajes == 0) { snprintf(erro, ne, "dados dos bots inválidos"); return false; }
    if (c->off_dif + c->n_dif * 100 > t || c->off_mis + c->n_mis > t || c->off_nomes + c->n_nomes * 24 > t || c->off_trajes + c->n_trajes * 24 > t) {
        snprintf(erro, ne, "dados dos bots truncados");
        return false;
    }
    preencher_ate = static_cast<int>(c->preencher);
    decisao_hz = c->dec;
    decisao_longe_hz = c->dec_longe;
    lod_distancia_m = c->lod;
    raios_visao_por_tick = static_cast<int>(c->raios);
    audicao_tiro_m = c->audicao;
    dif_ = reinterpret_cast<const DificuldadeBot*>(d + c->off_dif);
    mis_ = d + c->off_mis;
    nomes_ = reinterpret_cast<const char*>(d + c->off_nomes);
    trajes_ = reinterpret_cast<const char*>(d + c->off_trajes);
    n_dif_ = static_cast<int>(c->n_dif);
    n_mis_ = static_cast<int>(c->n_mis);
    n_nomes_ = static_cast<int>(c->n_nomes);
    n_trajes_ = static_cast<int>(c->n_trajes);
    for (int i = 0; i < n_mis_; i++)
        if (mis_[i] >= n_dif_) { snprintf(erro, ne, "mistura de bots inválida"); return false; }
    return true;
}
