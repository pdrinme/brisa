// Rasterizador de software da lista 2D (só no PC): prévia em PNG das telas e
// testes. Imita o GS no essencial: sprites, quads em tira, Gouraud e alfa.
#pragma once
#include "nucleo/desenho2d.h"

class Rasterizador {
public:
    static const int L = 640, A = 448;
    void desenhar(const ListaDesenho2D& lista);
    u32 pixel(int x, int y) const { return px_[y * L + x]; }  // 0xRRGGBB
    bool salvar_png(const char* caminho) const;
private:
    void misturar(int x, int y, f32 r, f32 g, f32 b, f32 a);
    void triangulo(const f32* xs, const f32* ys, const Cor* cs);
    u32 px_[L * A] = {};
};
