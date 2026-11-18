#include "mat.h"

#include <math.h>

f32 comprimento(Vec3 a) { return sqrtf(ponto(a, a)); }
Vec3 normalizar(Vec3 a) {
    f32 c = comprimento(a);
    return c > 1e-12f ? a * (1.0f / c) : Vec3(0, 0, 0);
}

Mat4 Mat4::identidade() {
    Mat4 r = {};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 Mat4::translacao(Vec3 t) {
    Mat4 r = identidade();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

Mat4 Mat4::escala(f32 s) {
    Mat4 r = identidade();
    r.m[0] = r.m[5] = r.m[10] = s;
    return r;
}

Mat4 Mat4::rotacao_y(f32 a) {
    Mat4 r = identidade();
    f32 c = cosf(a), s = sinf(a);
    r(0, 0) = c; r(0, 2) = s;
    r(2, 0) = -s; r(2, 2) = c;
    return r;
}

Mat4 Mat4::rotacao_x(f32 a) {
    Mat4 r = identidade();
    f32 c = cosf(a), s = sinf(a);
    r(1, 1) = c; r(1, 2) = -s;
    r(2, 1) = s; r(2, 2) = c;
    return r;
}

Mat4 Mat4::trs(Vec3 t, const f32 q[4], Vec3 s) {
    f32 x = q[0], y = q[1], z = q[2], w = q[3];
    Mat4 r;
    r.m[0] = (1 - 2 * (y * y + z * z)) * s.x; r.m[1] = 2 * (x * y + z * w) * s.x; r.m[2] = 2 * (x * z - y * w) * s.x; r.m[3] = 0;
    r.m[4] = 2 * (x * y - z * w) * s.y; r.m[5] = (1 - 2 * (x * x + z * z)) * s.y; r.m[6] = 2 * (y * z + x * w) * s.y; r.m[7] = 0;
    r.m[8] = 2 * (x * z + y * w) * s.z; r.m[9] = 2 * (y * z - x * w) * s.z; r.m[10] = (1 - 2 * (x * x + y * y)) * s.z; r.m[11] = 0;
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z; r.m[15] = 1;
    return r;
}

Mat4 Mat4::perspectiva(f32 fovy, f32 aspecto, f32 perto, f32 longe) {
    Mat4 r = {};
    f32 f = 1.0f / tanf(fovy * 0.5f);
    r(0, 0) = f / aspecto;
    r(1, 1) = f;
    r(2, 2) = (longe + perto) / (perto - longe);
    r(2, 3) = 2.0f * longe * perto / (perto - longe);
    r(3, 2) = -1.0f;
    return r;
}

Mat4 Mat4::olhar_para(Vec3 olho, Vec3 alvo, Vec3 cima) {
    Vec3 f = normalizar(alvo - olho);
    Vec3 s = normalizar(vetorial(f, cima));
    Vec3 u = vetorial(s, f);
    Mat4 r = identidade();
    r(0, 0) = s.x; r(0, 1) = s.y; r(0, 2) = s.z;
    r(1, 0) = u.x; r(1, 1) = u.y; r(1, 2) = u.z;
    r(2, 0) = -f.x; r(2, 1) = -f.y; r(2, 2) = -f.z;
    r(0, 3) = -ponto(s, olho);
    r(1, 3) = -ponto(u, olho);
    r(2, 3) = ponto(f, olho);
    return r;
}

Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int c = 0; c < 4; c++)
        for (int l = 0; l < 4; l++)
            r(l, c) = a(l, 0) * b(0, c) + a(l, 1) * b(1, c) + a(l, 2) * b(2, c) + a(l, 3) * b(3, c);
    return r;
}

Vec4 transformar(const Mat4& m, Vec3 p) {
    return {m(0, 0) * p.x + m(0, 1) * p.y + m(0, 2) * p.z + m(0, 3),
            m(1, 0) * p.x + m(1, 1) * p.y + m(1, 2) * p.z + m(1, 3),
            m(2, 0) * p.x + m(2, 1) * p.y + m(2, 2) * p.z + m(2, 3),
            m(3, 0) * p.x + m(3, 1) * p.y + m(3, 2) * p.z + m(3, 3)};
}

Vec3 transformar_direcao(const Mat4& m, Vec3 d) {
    return {m(0, 0) * d.x + m(0, 1) * d.y + m(0, 2) * d.z, m(1, 0) * d.x + m(1, 1) * d.y + m(1, 2) * d.z,
            m(2, 0) * d.x + m(2, 1) * d.y + m(2, 2) * d.z};
}

Mat4 inversa_rigida(const Mat4& m) {
    // M = T · (s·R)  →  M⁻¹ = (R^T / s) · T(−t)
    f32 s2 = m(0, 0) * m(0, 0) + m(1, 0) * m(1, 0) + m(2, 0) * m(2, 0);
    f32 k = s2 > 0 ? 1.0f / s2 : 0.0f;
    Mat4 r = Mat4::identidade();
    for (int l = 0; l < 3; l++)
        for (int c = 0; c < 3; c++) r(l, c) = m(c, l) * k;
    Vec3 t(m(0, 3), m(1, 3), m(2, 3));
    for (int l = 0; l < 3; l++) r(l, 3) = -(r(l, 0) * t.x + r(l, 1) * t.y + r(l, 2) * t.z);
    return r;
}

Frustum Frustum::de(const Mat4& vp) {
    Frustum f;
    for (int i = 0; i < 6; i++) {
        int eixo = i / 2;
        f32 sinal = (i % 2 == 0) ? 1.0f : -1.0f;
        Vec4 p{vp(3, 0) + sinal * vp(eixo, 0), vp(3, 1) + sinal * vp(eixo, 1), vp(3, 2) + sinal * vp(eixo, 2),
               vp(3, 3) + sinal * vp(eixo, 3)};
        f32 n = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
        if (n > 0) { p.x /= n; p.y /= n; p.z /= n; p.w /= n; }
        f.planos[i] = p;
    }
    return f;
}

int Frustum::esfera(Vec3 c, f32 r, bool* cruza_perto) const {
    int res = 1;
    if (cruza_perto) *cruza_perto = false;
    for (int i = 0; i < 6; i++) {
        const Vec4& p = planos[i];
        f32 d = p.x * c.x + p.y * c.y + p.z * c.z + p.w;
        if (d < -r) return 0;
        if (d < r) {
            res = 2;
            if (i == 4 && cruza_perto) *cruza_perto = true;
        }
    }
    return res;
}
