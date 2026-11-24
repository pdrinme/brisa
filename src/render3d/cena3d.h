// Descrição de um quadro 3D: câmera, luz, neblina e objetos (malha + matriz).
// Igual para os dois caminhos de renderização (EE e VU1) e para a prévia no PC.
#pragma once
#include "malha.h"

struct Camera3D {
    Mat4 vista, projecao, vp;  // vp = projecao · vista
    Vec3 posicao;
    f32 perto = 0.5f, longe = 400.0f;
    void configurar(Vec3 olho, Vec3 alvo, f32 fovy, f32 aspecto, f32 perto_, f32 longe_);
};

// Luz "toon" do Ventania em 2 faixas (toon_paleta.gdshader): a cor da face é a
// cor da paleta multiplicada por `sombra` (azulada) ou `luz`, com uma transição
// curta em torno de `limiar` (N·L) de largura `suavidade`.
struct LuzToon {
    Vec3 direcao = Vec3(-0.45f, 0.8f, 0.4f);  // para onde está o sol (normalizada ao configurar)
    Vec3 luz = Vec3(1.0f, 0.98f, 0.94f);
    Vec3 sombra = Vec3(0.58f, 0.62f, 0.86f);
    f32 limiar = 0.12f;
    f32 suavidade = 0.10f;
};

struct Neblina {
    f32 inicio = 60.0f, fim = 180.0f;  // distância (w) em metros
    Cor cor = {0xcf, 0xe8, 0xf6, 255};
};

struct Objeto3D {
    const Malha* malha;
    Mat4 modelo;
};

// Sombra "blob": um quadrado com a textura de sombra deitado no chão.
struct Sombra3D {
    Vec3 centro;  // no chão
    f32 raio;
};

// Parede da Tormenta (storm_visual.gd): cilindro aberto translúcido no círculo seguro.
struct Parede3D {
    bool ativa = false;
    f32 cx = 0, cz = 0, raio = 0, base = -20.0f, altura = 220.0f;
    f32 tempo = 0;                   // s: as faixas correm (o TIME do shader)
    Cor cor = {140, 64, 242, 96};    // roxo do Ventania (0,55; 0,25; 0,95), alfa 0,42 × ~0,9 das faixas
};

struct Cena3D {
    static const int MAX_OBJETOS = 4096;
    static const int MAX_SOMBRAS = 64;
    Sombra3D sombras[MAX_SOMBRAS];
    int n_sombras = 0;
    int textura_sombra = -1;  // id registrado na plataforma
    Parede3D parede;           // Etapa 8: desenhada por último, com alfa
    Camera3D camera;
    LuzToon luz;
    Neblina neblina;
    Cor ceu = {0xcf, 0xe8, 0xf6, 255};
    Objeto3D objetos[MAX_OBJETOS];
    int n = 0;
    void limpar() { n = 0; n_sombras = 0; }
    void sombra(Vec3 c, f32 r) {
        if (n_sombras < MAX_SOMBRAS) sombras[n_sombras++] = {c, r};
    }
    bool adicionar(const Malha* m, const Mat4& modelo) {
        if (n >= MAX_OBJETOS || !m || m->n == 0) return false;
        objetos[n].malha = m;
        objetos[n].modelo = modelo;
        n++;
        return true;
    }
};

// Fator de luz em 0..1 a partir de N·L (mesma fórmula no EE, no VU1 e no PC):
// s = clamp(N·L · a + b), com a = 1/suavidade e b = 0,5 − limiar/suavidade.
struct CoeficientesToon {
    f32 a, b;
    static CoeficientesToon de(const LuzToon& l) {
        f32 a = 1.0f / (l.suavidade > 0.001f ? l.suavidade : 0.001f);
        return {a, 0.5f - l.limiar * a};
    }
};

inline Cor sombrear_toon(Cor base, f32 ndotl, const LuzToon& l, const CoeficientesToon& k) {
    f32 s = ndotl * k.a + k.b;
    s = s < 0 ? 0 : (s > 1 ? 1 : s);
    f32 fr = l.sombra.x + (l.luz.x - l.sombra.x) * s;
    f32 fg = l.sombra.y + (l.luz.y - l.sombra.y) * s;
    f32 fb = l.sombra.z + (l.luz.z - l.sombra.z) * s;
    auto canal = [](u8 c, f32 f) -> u8 {
        f32 v = c * f;
        return static_cast<u8>(v > 255.0f ? 255 : static_cast<int>(v));  // trunca, como o ftoi0 do VU
    };
    return Cor{canal(base.r, fr), canal(base.g, fg), canal(base.b, fb), 255};
}

// Estatísticas de um quadro 3D.
struct Estatisticas3D {
    u32 objetos = 0, objetos_visiveis = 0, objetos_cortados_perto = 0;
    u32 triangulos_enviados = 0;   // que entraram no transformador (EE ou VU1)
    u32 triangulos_desenhados = 0; // depois de recorte e faces traseiras (só no caminho do EE)
    u32 lotes_vu1 = 0;
    u32 tempo_3d_us = 0;           // montar + enviar + esperar o 3D terminar
};
