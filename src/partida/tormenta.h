// A Tormenta (tormenta.gd do Ventania): a zona segura que encolhe. Determinística
// pela semente (PCG32). Fases: espera → fechamento (o círculo vai de "de" para
// "para") → próxima. Depois da última fase, o círculo final fica parado.
#pragma once
#include "combate/itens.h"
#include "dados_partida.h"
#include "nucleo/mat.h"

class Tormenta {
public:
    static const int TICKS = 30;
    // (centro x, z, raio) → zona aceitável (em terra e longe o bastante do mar).
    typedef bool (*Valido)(const void* ctx, f32 x, f32 z, f32 raio);
    void iniciar(const ConfigPartida& cfg, u32 semente, Valido valido = nullptr, const void* ctx = nullptr);
    // Avança um passo; true quando o estado muda (começou a fechar / fechou).
    bool passo();
    f32 progresso() const;                  // fração do fechamento atual (0..1)
    f32 cx() const, cz() const, raio() const;  // círculo seguro agora
    bool fora(Vec3 p) const;
    int dano_por_segundo() const;
    int ticks_restantes() const;            // até o fim da espera ou do fechamento
    int ticks_total() const;
    int fase = 0;
    bool fechando = false, terminou = false;
    int t = 0;
    f32 de_x = 0, de_z = 0, r_de = 0;       // zona atual (antes do fechamento)
    f32 para_x = 0, para_z = 0, r_para = 0; // próxima zona
    int n_fases() const { return cfg_ ? cfg_->n_fases : 0; }
private:
    void sortear_proxima();
    int ticks(f32 s) const;
    const ConfigPartida* cfg_ = nullptr;
    Aleatorio rng_;
    Valido valido_ = nullptr;
    const void* ctx_ = nullptr;
};
