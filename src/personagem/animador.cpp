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
    queda_livre_ = b.achar("queda_livre"); mergulho_ = b.achar("mergulho");
    abrir_ = b.achar("planador_abrir"); planador_ = b.achar("planador");
    return queda_livre_ >= 0 && mergulho_ >= 0 && abrir_ >= 0 && planador_ >= 0 && idle_ >= 0 && andar_ >= 0 && correr_ >= 0 && ag_ >= 0 && ag_andar_ >= 0 && pular_ >= 0 && queda_ >= 0 &&
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
    // Entrada na partida: pose pela fase da queda e pela velocidade (fall_pose do Ventania).
    bool no_ar = p.queda == QUEDA_LIVRE || p.queda == PLANADOR;
    bool merg = p.queda == QUEDA_LIVRE && -p.vel.y > mov::QUEDA_TERMINAL + 2.0f;
    w_ent_ = aproximar(w_ent_, no_ar ? 1.0f : 0.0f, dt * (no_ar ? 6.0f : 3.0f));
    w_merg_ = aproximar(w_merg_, merg ? 1.0f : 0.0f, dt * 2.5f);
    w_plan_ = aproximar(w_plan_, p.queda == PLANADOR ? 1.0f : 0.0f, dt * 5.0f);
    t_ent_ = no_ar ? t_ent_ + dt : 0;
    t_plan_ = p.queda == PLANADOR ? t_plan_ + dt : 0;
    if (no_ar) { w_ar_ = 0; t_pulo_ = -1; }
    // Inclinação: de bruços na queda livre, de cabeça para baixo no mergulho, quase em pé
    // no planador; rola nas curvas pela velocidade lateral (air_tilt), com uma oscilação leve.
    f32 cs = cosf(p.yaw), sn = sinf(p.yaw);
    f32 lx = cs * p.vel.x - sn * p.vel.z, lz = sn * p.vel.x + cs * p.vel.z;
    f32 lado = fmaxf(-1.0f, fminf(1.0f, lx / mov::QUEDA_HORIZONTAL));
    f32 frente = fmaxf(-1.0f, fminf(1.0f, -lz / mov::QUEDA_HORIZONTAL));
    f32 a_deitar = 0, a_rolar = 0;
    const f32 G = 0.01745329f;
    if (p.queda == QUEDA_LIVRE) {
        f32 base = (72.0f + 10.0f * frente) * G;
        a_deitar = base + (158.0f * G - base) * w_merg_;
        a_rolar = -lado * 28.0f * G;
    } else if (p.queda == PLANADOR) {
        a_deitar = (6.0f + 8.0f * frente) * G;
        a_rolar = -lado * 20.0f * G;
    }
    if (no_ar) a_rolar += sinf(t_ent_ * (p.queda == PLANADOR ? 1.7f : 2.3f)) * (p.queda == PLANADOR ? 2.0f : 3.0f) * G;
    f32 k = fminf(dt * 5.0f, 1.0f);
    deitar_ += (a_deitar - deitar_) * k;
    rolar_ += (a_rolar - rolar_) * k;
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
    if (w_ent_ > 0) {
        TRSLocal ent[ModeloBrisa::MAX_NOS];
        b_->amostrar_local(queda_livre_, t_ent_, m, ent);
        if (w_merg_ > 0) {
            b_->amostrar_local(mergulho_, t_ent_, m, tmp);
            misturar_trs(ent, tmp, N, w_merg_);
        }
        if (w_plan_ > 0) {
            f32 da = b_->duracao(abrir_);
            if (t_plan_ < da) b_->amostrar_local(abrir_, t_plan_, m, tmp);
            else b_->amostrar_local(planador_, t_plan_ - da, m, tmp);
            misturar_trs(ent, tmp, N, w_plan_);
        }
        misturar_trs(base, ent, N, w_ent_);
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
    if (w_ent_ > 0.5f) return w_plan_ > 0.5f ? "planador" : (w_merg_ > 0.5f ? "mergulho" : "queda_livre");
    if (t_pesado_ >= 0) return "aterrissar_pesado";
    if (t_aterr_ >= 0) return "aterrissar";
    if (w_ar_ > 0.5f) return t_pulo_ >= 0 ? "pular" : "queda";
    if (t_acenar_ >= 0) return "acenar";
    if (w_ag_ > 0.5f) return vel_ > 0.3f ? "agachado_andar" : "agachado";
    if (vel_ < 0.3f) return "idle";
    return vel_ > mov::ANDAR + 0.5f ? "correr" : "andar";
}
