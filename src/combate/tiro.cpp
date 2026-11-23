#include "tiro.h"

#include <math.h>

namespace tiro {

Raio mira(Vec3 pes, bool agachado, f32 yaw, f32 pitch) {
    f32 cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
    Raio r;
    r.origem = pes + Vec3(0, agachado ? OLHO_AGACHADO : OLHO_PE, 0) + Vec3(cy, 0, -sy) * OMBRO;
    r.dir = Vec3(-sy * cp, sp, -cy * cp);
    return r;
}

f32 dispersao(const BancoItens& b, int arma, bool mirando, bool movendo, bool agachado, bool no_chao) {
    const ArmaDado& w = b.arma(arma);
    const ModificadoresDispersao& m = b.modificadores();
    if (mirando && w.dispersao_mirando >= 0) return w.dispersao_mirando;
    f32 d = w.dispersao;
    if (mirando) d *= m.mirando;
    if (movendo) d *= m.movendo;
    if (agachado) d *= m.agachado;
    if (!no_chao) d *= m.no_ar;
    return d;
}

void projeteis(Vec3 dir, f32 graus, int n, u32 semente, Vec3* saida) {
    Aleatorio r;
    r.semear(semente);
    Vec3 lado = vetorial(dir, Vec3(0, 1, 0));
    if (ponto(lado, lado) < 1e-4f) lado = Vec3(1, 0, 0);
    lado = normalizar(lado);
    Vec3 cima = normalizar(vetorial(lado, dir));
    f32 rad = graus * 3.14159265f / 180.0f;
    for (int i = 0; i < n && i < MAX_PROJETEIS; i++) {
        f32 ang = r.unitario() * 2 * 3.14159265f;
        f32 raio = sqrtf(r.unitario()) * rad;
        saida[i] = normalizar(dir + (lado * cosf(ang) + cima * sinf(ang)) * tanf(raio));
    }
}

int dano_projetil(const BancoItens& b, int arma, int raridade, bool cabeca) {
    f32 d = b.dano(arma, raridade);
    if (cabeca) d *= b.arma(arma).cabeca;
    return static_cast<int>(floorf(d + 0.5f));
}

Forma forma(Vec3 p, bool agachado) {
    if (agachado) return {p + Vec3(0, 0.35f, 0), p + Vec3(0, 0.8f, 0), p + Vec3(0, 1.1f, 0)};
    return {p + Vec3(0, 0.4f, 0), p + Vec3(0, 1.3f, 0), p + Vec3(0, 1.62f, 0)};
}

f32 raio_esfera(Vec3 o, Vec3 d, Vec3 c, f32 r) {
    Vec3 oc = o - c;
    f32 b = ponto(oc, d), cc = ponto(oc, oc) - r * r, disc = b * b - cc;
    if (disc < 0) return -1;
    f32 t = -b - sqrtf(disc);
    if (t < 0) t = -b + sqrtf(disc);  // origem dentro da esfera
    return t >= 0 ? t : -1;
}

f32 raio_capsula(Vec3 o, Vec3 d, Vec3 a, Vec3 b, f32 r) {
    Vec3 ab = b - a;
    int n = static_cast<int>(ceilf(sqrtf(ponto(ab, ab)) / 0.15f)) + 1;
    if (n < 2) n = 2;
    f32 melhor = -1;
    for (int i = 0; i < n; i++) {
        f32 t = raio_esfera(o, d, a + ab * (static_cast<f32>(i) / (n - 1)), r);
        if (t >= 0 && (melhor < 0 || t < melhor)) melhor = t;
    }
    return melhor;
}

Acerto testar(Vec3 o, Vec3 d, Vec3 pes, bool agachado) {
    Forma f = forma(pes, agachado);
    f32 th = raio_esfera(o, d, f.cabeca, RAIO_CABECA), tb = raio_capsula(o, d, f.a, f.b, RAIO_CORPO);
    Acerto a;
    if (th >= 0 && (tb < 0 || th <= tb + 0.1f)) { a.t = th; a.cabeca = true; }
    else if (tb >= 0) a.t = tb;
    return a;
}

int passos_golpe(const BancoItens& b) {
    int n = static_cast<int>(floorf(30.0f / b.ferramenta(b.picareta()).cadencia + 0.5f));
    return n < 6 ? 6 : n;
}

int passo_impacto(const BancoItens& b) {
    int n = passos_golpe(b), k = static_cast<int>(floorf(n * b.ferramenta(b.picareta()).impacto + 0.5f));
    return k < 1 ? 1 : (k > n - 1 ? n - 1 : k);
}

f32 alcance_golpe(const BancoItens& b) { return b.ferramenta(b.picareta()).alcance; }

}  // namespace tiro
