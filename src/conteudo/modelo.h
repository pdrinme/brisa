// Modelo do Brisa (BMOD): nós, partes rígidas, marcadores e colisões. As malhas
// de cada parte apontam direto para os dados do pacote (sem cópia), no formato
// que o VU1 lê. Uma parte segue um nó: posição = mundo(nó) · inv_bind(nó) · p.
#pragma once
#include "nucleo/mat.h"
#include "render3d/malha.h"

struct NoBMOD {
    char nome[24];
    i32 pai;
    u32 flags;  // bit 0: animado, bit 1: osso
    f32 t[3], _a;
    f32 r[4];
    f32 s[3], _b;
    f32 inv_bind[16];
};
static_assert(sizeof(NoBMOD) == 144, "NoBMOD");

struct MarcadorBMOD {
    char nome[24];
    i32 no;
    f32 pos[3];
    u32 _[2];
};
static_assert(sizeof(MarcadorBMOD) == 48, "MarcadorBMOD");

// Colisão (v2): caixa, ou rampa (laje inclinada) que sobe na direção do tipo.
// Topo da rampa: de min.y + espessura (ponta baixa) até max.y (ponta alta).
struct ColisaoBMOD {
    enum Tipo : u32 { CAIXA = 0, RAMPA_MAIS_X = 1, RAMPA_MENOS_X = 2, RAMPA_MAIS_Z = 3, RAMPA_MENOS_Z = 4 };
    f32 min[3];
    u32 tipo;
    f32 max[3];
    f32 espessura;
};
static_assert(sizeof(ColisaoBMOD) == 32, "ColisaoBMOD");

class ModeloBrisa {
public:
    static const u32 VERSAO = 3;
    static const int MAX_PARTES = 40;
    static const int MAX_NOS = 32;
    bool carregar(const u8* dados, u32 tamanho, char* erro, int n_erro);
    int n_partes() const { return n_partes_; }
    int n_nos() const { return n_nos_; }
    const Malha& malha(int parte) const { return malhas_[parte]; }
    int no_da_parte(int parte) const { return no_parte_[parte]; }
    const NoBMOD& no(int i) const { return nos_[i]; }
    int achar_no(const char* nome) const;
    int n_marcadores() const { return n_marc_; }
    const MarcadorBMOD& marcador(int i) const { return marc_[i]; }
    int n_colisoes() const { return n_col_; }
    const ColisaoBMOD& colisao(int i) const { return col_[i]; }
    u32 triangulos() const { return n_vertices_ / 3; }
    bool esqueletico() const { return esqueletico_; }
    Vec3 aabb_min() const { return mn_; }
    Vec3 aabb_max() const { return mx_; }
    // CRC32 de "índice:nome" dos ossos e nós animados (confere se uma animação serve).
    u32 assinatura() const { return assinatura_; }
private:
    Malha malhas_[MAX_PARTES];
    int no_parte_[MAX_PARTES];
    const NoBMOD* nos_ = nullptr;
    const MarcadorBMOD* marc_ = nullptr;
    const ColisaoBMOD* col_ = nullptr;
    int n_partes_ = 0, n_nos_ = 0, n_marc_ = 0, n_col_ = 0;
    u32 n_vertices_ = 0, assinatura_ = 0;
    bool esqueletico_ = false;
    Vec3 mn_, mx_;
};

// Pose: a matriz de mundo (espaço do modelo) de cada nó.
struct Pose {
    Mat4 mundo[ModeloBrisa::MAX_NOS];
    int n = 0;
};

// Pose de repouso (os TRS gravados nos nós).
void pose_repouso(const ModeloBrisa& m, Pose& p);
// Matriz de um marcador no espaço do modelo.
Vec3 posicao_marcador(const ModeloBrisa& m, const Pose& p, int marcador);
