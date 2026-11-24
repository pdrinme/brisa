#include "tormenta.h"

#include <math.h>

void Tormenta::iniciar(const ConfigPartida& cfg, u32 semente, Valido valido, const void* ctx) {
    cfg_ = &cfg;
    rng_.semear(semente);
    valido_ = valido;
    ctx_ = ctx;
    fase = 0;
    fechando = terminou = false;
    t = 0;
    r_de = cfg.raio_inicial;
    de_x = de_z = 0;
    sortear_proxima();
}

int Tormenta::ticks(f32 s) const {
    int n = static_cast<int>(floorf(s * TICKS + 0.5f));
    return n < 1 ? 1 : n;
}

// Centro da próxima zona: dentro da zona atual, de modo que caiba inteira nela.
void Tormenta::sortear_proxima() {
    if (fase >= cfg_->n_fases) return;
    r_para = cfg_->fases[fase].raio;
    f32 folga = r_de - r_para > 0 ? r_de - r_para : 0;
    for (int i = 0; i < 40; i++) {
        f32 ang = rng_.unitario() * 6.2831853f;
        f32 d = sqrtf(rng_.unitario()) * folga;
        f32 x = de_x + cosf(ang) * d, z = de_z + sinf(ang) * d;
        if (sqrtf(x * x + z * z) > cfg_->margem_terra) continue;
        if (valido_ && !valido_(ctx_, x, z, r_para)) continue;
        para_x = x;
        para_z = z;
        return;
    }
    para_x = de_x;
    para_z = de_z;
}

bool Tormenta::passo() {
    if (terminou || !cfg_) return false;
    t++;
    const FaseTormenta& f = cfg_->fases[fase];
    if (!fechando && t >= ticks(f.espera_s)) {
        fechando = true;
        t = 0;
        return true;
    }
    if (fechando && t >= ticks(f.fechamento_s)) {
        de_x = para_x;
        de_z = para_z;
        r_de = r_para;
        fechando = false;
        t = 0;
        fase++;
        if (fase >= cfg_->n_fases) terminou = true;
        else sortear_proxima();
        return true;
    }
    return false;
}

f32 Tormenta::progresso() const {
    if (!fechando || terminou || !cfg_) return 0;
    f32 k = static_cast<f32>(t) / ticks(cfg_->fases[fase].fechamento_s);
    return k < 0 ? 0 : (k > 1 ? 1 : k);
}

f32 Tormenta::cx() const { return de_x + (para_x - de_x) * progresso(); }
f32 Tormenta::cz() const { return de_z + (para_z - de_z) * progresso(); }
f32 Tormenta::raio() const { return r_de + (r_para - r_de) * progresso(); }

bool Tormenta::fora(Vec3 p) const {
    f32 dx = p.x - cx(), dz = p.z - cz();
    return sqrtf(dx * dx + dz * dz) > raio();
}

int Tormenta::dano_por_segundo() const {
    if (!cfg_) return 0;
    int k = fase < cfg_->n_fases ? fase : cfg_->n_fases - 1;
    return static_cast<int>(cfg_->fases[k].dano);
}

int Tormenta::ticks_total() const {
    if (terminou || !cfg_) return 0;
    const FaseTormenta& f = cfg_->fases[fase];
    return ticks(fechando ? f.fechamento_s : f.espera_s);
}

int Tormenta::ticks_restantes() const {
    int r = ticks_total() - t;
    return terminou ? 0 : (r < 0 ? 0 : r);
}
