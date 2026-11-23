// Mundo de colisão do Brisa (portátil): caixas e rampas dos modelos, giradas em
// Y (e escaladas) pela instância, numa grade espacial compacta; mais um chão
// plano opcional ou o relevo do mapa (altura bilinear, como o ground_height do
// Ventania). Sem física
// de corpos rígidos (BRISA_GAME_DESIGN §7): só o que o personagem e a câmera
// precisam (chão, paredes, teto, raio).
#pragma once
#include "conteudo/modelo.h"
#include "nucleo/mat.h"

// Relevo em grade regular (alturas em centímetros, i16), lido do mapa sem cópia.
struct Relevo {
    const i16* alturas_cm = nullptr;
    int n = 0;          // amostras por lado
    f32 passo = 3.0f;   // metros
    f32 origem = -300;  // x e z da amostra (0, 0)
    f32 amostra(int i, int j) const {
        i = i < 0 ? 0 : (i >= n ? n - 1 : i);
        j = j < 0 ? 0 : (j >= n ? n - 1 : j);
        return alturas_cm[j * n + i] * 0.01f;
    }
    f32 altura(f32 x, f32 z) const;  // bilinear
};

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
    // Caixa e cunha: base plana; rampa (laje): espessura abaixo do topo.
    f32 base(f32 lx, f32 lz) const {
        return (tipo == ColisaoBMOD::CAIXA || tipo > ColisaoBMOD::RAMPA_MENOS_Z) ? origem.y + mn.y : topo(lx, lz) - espessura;
    }
};

struct ImpactoRaio {
    f32 t = 1.0f;  // fração do segmento (1 = nada no caminho)
    int colisor = -1;
};

class MundoColisao {
public:
    static const int MAX_COLISORES = 4096;
    static const int GRADE = 80;          // 80 × 80 células sobre 2 × meia_largura
    static const int MAX_REFS = 16384;    // referências colisor→célula (grade compacta)
    void limpar(f32 meia_largura);
    void definir_chao(bool tem, f32 y = 0) { tem_chao_ = tem; chao_y_ = y; }
    // Relevo (substitui o chão plano); nullptr tira.
    void definir_relevo(const Relevo* r) { relevo_ = r; }
    const Relevo* relevo() const { return relevo_; }
    // Adiciona as colisões de um modelo na posição/rotação (graus)/escala dadas.
    int adicionar_modelo(const ModeloBrisa& m, Vec3 pos, f32 rot_graus, f32 escala = 1.0f);
    bool adicionar_caixa(Vec3 pos, f32 rot_graus, Vec3 mn, Vec3 mx);
    int referencias() const { return n_refs_; }
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
    void montar_grade() const;  // grade compacta (CSR), refeita quando algo foi adicionado
    void celulas(const Colisor& c, int* x0, int* z0, int* x1, int* z1) const;
    Colisor col_[MAX_COLISORES];
    int n_ = 0, descartados_ = 0;
    mutable u16 refs_[MAX_REFS];
    mutable u16 inicio_[GRADE * GRADE + 1];  // refs da célula k: [inicio_[k], inicio_[k+1])
    mutable int n_refs_ = 0;
    mutable bool sujo_ = true;
    f32 meia_ = 256;
    bool tem_chao_ = true;
    f32 chao_y_ = 0;
    const Relevo* relevo_ = nullptr;
    mutable u32 marca_[MAX_COLISORES];
    mutable u32 visita_ = 0;
};
