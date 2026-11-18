// Animação do personagem a partir do movimento (a árvore de
// gameplay/character/pawn_visual.gd do Ventania): idle/andar/correr misturados
// pela velocidade com fase sincronizada, agachado/agachado_andar, pulo (one-shot)
// sobre a queda, aterrissagem, aterrissagem pesada (queda alta) e aceno.
#pragma once
#include "conteudo/animacao.h"
#include "movimento.h"

class Animador {
public:
    bool iniciar(const BancoAnimacoes& banco);
    void atualizar(const Personagem& p, bool acenar, f32 dt);
    void pose(const ModeloBrisa& m, Pose& saida) const;
    const char* estado() const;   // para o HUD e os testes
    f32 peso_ar() const { return w_ar_; }
    f32 peso_agachado() const { return w_ag_; }
    f32 velocidade() const { return vel_; }
private:
    const BancoAnimacoes* b_ = nullptr;
    int idle_, andar_, correr_, ag_, ag_andar_, pular_, queda_, aterrissar_, pesado_, acenar_;
    f32 fase_ = 0, fase_ag_ = 0, t_ar_ = 0, t_pulo_ = -1, t_aterr_ = -1, t_pesado_ = -1, t_acenar_ = -1;
    f32 vel_ = 0, w_ar_ = 0, w_ag_ = 0;
};
