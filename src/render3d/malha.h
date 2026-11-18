// Malha estática em lista de triângulos, guardada em arrays separados (SoA) no
// formato que o VU1 recebe sem conversão (BRISA_ARCHITECTURE §5):
//   pos:    4×i16 por vértice (x, y, z, 0)/escala → UNPACK V4-16 com sinal
//           (o VU1 converte com ITOF0 e a escala entra na matriz)
//   normal: 4×i8  por vértice (nx, ny, nz, 0)·127 → UNPACK V4-8 com sinal
//   cor:    4×u8  por vértice (r, g, b, 255)      → UNPACK V4-8 sem sinal
// Os arrays têm 4 vértices de folga zerados: o VIF lê de 16 em 16 bytes.
// Na Etapa 3 o conteúdo vem do conversor (BMD); aqui, de geradores procedurais.
#pragma once
#include "nucleo/arena.h"
#include "nucleo/desenho2d.h"
#include "nucleo/mat.h"

struct Malha {
    u32 n = 0;            // vértices (múltiplo de 3)
    i16* pos = nullptr;   // 4 por vértice, em unidades de `escala` metros
    f32 escala = 1.0f;
    u32* normal = nullptr;
    u32* cor = nullptr;
    Vec3 centro;          // esfera envolvente (espaço do objeto)
    f32 raio = 0;
    u32 triangulos() const { return n / 3; }
    Vec3 posicao(u32 i) const { return Vec3(pos[i * 4] * escala, pos[i * 4 + 1] * escala, pos[i * 4 + 2] * escala); }
};

// Posição → int16 na escala dada (devolve false se não couber).
inline bool quantizar_posicao(Vec3 p, f32 escala, i16* saida) {
    f32 inv = 1.0f / escala;
    f32 v[3] = {p.x * inv, p.y * inv, p.z * inv};
    bool ok = true;
    for (int k = 0; k < 3; k++) {
        f32 a = v[k] + (v[k] >= 0 ? 0.5f : -0.5f);
        if (a > 32767.0f) { a = 32767.0f; ok = false; }
        if (a < -32767.0f) { a = -32767.0f; ok = false; }
        saida[k] = static_cast<i16>(a);
    }
    saida[3] = 0;
    return ok;
}

inline u32 empacotar_normal(Vec3 n) {
    auto q = [](f32 v) -> u32 {
        int i = static_cast<int>(v * 127.0f + (v >= 0 ? 0.5f : -0.5f));
        if (i > 127) i = 127;
        if (i < -127) i = -127;
        return static_cast<u32>(static_cast<u8>(static_cast<i8>(i)));
    };
    return q(n.x) | (q(n.y) << 8) | (q(n.z) << 16);
}
inline Vec3 desempacotar_normal(u32 v) {
    return Vec3(static_cast<i8>(v & 255) / 127.0f, static_cast<i8>((v >> 8) & 255) / 127.0f,
                static_cast<i8>((v >> 16) & 255) / 127.0f);
}
inline u32 empacotar_cor(Cor c) { return c.r | (c.g << 8) | (c.b << 16) | (255u << 24); }
inline Cor desempacotar_cor(u32 v) {
    return Cor{static_cast<u8>(v), static_cast<u8>(v >> 8), static_cast<u8>(v >> 16), 255};
}

// Constrói uma malha triângulo a triângulo (normal plana por face, sombreamento
// "low-poly" como o kit do Ventania). Faces em sentido anti-horário vistas de fora.
class ConstrutorMalha {
public:
    // `extensao`: maior |coordenada| esperada (metros); define a escala das posições int16.
    ConstrutorMalha(Arena& arena, u32 max_triangulos, f32 extensao = 64.0f);
    bool ok() const { return pos_ != nullptr; }
    void triangulo(Vec3 a, Vec3 b, Vec3 c, Cor cor);
    void quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Cor cor);  // a,b,c,d anti-horário
    // Caixa alinhada aos eixos (mínimo, máximo), opcionalmente sem a face de baixo.
    void caixa(Vec3 mn, Vec3 mx, Cor cor, bool com_base = false);
    // Prisma de n lados em pé (cilindro low-poly) e cone.
    void cilindro(Vec3 base, f32 raio, f32 altura, int lados, Cor cor);
    void cone(Vec3 base, f32 raio, f32 altura, int lados, Cor cor);
    // Pirâmide de base quadrada (telhado do kit).
    void piramide(Vec3 mn, Vec3 mx, f32 altura, Cor cor);
    // Esfera geodésica (icosaedro subdividido), usada na copa e na carga de teste.
    void esfera(Vec3 centro, f32 raio, int subdivisoes, Cor cor, f32 achatar_y = 1.0f);
    // Grade de chão em xz com alturas de f(x, z) e cor alternada.
    void grade(f32 x0, f32 z0, f32 tamanho, int n, Cor c1, Cor c2, f32 (*altura)(f32, f32));
    Malha terminar();
    u32 triangulos() const { return n_ / 3; }
    bool estourou() const { return estourou_; }
private:
    void vertice(Vec3 p, u32 normal, u32 cor);
    i16* pos_ = nullptr;
    f32 escala_ = 1.0f;
    u32* nrm_ = nullptr;
    u32* cor_ = nullptr;
    u32 n_ = 0, max_ = 0;
    bool estourou_ = false;
};
