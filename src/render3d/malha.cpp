#include "malha.h"

#include <math.h>
#include <string.h>

ConstrutorMalha::ConstrutorMalha(Arena& arena, u32 max_triangulos) {
    max_ = max_triangulos * 3;
    u32 com_folga = max_ + 4;
    pos_ = arena.alocar_n<f32>(com_folga * 4);
    nrm_ = arena.alocar_n<u32>(com_folga);
    cor_ = arena.alocar_n<u32>(com_folga);
    if (!pos_ || !nrm_ || !cor_) { pos_ = nullptr; return; }
    memset(pos_, 0, sizeof(f32) * com_folga * 4);
    memset(nrm_, 0, sizeof(u32) * com_folga);
    memset(cor_, 0, sizeof(u32) * com_folga);
}

void ConstrutorMalha::vertice(Vec3 p, u32 normal, u32 cor) {
    f32* d = pos_ + n_ * 4;
    d[0] = p.x; d[1] = p.y; d[2] = p.z; d[3] = 1.0f;
    nrm_[n_] = normal;
    cor_[n_] = cor;
    n_++;
}

void ConstrutorMalha::triangulo(Vec3 a, Vec3 b, Vec3 c, Cor cor) {
    if (!pos_ || n_ + 3 > max_) { estourou_ = true; return; }
    Vec3 n = normalizar(vetorial(b - a, c - a));
    u32 pn = empacotar_normal(n), pc = empacotar_cor(cor);
    vertice(a, pn, pc);
    vertice(b, pn, pc);
    vertice(c, pn, pc);
}

void ConstrutorMalha::quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Cor cor) {
    triangulo(a, b, c, cor);
    triangulo(a, c, d, cor);
}

void ConstrutorMalha::caixa(Vec3 mn, Vec3 mx, Cor cor, bool com_base) {
    Vec3 p000(mn.x, mn.y, mn.z), p100(mx.x, mn.y, mn.z), p010(mn.x, mx.y, mn.z), p110(mx.x, mx.y, mn.z);
    Vec3 p001(mn.x, mn.y, mx.z), p101(mx.x, mn.y, mx.z), p011(mn.x, mx.y, mx.z), p111(mx.x, mx.y, mx.z);
    quad(p001, p101, p111, p011, cor);  // +z
    quad(p100, p000, p010, p110, cor);  // −z
    quad(p101, p100, p110, p111, cor);  // +x
    quad(p000, p001, p011, p010, cor);  // −x
    quad(p011, p111, p110, p010, cor);  // +y
    if (com_base) quad(p000, p100, p101, p001, cor);  // −y
}

void ConstrutorMalha::cilindro(Vec3 b, f32 r, f32 h, int lados, Cor cor) {
    for (int i = 0; i < lados; i++) {
        f32 a0 = 6.2831853f * i / lados, a1 = 6.2831853f * (i + 1) / lados;
        Vec3 q0(b.x + cosf(a0) * r, b.y, b.z - sinf(a0) * r), q1(b.x + cosf(a1) * r, b.y, b.z - sinf(a1) * r);
        Vec3 t0 = q0 + Vec3(0, h, 0), t1 = q1 + Vec3(0, h, 0);
        quad(q0, q1, t1, t0, cor);
        triangulo(b + Vec3(0, h, 0), t0, t1, cor);
    }
}

void ConstrutorMalha::cone(Vec3 b, f32 r, f32 h, int lados, Cor cor) {
    Vec3 topo = b + Vec3(0, h, 0);
    for (int i = 0; i < lados; i++) {
        f32 a0 = 6.2831853f * i / lados, a1 = 6.2831853f * (i + 1) / lados;
        Vec3 q0(b.x + cosf(a0) * r, b.y, b.z - sinf(a0) * r), q1(b.x + cosf(a1) * r, b.y, b.z - sinf(a1) * r);
        triangulo(q0, q1, topo, cor);
        triangulo(q1, q0, b, cor);  // base (vista de baixo)
    }
}

void ConstrutorMalha::piramide(Vec3 mn, Vec3 mx, f32 h, Cor cor) {
    Vec3 a(mn.x, mn.y, mx.z), b(mx.x, mn.y, mx.z), c(mx.x, mn.y, mn.z), d(mn.x, mn.y, mn.z);
    Vec3 t((mn.x + mx.x) * 0.5f, mn.y + h, (mn.z + mx.z) * 0.5f);
    triangulo(a, b, t, cor);
    triangulo(b, c, t, cor);
    triangulo(c, d, t, cor);
    triangulo(d, a, t, cor);
}

static void esfera_rec(ConstrutorMalha& m, Vec3 a, Vec3 b, Vec3 c, int nivel, Vec3 centro, f32 r, f32 ay, Cor cor) {
    if (nivel == 0) {
        auto ponto_final = [&](Vec3 v) { return Vec3(centro.x + v.x * r, centro.y + v.y * r * ay, centro.z + v.z * r); };
        m.triangulo(ponto_final(a), ponto_final(b), ponto_final(c), cor);
        return;
    }
    Vec3 ab = normalizar(a + b), bc = normalizar(b + c), ca = normalizar(c + a);
    esfera_rec(m, a, ab, ca, nivel - 1, centro, r, ay, cor);
    esfera_rec(m, ab, b, bc, nivel - 1, centro, r, ay, cor);
    esfera_rec(m, ca, bc, c, nivel - 1, centro, r, ay, cor);
    esfera_rec(m, ab, bc, ca, nivel - 1, centro, r, ay, cor);
}

void ConstrutorMalha::esfera(Vec3 centro, f32 r, int sub, Cor cor, f32 ay) {
    const f32 t = 1.6180339f;
    Vec3 v[12] = {{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t},
                  {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
    for (Vec3& p : v) p = normalizar(p);
    static const u8 f[20][3] = {{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4},
                                {11, 10, 2}, {10, 7, 6}, {7, 1, 8}, {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8},
                                {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};
    for (auto& k : f) esfera_rec(*this, v[k[0]], v[k[1]], v[k[2]], sub, centro, r, ay, cor);
}

void ConstrutorMalha::grade(f32 x0, f32 z0, f32 tam, int n, Cor c1, Cor c2, f32 (*h)(f32, f32)) {
    f32 p = tam / n;
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            f32 xa = x0 + i * p, xb = xa + p, za = z0 + j * p, zb = za + p;
            Vec3 a(xa, h(xa, zb), zb), b(xb, h(xb, zb), zb), c(xb, h(xb, za), za), d(xa, h(xa, za), za);
            quad(a, b, c, d, ((i + j) & 1) ? c1 : c2);
        }
}

Malha ConstrutorMalha::terminar() {
    Malha m;
    m.n = n_;
    m.pos = pos_;
    m.normal = nrm_;
    m.cor = cor_;
    if (n_ == 0) return m;
    Vec3 mn(1e30f, 1e30f, 1e30f), mx(-1e30f, -1e30f, -1e30f);
    for (u32 i = 0; i < n_; i++) {
        const f32* p = pos_ + i * 4;
        mn = Vec3(fminf(mn.x, p[0]), fminf(mn.y, p[1]), fminf(mn.z, p[2]));
        mx = Vec3(fmaxf(mx.x, p[0]), fmaxf(mx.y, p[1]), fmaxf(mx.z, p[2]));
    }
    m.centro = (mn + mx) * 0.5f;
    f32 r2 = 0;
    for (u32 i = 0; i < n_; i++) {
        const f32* p = pos_ + i * 4;
        Vec3 d = Vec3(p[0], p[1], p[2]) - m.centro;
        if (ponto(d, d) > r2) r2 = ponto(d, d);
    }
    m.raio = sqrtf(r2);
    return m;
}
