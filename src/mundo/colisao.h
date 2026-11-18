// Mundo de colisão do Brisa (portátil): caixas e rampas dos modelos, giradas em
// Y pela instância, numa grade espacial; mais um chão plano opcional. Sem física
// de corpos rígidos (BRISA_GAME_DESIGN §7): só o que o personagem e a câmera
// precisam (chão, paredes, teto, raio).
#pragma once
#include "conteudo/modelo.h"
#include "nucleo/mat.h"

struct Colisor {
    Vec3 origem;      // posição da instância
    f32 c, s;         // cosseno e seno da rotação em Y
    Vec3 mn, mx;      // no espaço do modelo
    u32 tipo;         // ColisaoBMOD::Tipo
    f32 espessura;
    f32 wmin_x, wmin_z, wmax_x, wmax_z;  // caixa envolvente no mundo (xz)
    // Ponto do mundo → espaço do modelo (x, z).
    void local(f32 x, f32 z, f32* lx, f32* lz) const {
        f32 dx = x - origem.x, dz = z - origem.z;
        *lx = c * dx - s * dz;
        *lz = s * dx + c * dz;
    }
    // Topo e base num ponto local (preso ao retângulo).
    f32 topo(f32 lx, f32 lz) const;
    f32 base(f32 lx, f32 lz) const { return tipo == ColisaoBMOD::CAIXA ? origem.y + mn.y : topo(lx, lz) - espessura; }
};

struct ImpactoRaio {
    f32 t = 1.0f;  // fração do segmento (1 = nada no caminho)
    int colisor = -1;
};

class MundoColisao {
public:
    static const int MAX_COLISORES = 2048;
    static const int CELULA = 8;          // metros
    static const int GRADE = 64;          // 64 × 64 células = 512 m de lado
    static const int MAX_POR_CELULA = 48;
    void limpar(f32 meia_largura);
    void definir_chao(bool tem, f32 y = 0) { tem_chao_ = tem; chao_y_ = y; }
    // Adiciona as colisões de um modelo na posição/rotação (graus) dadas.
    int adicionar_modelo(const ModeloBrisa& m, Vec3 pos, f32 rot_graus);
    bool adicionar_caixa(Vec3 pos, f32 rot_graus, Vec3 mn, Vec3 mx);
    int n() const { return n_; }
    const Colisor& colisor(int i) const { return col_[i]; }
    int descartados() const { return descartados_; }

    // Maior topo com apoio sob o círculo (x, z, raio) que não passe de `limite`.
    // Devolve false se não há apoio (cair).
    bool chao(f32 x, f32 z, f32 raio, f32 limite, f32* y, int* colisor = nullptr) const;
    bool eh_rampa(int colisor) const { return colisor >= 0 && col_[colisor].tipo != ColisaoBMOD::CAIXA; }
    // Empurra um cilindro (pés, altura, raio) para fora das paredes. `passo`:
    // obstáculos com topo até pés + passo não bloqueiam (são degrau/rampa).
    // Devolve true se empurrou.
    bool empurrar(Vec3& pes, f32 altura, f32 raio, f32 passo) const;
    // Primeira base acima de `cabeca` que cobre o círculo, até `ate` (teto). Devolve false se não há.
    bool teto(f32 x, f32 z, f32 raio, f32 cabeca, f32 ate, f32* y) const;
    // O cilindro cabe aqui (nada atravessando)? Usado para levantar do agachado.
    bool livre(Vec3 pes, f32 altura, f32 raio) const;
    // Raio (segmento a → b) contra colisores e chão.
    ImpactoRaio raio(Vec3 a, Vec3 b) const;

private:
    template <typename F> void visitar(f32 x0, f32 z0, f32 x1, f32 z1, F f) const;
    void inserir(int i);
    Colisor col_[MAX_COLISORES];
    int n_ = 0, descartados_ = 0;
    u16 celula_[GRADE][GRADE][MAX_POR_CELULA];
    u8 n_celula_[GRADE][GRADE];
    f32 meia_ = 256;
    bool tem_chao_ = true;
    f32 chao_y_ = 0;
    mutable u32 marca_[MAX_COLISORES];
    mutable u32 visita_ = 0;
};
