// Lista de primitivas 2D, independente da plataforma. A tela monta a lista; a
// plataforma (gsKit no PS2, rasterizador de software no PC) a desenha.
// Coordenadas em pixels da tela lógica (640×448), origem no canto superior esquerdo.
#pragma once
#include "tipos.h"

struct Cor {
    u8 r, g, b, a;  // a: 0 = transparente, 255 = opaco
};

constexpr Cor rgb(u32 hex, u8 a = 255) {
    return Cor{static_cast<u8>(hex >> 16), static_cast<u8>(hex >> 8), static_cast<u8>(hex), a};
}

enum class TipoPrim : u8 {
    RETANGULO,   // x0,y0 (canto) e x1,y1 (canto oposto), cor[0]
    GRADIENTE,   // retângulo com cor por canto: [0]=sup-esq, [1]=sup-dir, [2]=inf-esq, [3]=inf-dir
    QUAD,        // 4 vértices em ordem de tira (0,1,2 e 1,2,3), cor[0]
    SPRITE_TEX,  // retângulo texturizado (textura registrada na plataforma), cor[0] modula (128 = 1,0);
                 // x[2], x[3] = u0, u1 e y[2], y[3] = v0, v1 (0..1 da textura)
};

struct Prim {
    TipoPrim tipo;
    i16 textura;
    f32 x[4], y[4];
    Cor cor[4];
};

class ListaDesenho2D {
public:
    static const int CAPACIDADE = 6144;
    void limpar(Cor fundo) { n_ = 0; descartadas_ = 0; fundo_ = fundo; }
    void retangulo(f32 x, f32 y, f32 l, f32 a, Cor c);
    void gradiente(f32 x, f32 y, f32 l, f32 a, Cor se, Cor sd, Cor ie, Cor id);
    void quad(const f32 xs[4], const f32 ys[4], Cor c);
    // Contorno de retângulo com espessura e (para dentro).
    void moldura(f32 x, f32 y, f32 l, f32 a, f32 e, Cor c);
    // Textura inteira esticada no retângulo. modulacao 128,128,128,128 = cor original.
    void sprite(f32 x, f32 y, f32 l, f32 a, int textura, Cor modulacao = Cor{128, 128, 128, 128});
    // Só o pedaço (u0, v0)–(u1, v1) da textura, em 0..1 (minimapa).
    void sprite_uv(f32 x, f32 y, f32 l, f32 a, int textura, f32 u0, f32 v0, f32 u1, f32 v1,
                   Cor modulacao = Cor{128, 128, 128, 128});
    int tamanho() const { return n_; }
    int descartadas() const { return descartadas_; }
    const Prim& operator[](int i) const { return prims_[i]; }
    Cor fundo() const { return fundo_; }
private:
    Prim* nova();
    Prim prims_[CAPACIDADE];
    int n_ = 0;
    int descartadas_ = 0;
    Cor fundo_ = {0, 0, 0, 255};
};
