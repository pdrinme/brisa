// Navegação dos bots (nav_grade.gd do Ventania): o grafo vem pronto do conversor
// (mapa/ilha.bnav: 150 × 150 células de 4 m do chão, andares e ligações de escadas
// e rampas) e o A* roda aqui, com o mesmo custo e heurística (×1,3) e o mesmo
// limite de 7.000 expansões. O caminho sai com pontos "precisos" (porta, escada,
// rampa: chegar perto antes de seguir).
#pragma once
#include "nucleo/mat.h"

class Navegacao {
public:
    static const u32 VERSAO = 1;
    static const int N = 150;
    static const int MAX_EXPANSOES = 7000;
    static const int MAX_PONTOS = 96;
    static const int MAX_EVITAR = 16;
    struct Caminho {
        int n = 0;
        Vec3 pontos[MAX_PONTOS];
        u8 precisos[MAX_PONTOS];
        int nos[MAX_PONTOS];  // nós da rota (para evitar a aresta onde travou)
        int n_nos = 0;
        bool vazio() const { return n == 0; }
    };
    bool carregar(const u8* dados, u32 tamanho, char* erro, int n_erro);
    int n_nos() const { return N * N + n_andares_; }
    int n_andares() const { return n_andares_; }
    int n_ligacoes() const { return n_lig_; }
    int n_comodos() const { return n_comodos_; }
    Vec3 comodo(int i) const { return Vec3(comodos_[i * 4], comodos_[i * 4 + 1], comodos_[i * 4 + 2]); }
    bool bloqueada(int i, int j) const { return bloq_[j * N + i] != 0; }
    f32 altura(int k) const { return alt_[k] * 0.01f; }
    Vec3 posicao(int no) const;
    int no_de(Vec3 p) const;
    static u32 chave_aresta(int a, int b) { return static_cast<u32>(a < b ? a : b) * 131072u + static_cast<u32>(a < b ? b : a); }
    // A* de `de` até `para`, sem as arestas em `evitar`. Devolve false se não achou no orçamento.
    bool caminho(Vec3 de, Vec3 para, const u32* evitar, int n_evitar, Caminho& saida);
    u32 caminhos = 0, expansoes = 0;  // diagnóstico
private:
    struct Andar { f32 x, y, z; i16 i, j, nivel, _; i32 viz[4]; };
    struct Ligacao { i32 a, b; f32 custo; u16 ponto, n_pontos; };
    bool livre(int de_i, int de_j, int para_i, int para_j) const;
    bool porta_entre(int a, int b) const;
    int ligacao(int a, int b) const;   // índice da ligação a→b ou −1
    int livre_perto(int i, int j) const;
    const i16* alt_ = nullptr;
    const u8* bloq_ = nullptr;
    const u8* arestas_ = nullptr;
    const u8* portas_ = nullptr;
    const Andar* andares_ = nullptr;
    const Ligacao* lig_ = nullptr;
    const f32* pontos_ = nullptr;
    const f32* comodos_ = nullptr;
    int n_andares_ = 0, n_lig_ = 0, n_pontos_ = 0, n_comodos_ = 0;
    f32 t_ = 4, limite_ = 300;
};
