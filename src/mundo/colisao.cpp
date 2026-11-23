#include "colisao.h"

#include <math.h>
#include <string.h>

f32 Relevo::altura(f32 x, f32 z) const {
    f32 fx = (x - origem) / passo, fz = (z - origem) / passo;
    int i = static_cast<int>(floorf(fx)), j = static_cast<int>(floorf(fz));
    f32 tx = fx - i, tz = fz - j;
    f32 a = amostra(i, j) + (amostra(i + 1, j) - amostra(i, j)) * tx;
    f32 b = amostra(i, j + 1) + (amostra(i + 1, j + 1) - amostra(i, j + 1)) * tx;
    return a + (b - a) * tz;
}

f32 Colisor::topo(f32 lx, f32 lz) const {
    if (tipo == ColisaoBMOD::CAIXA) return origem.y + mx.y;
    lx = lx < mn.x ? mn.x : (lx > mx.x ? mx.x : lx);
    lz = lz < mn.z ? mn.z : (lz > mx.z ? mx.z : lz);
    f32 u;
    switch (tipo > ColisaoBMOD::RAMPA_MENOS_Z ? tipo - 4 : tipo) {  // a cunha tem o mesmo topo da rampa
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
    relevo_ = nullptr;
    sujo_ = true;
}

void MundoColisao::celulas(const Colisor& c, int* x0, int* z0, int* x1, int* z1) const {
    const f32 tam = 2 * meia_ / GRADE;
    *x0 = static_cast<int>(floorf((c.wmin_x + meia_) / tam));
    *x1 = static_cast<int>(floorf((c.wmax_x + meia_) / tam));
    *z0 = static_cast<int>(floorf((c.wmin_z + meia_) / tam));
    *z1 = static_cast<int>(floorf((c.wmax_z + meia_) / tam));
    if (*x0 < 0) *x0 = 0;
    if (*z0 < 0) *z0 = 0;
    if (*x1 >= GRADE) *x1 = GRADE - 1;
    if (*z1 >= GRADE) *z1 = GRADE - 1;
}

// Grade compacta em duas passadas: conta por célula, depois preenche. O mundo
// estático (mapa, campo de treino) monta uma vez, na primeira consulta.
void MundoColisao::montar_grade() const {
    static u16 conta[GRADE * GRADE];
    memset(conta, 0, sizeof conta);
    int total = 0;
    for (int i = 0; i < n_; i++) {
        int x0, z0, x1, z1;
        celulas(col_[i], &x0, &z0, &x1, &z1);
        for (int gz = z0; gz <= z1; gz++)
            for (int gx = x0; gx <= x1; gx++) { conta[gz * GRADE + gx]++; total++; }
    }
    int acum = 0;
    for (int k = 0; k < GRADE * GRADE; k++) {
        inicio_[k] = static_cast<u16>(acum < MAX_REFS ? acum : MAX_REFS);
        acum += conta[k];
    }
    inicio_[GRADE * GRADE] = static_cast<u16>(acum < MAX_REFS ? acum : MAX_REFS);
    memset(conta, 0, sizeof conta);
    int perdidos = 0;
    for (int i = 0; i < n_; i++) {
        int x0, z0, x1, z1;
        celulas(col_[i], &x0, &z0, &x1, &z1);
        for (int gz = z0; gz <= z1; gz++)
            for (int gx = x0; gx <= x1; gx++) {
                int k = gz * GRADE + gx, pos = inicio_[k] + conta[k];
                if (pos >= inicio_[k + 1]) { perdidos++; continue; }
                refs_[pos] = static_cast<u16>(i);
                conta[k]++;
            }
    }
    n_refs_ = total;
    // Referências além de MAX_REFS: o colisor fica invisível em parte da grade.
    if (perdidos) const_cast<MundoColisao*>(this)->descartados_ += perdidos;
    sujo_ = false;
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
    marca_[n_] = 0;
    n_++;
    sujo_ = true;
    return true;
}

int MundoColisao::adicionar_modelo(const ModeloBrisa& m, Vec3 pos, f32 rot, f32 e) {
    int k = 0;
    for (int i = 0; i < m.n_colisoes(); i++) {
        const ColisaoBMOD& b = m.colisao(i);
        if (!adicionar_caixa(pos, rot, Vec3(b.min[0], b.min[1], b.min[2]) * e, Vec3(b.max[0], b.max[1], b.max[2]) * e)) break;
        col_[n_ - 1].tipo = b.tipo;
        col_[n_ - 1].espessura = b.espessura * e;
        k++;
    }
    return k;
}

template <typename F> void MundoColisao::visitar(f32 x0, f32 z0, f32 x1, f32 z1, F f) const {
    if (sujo_) montar_grade();
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
            for (int k = inicio_[gz * GRADE + gx]; k < inicio_[gz * GRADE + gx + 1]; k++) {
                int i = refs_[k];
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
    if (relevo_) { melhor = relevo_->altura(x, z); tem = true; }  // o relevo é sempre chão (mesmo acima do limite)
    else if (tem_chao_ && chao_y_ <= limite) { melhor = chao_y_; tem = true; }
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
    if (relevo_) {
        // Marcha de ~1 m no relevo e bisseção no primeiro ponto abaixo do chão.
        f32 comp = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
        int n = static_cast<int>(comp) + 1;
        if (n > 64) n = 64;
        f32 ant = 0;
        for (int s = 1; s <= n; s++) {
            f32 t = static_cast<f32>(s) / n;
            Vec3 q = a + d * t;
            if (q.y < relevo_->altura(q.x, q.z)) {
                f32 lo = ant, hi = t;
                for (int k = 0; k < 8; k++) {
                    f32 m = (lo + hi) * 0.5f;
                    Vec3 w = a + d * m;
                    if (w.y < relevo_->altura(w.x, w.z)) hi = m; else lo = m;
                }
                r.t = lo;
                break;
            }
            ant = t;
        }
    } else if (tem_chao_ && a.y > chao_y_ && b.y < chao_y_) {
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
