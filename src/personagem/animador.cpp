#include "animador.h"

#include <math.h>

namespace {
f32 aproximar(f32 v, f32 alvo, f32 d) { return v < alvo ? fminf(v + d, alvo) : fmaxf(v - d, alvo); }
// Peso de um one-shot com entrada e saída (fadein/fadeout do AnimationNodeOneShot).
f32 peso_oneshot(f32 t, f32 dur, f32 entra, f32 sai) {
    if (t < 0 || t > dur) return 0;
    f32 w = 1;
    if (entra > 0 && t < entra) w = t / entra;
    if (sai > 0 && t > dur - sai) w = fminf(w, (dur - t) / sai);
    return w < 0 ? 0 : w;
}
}  // namespace

bool Animador::iniciar(const BancoAnimacoes& b) {
    b_ = &b;
    idle_ = b.achar("idle"); andar_ = b.achar("andar"); correr_ = b.achar("correr");
    ag_ = b.achar("agachado"); ag_andar_ = b.achar("agachado_andar");
    pular_ = b.achar("pular"); queda_ = b.achar("queda"); aterrissar_ = b.achar("aterrissar");
    pesado_ = b.achar("aterrissar_pesado"); acenar_ = b.achar("acenar");
    return idle_ >= 0 && andar_ >= 0 && correr_ >= 0 && ag_ >= 0 && ag_andar_ >= 0 && pular_ >= 0 && queda_ >= 0 &&
           aterrissar_ >= 0 && pesado_ >= 0 && acenar_ >= 0;
}

void Animador::atualizar(const Personagem& p, bool acenar, f32 dt) {
    vel_ = sqrtf(p.vel.x * p.vel.x + p.vel.z * p.vel.z);
    // Fase da locomoção: avança pela duração do clipe que domina na velocidade atual.
    f32 dur;
    if (vel_ < mov::ANDAR) {
        f32 w = vel_ / mov::ANDAR;
        dur = b_->duracao(idle_) * (1 - w) + b_->duracao(andar_) * w;
    } else {
        f32 w = fminf((vel_ - mov::ANDAR) / (mov::CORRER - mov::ANDAR), 1.0f);
        dur = b_->duracao(andar_) * (1 - w) + b_->duracao(correr_) * w;
    }
    fase_ = fmodf(fase_ + dt / (dur > 0.05f ? dur : 0.05f), 1.0f);
    f32 dur_ag = b_->duracao(ag_) * (1 - fminf(vel_ / mov::AGACHADO, 1.0f)) + b_->duracao(ag_andar_) * fminf(vel_ / mov::AGACHADO, 1.0f);
    fase_ag_ = fmodf(fase_ag_ + dt / (dur_ag > 0.05f ? dur_ag : 0.05f), 1.0f);
    w_ag_ = aproximar(w_ag_, p.agachado ? 1.0f : 0.0f, dt / 0.15f);
    // Ar: entra rápido; o pulo é um one-shot sobre a queda.
    if (p.pulou) t_pulo_ = 0;
    if (!p.no_chao) { t_ar_ += dt; w_ar_ = aproximar(w_ar_, 1, dt / 0.08f); }
    else { t_ar_ = 0; w_ar_ = aproximar(w_ar_, 0, dt / 0.05f); }
    if (t_pulo_ >= 0) { t_pulo_ += dt; if (t_pulo_ > b_->duracao(pular_) + 0.25f) t_pulo_ = -1; }
    if (p.aterrissou) {
        if (p.altura_queda >= mov::QUEDA_PESADA) t_pesado_ = 0;
        else if (p.altura_queda > 0.6f) t_aterr_ = 0;
        t_pulo_ = -1;
    }
    if (t_aterr_ >= 0) { t_aterr_ += dt; if (t_aterr_ > b_->duracao(aterrissar_)) t_aterr_ = -1; }
    if (t_pesado_ >= 0) { t_pesado_ += dt; if (t_pesado_ > b_->duracao(pesado_)) t_pesado_ = -1; }
    if (acenar && t_acenar_ < 0) t_acenar_ = 0;
    if (t_acenar_ >= 0) { t_acenar_ += dt; if (t_acenar_ > b_->duracao(acenar_)) t_acenar_ = -1; }
}

void Animador::pose(const ModeloBrisa& m, Pose& saida) const {
    const int N = m.n_nos();
    TRSLocal base[ModeloBrisa::MAX_NOS], tmp[ModeloBrisa::MAX_NOS];
    // Em pé: idle → andar → correr pela velocidade, todos na mesma fase.
    auto amostra_fase = [&](int clipe, f32 fase, TRSLocal* out) { b_->amostrar_local(clipe, fase * b_->duracao(clipe), m, out); };
    if (vel_ < mov::ANDAR) {
        amostra_fase(idle_, fase_, base);
        amostra_fase(andar_, fase_, tmp);
        misturar_trs(base, tmp, N, vel_ / mov::ANDAR);
    } else {
        amostra_fase(andar_, fase_, base);
        amostra_fase(correr_, fase_, tmp);
        misturar_trs(base, tmp, N, fminf((vel_ - mov::ANDAR) / (mov::CORRER - mov::ANDAR), 1.0f));
    }
    if (w_ag_ > 0) {
        TRSLocal ag[ModeloBrisa::MAX_NOS];
        amostra_fase(ag_, fase_ag_, ag);
        amostra_fase(ag_andar_, fase_ag_, tmp);
        misturar_trs(ag, tmp, N, fminf(vel_ / mov::AGACHADO, 1.0f));
        misturar_trs(base, ag, N, w_ag_);
    }
    if (w_ar_ > 0) {
        TRSLocal ar[ModeloBrisa::MAX_NOS];
        b_->amostrar_local(queda_, t_ar_, m, ar);
        if (t_pulo_ >= 0) {
            b_->amostrar_local(pular_, t_pulo_, m, tmp);
            misturar_trs(ar, tmp, N, peso_oneshot(t_pulo_, b_->duracao(pular_) + 0.25f, 0.05f, 0.25f));
        }
        misturar_trs(base, ar, N, w_ar_);
    }
    if (t_aterr_ >= 0) {
        b_->amostrar_local(aterrissar_, t_aterr_, m, tmp);
        misturar_trs(base, tmp, N, peso_oneshot(t_aterr_, b_->duracao(aterrissar_), 0.04f, 0.22f));
    }
    if (t_pesado_ >= 0) {
        b_->amostrar_local(pesado_, t_pesado_, m, tmp);
        misturar_trs(base, tmp, N, peso_oneshot(t_pesado_, b_->duracao(pesado_), 0.03f, 0.25f));
    }
    if (t_acenar_ >= 0) {
        b_->amostrar_local(acenar_, t_acenar_, m, tmp);
        misturar_trs(base, tmp, N, peso_oneshot(t_acenar_, b_->duracao(acenar_), 0.2f, 0.3f));
    }
    compor_pose(m, base, saida);
}

const char* Animador::estado() const {
    if (t_pesado_ >= 0) return "aterrissar_pesado";
    if (t_aterr_ >= 0) return "aterrissar";
    if (w_ar_ > 0.5f) return t_pulo_ >= 0 ? "pular" : "queda";
    if (t_acenar_ >= 0) return "acenar";
    if (w_ag_ > 0.5f) return vel_ > 0.3f ? "agachado_andar" : "agachado";
    if (vel_ < 0.3f) return "idle";
    return vel_ > mov::ANDAR + 0.5f ? "correr" : "andar";
}
