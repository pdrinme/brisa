// Rasterizador de software da lista 2D (só no PC): prévia em PNG das telas e
// testes. Imita o GS no essencial: sprites, quads em tira, Gouraud e alfa.
#pragma once
#include "nucleo/desenho2d.h"
#include "render3d/pipeline_sw.h"

class Rasterizador : public SaidaTriangulos {
public:
    static const int L = 640, A = 448;
    // 2D por cima do que já está na imagem (limpar = pinta o fundo da lista antes).
    void desenhar(const ListaDesenho2D& lista, bool limpar = true);
    // 3D: limpa cor e Z, depois cada triângulo com teste Z "maior ou igual" e
    // neblina por vértice, como o GS (TEST.ZTST = GEQUAL, PRIM.FGE = 1).
    void iniciar_3d(Cor fundo, Cor neblina, f32 perto);
    void triangulo(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c) override;
    u32 z(int x, int y) const { return z_[y * L + x]; }
    u32 pixel(int x, int y) const { return px_[y * L + x]; }  // 0xRRGGBB
    bool salvar_png(const char* caminho) const;
private:
    void misturar(int x, int y, f32 r, f32 g, f32 b, f32 a);
    void triangulo(const f32* xs, const f32* ys, const Cor* cs);
    u32 px_[L * A] = {};
    u32 z_[L * A] = {};
    Cor neblina_ = {0, 0, 0, 255};
    f32 perto_ = 0.5f;
};
