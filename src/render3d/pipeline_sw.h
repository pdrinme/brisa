// Pipeline 3D de referência, em C++ portátil: transforma, ilumina, recorta no
// plano perto, descarta faces traseiras e projeta para a tela. É o "caminho A"
// no PS2 (o EE faz tudo e o GS só rasteriza) e a referência dos testes e da
// prévia no PC. O caminho B (VU1) faz a mesma conta em microcódigo.
#pragma once
#include "cena3d.h"

struct VerticeTela {
    f32 x, y;   // pixels da tela lógica (640×448), origem no canto superior esquerdo
    f32 q;      // 1/w (para o Z do GS)
    f32 neblina;  // 0..255 (255 = sem neblina), como o registrador F do GS
    Cor cor;
    f32 s = 0, t = 0;  // coordenadas de textura (0..1), só nas sombras
};

class SaidaTriangulos {
public:
    virtual ~SaidaTriangulos() = default;
    virtual void triangulo(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c) = 0;
    // Triângulo com textura (s, t) e alfa, sem neblina (sombras blob).
    virtual void triangulo_textura(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c, int textura) {
        (void)a; (void)b; (void)c; (void)textura;
    }
};

struct ConfigTela {
    f32 largura = 640, altura = 448;
};

// Valores do GS usados pelos dois caminhos e pelo rasterizador do PC.
namespace gs3d {
const f32 Z_MAX = 16777000.0f;  // Z de 24 bits
inline u32 z_de(f32 q, f32 perto) {
    f32 z = q * perto * Z_MAX;
    return z <= 0 ? 0 : (z >= Z_MAX ? static_cast<u32>(Z_MAX) : static_cast<u32>(z));
}
inline f32 neblina_de(f32 w, const Neblina& n) {
    f32 k = 255.0f / (n.fim - n.inicio > 0.01f ? n.fim - n.inicio : 0.01f);
    f32 f = (n.fim - w) * k;
    return f < 0 ? 0 : (f > 255 ? 255 : f);
}
}  // namespace gs3d

// Desenha um objeto (ou todos). Devolve os triângulos que chegaram à saída.
u32 pipeline_objeto(const Cena3D& cena, const Objeto3D& obj, const ConfigTela& tela, SaidaTriangulos& saida);
void pipeline_cena(const Cena3D& cena, const ConfigTela& tela, SaidaTriangulos& saida, Estatisticas3D& est);

// Sombras blob: cada uma vira 2 triângulos com s,t. Pula as que cruzam o plano
// perto. Devolve quantos triângulos saíram.
u32 projetar_sombras(const Cena3D& cena, const ConfigTela& tela, SaidaTriangulos& saida);
