#include "desenho2d.h"

Prim* ListaDesenho2D::nova() {
    if (n_ >= CAPACIDADE) { descartadas_++; return nullptr; }
    return &prims_[n_++];
}

void ListaDesenho2D::retangulo(f32 x, f32 y, f32 l, f32 a, Cor c) {
    if (l <= 0 || a <= 0 || c.a == 0) return;
    Prim* p = nova();
    if (!p) return;
    p->tipo = TipoPrim::RETANGULO;
    p->x[0] = x; p->y[0] = y; p->x[1] = x + l; p->y[1] = y + a;
    p->cor[0] = c;
}

void ListaDesenho2D::gradiente(f32 x, f32 y, f32 l, f32 a, Cor se, Cor sd, Cor ie, Cor id) {
    if (l <= 0 || a <= 0) return;
    Prim* p = nova();
    if (!p) return;
    p->tipo = TipoPrim::GRADIENTE;
    p->x[0] = x; p->y[0] = y; p->x[1] = x + l; p->y[1] = y + a;
    p->cor[0] = se; p->cor[1] = sd; p->cor[2] = ie; p->cor[3] = id;
}

void ListaDesenho2D::quad(const f32 xs[4], const f32 ys[4], Cor c) {
    if (c.a == 0) return;
    Prim* p = nova();
    if (!p) return;
    p->tipo = TipoPrim::QUAD;
    for (int i = 0; i < 4; i++) { p->x[i] = xs[i]; p->y[i] = ys[i]; }
    p->cor[0] = c;
}

void ListaDesenho2D::moldura(f32 x, f32 y, f32 l, f32 a, f32 e, Cor c) {
    retangulo(x, y, l, e, c);
    retangulo(x, y + a - e, l, e, c);
    retangulo(x, y + e, e, a - 2 * e, c);
    retangulo(x + l - e, y + e, e, a - 2 * e, c);
}
