// Loot (loot_server.gd do Ventania): itens no chão, baús e munição. Pontos de
// loot: 75% de chance de item (60% arma com uma caixa de munição, senão um
// consumível). Baú: arma (raridade de baú) + a munição dela + consumível + uma
// caixa de munição qualquer. Sorteios determinísticos pela semente.
#pragma once
#include "inventario.h"
#include "nucleo/mat.h"

struct ItemChao {
    u16 id = 0;           // 0 = livre
    u8 item = 0;          // espaço: índice do BancoItens; 0 se reserva
    u8 raridade = 0;
    u8 municao = 0;       // tipo + 1 (reserva de munição)
    u8 material = 0;      // tipo + 1
    u16 qtd = 0;
    i16 pente = -1;
    Vec3 pos;
    u16 dono = 0;         // quem largou não recoleta a munição até `ate`
    u32 ate = 0;
    Vec3 de;              // visual: salta daqui até pos (baú abrindo)
    f32 salto = 1.0f;     // 0..1
    bool reserva() const { return municao != 0 || material != 0; }
};

struct BauMundo {
    Vec3 pos;
    f32 rot = 0;          // graus
    bool aberto = false;
    u32 reabrir = 0;
    u16 abrindo = 0;      // quem está abrindo (um por vez)
    f32 tampa = 0;        // visual: 0 fechada .. 1 aberta
};

class Loot {
public:
    static const int MAX_ITENS = 384, MAX_BAUS = 64, MAX_PONTOS = 128;
    static constexpr f32 PEGAR_DIST = 2.2f, BAU_DIST = 2.4f, MUNICAO_AUTO_DIST = 1.3f;
    static const int ABRIR_BAU_PASSOS = 15;         // segurar 0,5 s
    static const u32 BLOQUEIO_DONO = 2 * 30;
    static const u32 REABASTECER = 45 * 30;
    bool reabastecer = true;  // treino: baús fecham e reabastecem; partida: não
    void iniciar(const BancoItens* b, u32 semente);
    void adicionar_bau(Vec3 pos, f32 rot);
    void adicionar_ponto(Vec3 pos);
    void sortear_pontos();                          // depois de todos os pontos
    int criar_item(int item, int raridade, int qtd, int pente, Vec3 pos, u16 dono = 0, u32 tick = 0);
    int criar_municao(int tipo, Vec3 pos, int qtd = -1, u16 dono = 0, u32 tick = 0);
    void remover(int indice);
    int n_itens() const { return n_itens_; }        // posições usadas (algumas podem estar livres)
    const ItemChao& item(int i) const { return itens_[i]; }
    int n_baus() const { return n_baus_; }
    const BauMundo& bau(int i) const { return baus_[i]; }
    BauMundo& bau_mut(int i) { return baus_[i]; }
    int mais_perto(Vec3 pos) const;                 // item de espaço até PEGAR_DIST (índice) ou −1
    int bau_perto(Vec3 pos) const;                  // baú fechado até BAU_DIST ou −1
    // Pega para o inventário; o que não cabe fica no chão, o trocado cai em pos_troca.
    Inventario::Erro pegar(int indice, Inventario& inv, int troca, Vec3 pos_troca);
    bool coletar_municao(Inventario& inv, Vec3 pos, u16 jogador, u32 tick);
    void abrir_bau(int i);
    void atualizar(u32 tick, f32 dt);               // reabastece e anima (tampa, saltos)
    void largar_inventario(Inventario& inv, Vec3 pos);
    int contar(int item) const;                     // testes de conservação
    int contar_municao(int tipo) const;
private:
    int criar(const ItemChao& base);
    void sortear_chao(Vec3 p);
    const BancoItens* b_ = nullptr;
    Aleatorio rng_;
    ItemChao itens_[MAX_ITENS];
    int n_itens_ = 0;
    u16 proximo_id_ = 1;
    BauMundo baus_[MAX_BAUS];
    int n_baus_ = 0;
    Vec3 pontos_[MAX_PONTOS];
    int n_pontos_ = 0;
};
