#include "colisao.h"

#include <math.h>
#include <string.h>

f32 Colisor::topo(f32 lx, f32 lz) const {
    if (tipo == ColisaoBMOD::CAIXA) return origem.y + mx.y;
    lx = lx < mn.x ? mn.x : (lx > mx.x ? mx.x : lx);
    lz = lz < mn.z ? mn.z : (lz > mx.z ? mx.z : lz);
    f32 u;
    switch (tipo) {
    case ColisaoBMOD::RAMPA_MAIS_X: u = (lx - mn.x) / (mx.x - mn.x); break;
    case ColisaoBMOD::RAMPA_MENOS_X: u = (mx.x - lx) / (mx.x - mn.x); break;
    case ColisaoBMOD::RAMPA_MAIS_Z: u = (lz - mn.z) / (mx.z - mn.z); break;
    default: u = (mx.z - lz) / (mx.z - mn.z); break;
    }
    f32 baixo = mn.y + espessura;
    return origem.y + baixo + u * (mx.y - baixo);
}

void MundoColisao::limpar(f32 meia_largura) {
    n_ = 0;
    descartados_ = 0;
    meia_ = meia_largura;
    memset(n_celula_, 0, sizeof n_celula_);
}

void MundoColisao::inserir(int i) {
    const Colisor& c = col_[i];
    const f32 tam = 2 * meia_ / GRADE;
    int x0 = static_cast<int>(floorf((c.wmin_x + meia_) / tam)), x1 = static_cast<int>(floorf((c.wmax_x + meia_) / tam));
    int z0 = static_cast<int>(floorf((c.wmin_z + meia_) / tam)), z1 = static_cast<int>(floorf((c.wmax_z + meia_) / tam));
    for (int gz = z0 < 0 ? 0 : z0; gz <= z1 && gz < GRADE; gz++)
        for (int gx = x0 < 0 ? 0 : x0; gx <= x1 && gx < GRADE; gx++) {
            if (n_celula_[gz][gx] >= MAX_POR_CELULA) { descartados_++; continue; }
            celula_[gz][gx][n_celula_[gz][gx]++] = static_cast<u16>(i);
        }
}

bool MundoColisao::adicionar_caixa(Vec3 pos, f32 rot, Vec3 mn, Vec3 mx) {
    if (n_ >= MAX_COLISORES) { descartados_++; return false; }
    Colisor& c = col_[n_];
    f32 a = rot * 3.14159265f / 180.0f;
    c.origem = pos;
    c.c = cosf(a);
    c.s = sinf(a);
    c.mn = mn;
    c.mx = mx;
    c.tipo = ColisaoBMOD::CAIXA;
    c.espessura = 0;
    c.wmin_x = c.wmin_z = 1e30f;
    c.wmax_x = c.wmax_z = -1e30f;
    const f32 xs[2] = {mn.x, mx.x}, zs[2] = {mn.z, mx.z};
    for (f32 lx : xs)
        for (f32 lz : zs) {
            // local → mundo: x = c·lx + s·lz, z = −s·lx + c·lz
            f32 wx = pos.x + c.c * lx + c.s * lz, wz = pos.z - c.s * lx + c.c * lz;
            c.wmin_x = fminf(c.wmin_x, wx); c.wmax_x = fmaxf(c.wmax_x, wx);
            c.wmin_z = fminf(c.wmin_z, wz); c.wmax_z = fmaxf(c.wmax_z, wz);
        }
    inserir(n_);
    marca_[n_] = 0;
    n_++;
    return true;
}

int MundoColisao::adicionar_modelo(const ModeloBrisa& m, Vec3 pos, f32 rot) {
    int k = 0;
    for (int i = 0; i < m.n_colisoes(); i++) {
        const ColisaoBMOD& b = m.colisao(i);
        if (!adicionar_caixa(pos, rot, Vec3(b.min[0], b.min[1], b.min[2]), Vec3(b.max[0], b.max[1], b.max[2]))) break;
        col_[n_ - 1].tipo = b.tipo;
        col_[n_ - 1].espessura = b.espessura;
        k++;
    }
    return k;
}

template <typename F> void MundoColisao::visitar(f32 x0, f32 z0, f32 x1, f32 z1, F f) const {
    const f32 tam = 2 * meia_ / GRADE;
    int gx0 = static_cast<int>(floorf((x0 + meia_) / tam)), gx1 = static_cast<int>(floorf((x1 + meia_) / tam));
    int gz0 = static_cast<int>(floorf((z0 + meia_) / tam)), gz1 = static_cast<int>(floorf((z1 + meia_) / tam));
    if (gx0 < 0) gx0 = 0;
    if (gz0 < 0) gz0 = 0;
    if (gx1 >= GRADE) gx1 = GRADE - 1;
    if (gz1 >= GRADE) gz1 = GRADE - 1;
    visita_++;
    for (int gz = gz0; gz <= gz1; gz++)
        for (int gx = gx0; gx <= gx1; gx++)
            for (int k = 0; k < n_celula_[gz][gx]; k++) {
                int i = celula_[gz][gx][k];
                if (marca_[i] == visita_) continue;
                marca_[i] = visita_;
                f(i, col_[i]);
            }
}

// Distância (no plano xz local) de um ponto ao retângulo do colisor; preenche o ponto mais perto.
static f32 dist_ret(const Colisor& c, f32 lx, f32 lz, f32* qx, f32* qz) {
    *qx = lx < c.mn.x ? c.mn.x : (lx > c.mx.x ? c.mx.x : lx);
    *qz = lz < c.mn.z ? c.mn.z : (lz > c.mx.z ? c.mx.z : lz);
    f32 dx = lx - *qx, dz = lz - *qz;
    return sqrtf(dx * dx + dz * dz);
}

bool MundoColisao::chao(f32 x, f32 z, f32 raio, f32 limite, f32* y, int* id) const {
    bool tem = false;
    f32 melhor = -1e30f;
    int qual = -1;
    if (tem_chao_ && chao_y_ <= limite) { melhor = chao_y_; tem = true; }
    visitar(x - raio, z - raio, x + raio, z + raio, [&](int i, const Colisor& c) {
        f32 lx, lz, qx, qz;
        c.local(x, z, &lx, &lz);
        if (dist_ret(c, lx, lz, &qx, &qz) > raio) return;
        f32 t = c.topo(qx, qz);
        if (c.tipo != ColisaoBMOD::CAIXA) t = c.topo(lx, lz);  // rampa: altura sob o centro
        if (t <= limite && t > melhor) { melhor = t; tem = true; qual = i; }
    });
    if (tem) *y = melhor;
    if (id) *id = qual;
    return tem;
}

bool MundoColisao::empurrar(Vec3& p, f32 altura, f32 raio, f32 passo) const {
    bool mexeu = false;
    for (int volta = 0; volta < 3; volta++) {
        bool nesta = false;
        visitar(p.x - raio, p.z - raio, p.x + raio, p.z + raio, [&](int, const Colisor& c) {
            f32 lx, lz, qx, qz;
            c.local(p.x, p.z, &lx, &lz);
            f32 d = dist_ret(c, lx, lz, &qx, &qz);
            if (d >= raio) return;
            // Altura do obstáculo no ponto de contato: abaixo do passo, é chão (degrau, rampa).
            f32 topo = c.tipo == ColisaoBMOD::CAIXA ? c.topo(qx, qz) : c.topo(lx, lz);
            f32 base = c.tipo == ColisaoBMOD::CAIXA ? c.base(qx, qz) : c.base(lx, lz);
            if (topo <= p.y + passo || base >= p.y + altura) return;
            f32 ex, ez, pen;
            if (d > 1e-5f) {
                ex = (lx - qx) / d;
                ez = (lz - qz) / d;
                pen = raio - d;
            } else {
                // Centro dentro do retângulo: sai pela borda mais perto.
                f32 dxn = lx - c.mn.x, dxp = c.mx.x - lx, dzn = lz - c.mn.z, dzp = c.mx.z - lz;
                f32 m = fminf(fminf(dxn, dxp), fminf(dzn, dzp));
                ex = m == dxn ? -1.0f : (m == dxp ? 1.0f : 0.0f);
                ez = (ex == 0) ? (m == dzn ? -1.0f : 1.0f) : 0.0f;
                pen = m + raio;
            }
            // local → mundo
            f32 wx = c.c * ex + c.s * ez, wz = -c.s * ex + c.c * ez;
            p.x += wx * (pen + 1e-4f);
            p.z += wz * (pen + 1e-4f);
            nesta = true;
        });
        if (!nesta) break;
        mexeu = true;
    }
    return mexeu;
}

bool MundoColisao::teto(f32 x, f32 z, f32 raio, f32 cabeca, f32 ate, f32* y) const {
    bool tem = false;
    f32 melhor = 1e30f;
    visitar(x - raio, z - raio, x + raio, z + raio, [&](int, const Colisor& c) {
        f32 lx, lz, qx, qz;
        c.local(x, z, &lx, &lz);
        if (dist_ret(c, lx, lz, &qx, &qz) > raio * 0.8f) return;
        f32 b = c.base(lx, lz);
        if (b >= cabeca - 0.01f && b <= ate && b < melhor) { melhor = b; tem = true; }
    });
    if (tem) *y = melhor;
    return tem;
}

bool MundoColisao::livre(Vec3 p, f32 altura, f32 raio) const {
    bool ok = true;
    visitar(p.x - raio, p.z - raio, p.x + raio, p.z + raio, [&](int, const Colisor& c) {
        f32 lx, lz, qx, qz;
        c.local(p.x, p.z, &lx, &lz);
        if (dist_ret(c, lx, lz, &qx, &qz) >= raio) return;
        f32 t = c.topo(lx, lz), b = c.base(lx, lz);
        if (t > p.y + 0.05f && b < p.y + altura) ok = false;
    });
    return ok;
}

ImpactoRaio MundoColisao::raio(Vec3 a, Vec3 b) const {
    ImpactoRaio r;
    Vec3 d = b - a;
    if (tem_chao_ && a.y > chao_y_ && b.y < chao_y_) {
        r.t = (a.y - chao_y_) / (a.y - b.y);
    }
    visitar(fminf(a.x, b.x), fminf(a.z, b.z), fmaxf(a.x, b.x), fmaxf(a.z, b.z), [&](int i, const Colisor& c) {
        // Raio no espaço local (slab) contra a caixa envolvente; nas rampas, confere a laje.
        f32 ax, az, bx, bz;
        c.local(a.x, a.z, &ax, &az);
        c.local(b.x, b.z, &bx, &bz);
        f32 o[3] = {ax, a.y - c.origem.y, az}, v[3] = {bx - ax, d.y, bz - az};
        f32 mn[3] = {c.mn.x, c.mn.y, c.mn.z}, mx[3] = {c.mx.x, c.mx.y, c.mx.z};
        f32 t0 = 0, t1 = r.t;
        for (int k = 0; k < 3; k++) {
            if (fabsf(v[k]) < 1e-9f) {
                if (o[k] < mn[k] || o[k] > mx[k]) return;
                continue;
            }
            f32 ta = (mn[k] - o[k]) / v[k], tb = (mx[k] - o[k]) / v[k];
            if (ta > tb) { f32 x = ta; ta = tb; tb = x; }
            t0 = fmaxf(t0, ta);
            t1 = fminf(t1, tb);
            if (t0 > t1) return;
        }
        if (c.tipo != ColisaoBMOD::CAIXA) {
            // Procura o primeiro ponto dentro da laje, em 12 amostras entre t0 e t1.
            bool achou = false;
            for (int s = 0; s <= 12 && !achou; s++) {
                f32 t = t0 + (t1 - t0) * s / 12.0f;
                f32 lx = o[0] + v[0] * t, ly = o[1] + v[1] * t + c.origem.y, lz = o[2] + v[2] * t;
                if (ly <= c.topo(lx, lz) && ly >= c.base(lx, lz)) { t0 = t; achou = true; }
            }
            if (!achou) return;
        }
        if (t0 < r.t) { r.t = t0; r.colisor = i; }
    });
    return r;
}
