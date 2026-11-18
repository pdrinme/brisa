#include "animacao.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

bool BancoAnimacoes::carregar(const u8* d, u32 t, char* erro, int ne) {
    if (!d || t < 32 || memcmp(d, "BANI", 4) != 0) { snprintf(erro, ne, "animações sem assinatura BANI"); return false; }
    u32 c[5];
    memcpy(c, d + 4, sizeof c);
    if (c[0] != VERSAO) { snprintf(erro, ne, "versão das animações %u (o jogo lê a %u)", static_cast<unsigned>(c[0]), static_cast<unsigned>(VERSAO)); return false; }
    n_nos_ = c[1];
    n_clipes_ = static_cast<int>(c[2]);
    fps_ = c[3] ? c[3] : 30;
    assinatura_ = c[4];
    if (n_nos_ > static_cast<u32>(ModeloBrisa::MAX_NOS) || 32 + 48 * c[2] > t) { snprintf(erro, ne, "animações inválidas"); return false; }
    clipes_ = reinterpret_cast<const Clipe*>(d + 32);
    for (int i = 0; i < n_clipes_; i++)
        if (clipes_[i].offset + clipes_[i].n_quadros * n_nos_ * sizeof(Amostra) > t || clipes_[i].n_quadros == 0) {
            snprintf(erro, ne, "clipe %d fora do arquivo", i);
            return false;
        }
    dados_ = d;
    tamanho_ = t;
    return true;
}

int BancoAnimacoes::achar(const char* nome) const {
    for (int i = 0; i < n_clipes_; i++)
        if (strncmp(clipes_[i].nome, nome, sizeof clipes_[i].nome) == 0) return i;
    return -1;
}

const char* BancoAnimacoes::nome(int i) const { return (i >= 0 && i < n_clipes_) ? clipes_[i].nome : ""; }
bool BancoAnimacoes::loop(int i) const { return i >= 0 && i < n_clipes_ && clipes_[i].loop; }
f32 BancoAnimacoes::duracao(int i) const {
    return (i >= 0 && i < n_clipes_) ? static_cast<f32>(clipes_[i].n_quadros - 1) / static_cast<f32>(fps_) : 0.0f;
}

void misturar_trs(TRSLocal* a, const TRSLocal* b, int n, f32 w) {
    if (w <= 0) return;
    if (w >= 1) {
        for (int i = 0; i < n; i++) a[i] = b[i];
        return;
    }
    for (int i = 0; i < n; i++) {
        f32 d = a[i].q[0] * b[i].q[0] + a[i].q[1] * b[i].q[1] + a[i].q[2] * b[i].q[2] + a[i].q[3] * b[i].q[3];
        f32 s = d < 0 ? -1.0f : 1.0f, len = 0;
        for (int k = 0; k < 4; k++) {
            a[i].q[k] += (b[i].q[k] * s - a[i].q[k]) * w;
            len += a[i].q[k] * a[i].q[k];
        }
        len = len > 1e-12f ? 1.0f / sqrtf(len) : 0.0f;
        for (int k = 0; k < 4; k++) a[i].q[k] *= len;
        a[i].t = a[i].t + (b[i].t - a[i].t) * w;
    }
}

void trs_repouso(const ModeloBrisa& m, TRSLocal* l) {
    for (int i = 0; i < m.n_nos(); i++) {
        for (int k = 0; k < 4; k++) l[i].q[k] = m.no(i).r[k];
        l[i].t = Vec3(m.no(i).t[0], m.no(i).t[1], m.no(i).t[2]);
    }
}

void compor_pose(const ModeloBrisa& m, const TRSLocal* l, Pose& p) {
    p.n = m.n_nos();
    for (int i = 0; i < p.n; i++) {
        const NoBMOD& no = m.no(i);
        Mat4 local = Mat4::trs(l[i].t, l[i].q, Vec3(no.s[0], no.s[1], no.s[2]));
        p.mundo[i] = no.pai < 0 ? local : p.mundo[no.pai] * local;
    }
}

void BancoAnimacoes::amostrar(int ci, f32 t, const ModeloBrisa& m, Pose& p) const {
    TRSLocal l[ModeloBrisa::MAX_NOS];
    amostrar_local(ci, t, m, l);
    compor_pose(m, l, p);
}

void BancoAnimacoes::amostrar_local(int ci, f32 t, const ModeloBrisa& m, TRSLocal* out) const {
    if (ci < 0 || ci >= n_clipes_ || static_cast<u32>(m.n_nos()) != n_nos_) {
        trs_repouso(m, out);
        return;
    }
    const Clipe& c = clipes_[ci];
    f32 f = t * static_cast<f32>(fps_);
    u32 n = c.n_quadros;
    u32 q0, q1;
    f32 u;
    if (n == 1) {
        q0 = q1 = 0;
        u = 0;
    } else if (c.loop) {
        // O último quadro do laço é igual ao primeiro: o período é n − 1 quadros.
        f32 periodo = static_cast<f32>(n - 1);
        f = fmodf(f, periodo);
        if (f < 0) f += periodo;
        q0 = static_cast<u32>(f);
        q1 = q0 + 1 < n ? q0 + 1 : 0;
        u = f - static_cast<f32>(q0);
    } else {
        if (f < 0) f = 0;
        if (f >= static_cast<f32>(n - 1)) { f = static_cast<f32>(n - 1); }
        q0 = static_cast<u32>(f);
        q1 = q0 + 1 < n ? q0 + 1 : q0;
        u = f - static_cast<f32>(q0);
    }
    const Amostra* a0 = reinterpret_cast<const Amostra*>(dados_ + c.offset) + q0 * n_nos_;
    const Amostra* a1 = reinterpret_cast<const Amostra*>(dados_ + c.offset) + q1 * n_nos_;
    for (int i = 0; i < m.n_nos(); i++) {
        const NoBMOD& no = m.no(i);
        f32 q[4], qb[4];
        f32 dot = 0;
        for (int k = 0; k < 4; k++) {
            q[k] = a0[i].q[k] / 32767.0f;
            qb[k] = a1[i].q[k] / 32767.0f;
            dot += q[k] * qb[k];
        }
        f32 s = dot < 0 ? -1.0f : 1.0f;
        f32 len = 0;
        for (int k = 0; k < 4; k++) {
            q[k] = q[k] + (qb[k] * s - q[k]) * u;
            len += q[k] * q[k];
        }
        len = len > 1e-12f ? 1.0f / sqrtf(len) : 0.0f;
        for (int k = 0; k < 4; k++) q[k] *= len;
        for (int k = 0; k < 4; k++) out[i].q[k] = q[k];
        out[i].t = Vec3((a0[i].t[0] + (a1[i].t[0] - a0[i].t[0]) * u) * 0.001f, (a0[i].t[1] + (a1[i].t[1] - a0[i].t[1]) * u) * 0.001f,
                        (a0[i].t[2] + (a1[i].t[2] - a0[i].t[2]) * u) * 0.001f);
        (void)no;
    }
}
