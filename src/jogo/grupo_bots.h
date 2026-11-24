// Os bots de uma partida (game_server.gd do Ventania: add_bot, os orçamentos de
// raios e de A* por passo, bots_hear, eliminações). Cada bot é um Lutador com um
// Cerebro; o jogador entra como mais um lutador (pode ficar de fora: partida só
// de bots, para os testes de aceite).
#pragma once
#include "ia/cerebro.h"
#include "lutador.h"
#include "mundo/mapa.h"

class PartidaBR;

struct Eliminacao {
    char quem[24], vitima[24];
    int arma;      // índice do item (0 = picareta/desconhecido)
    u32 tick;
};

class GrupoBots {
public:
    static const int MAX_BOTS = 15;
    static const int MAX_LUTADORES = MAX_BOTS + 1;
    static const int FEED = 5;
    static const u32 REAPARECER = 4 * 30;  // RESPAWN_TICKS do Ventania (fora da partida BR)
    bool reaparecer = true;                 // treino livre; a partida (Etapa 8) e o teste de aceite: não
    PartidaBR* partida = nullptr;           // Etapa 8: a partida BR em andamento (ou nullptr)
    // A Tormenta eliminou `v` (feed "TORMENTA > v").
    void eliminar_pela_tormenta(Lutador& v, u32 tick);
    bool iniciar(const Pacote& pak, const RecursosLutador* rec, const MapaBrisa* mapa, Navegacao* nav, char* erro, int n_erro);
    // Cria `n` bots (≤ MAX_BOTS) nos pontos de início do mapa, a partir de `primeiro_spawn`.
    void povoar(int n, u32 semente, int primeiro_spawn);
    // 1) todos os bots pensam (antes de qualquer um se mover, como no Ventania).
    void pensar(Lutador* jogador, const MundoColisao& m, const Loot& loot, u32 tick);
    // 2) os bots andam e lutam (o jogador já andou: Ilha::passo).
    void simular(Lutador* jogador, const MundoColisao& m, Loot& loot, u32 tick);
    // 3) consequências dos eventos de um lutador: dano (avisa a vítima), tiro (ouvem a até 70 m), eliminação.
    void processar(Lutador& atirador, const EventosCombate& ev, Lutador* jogador, Loot& loot, u32 tick);
    // Alvos para o tiro de `atirador` (todos os vivos menos ele).
    int alvos(const Lutador* atirador, Lutador* jogador, AlvoCombate* saida);
    int n_bots() const { return n_bots_; }
    Lutador& bot(int i) { return lut_[i]; }
    const Lutador& bot(int i) const { return lut_[i]; }
    Cerebro& cerebro(int i) { return cer_[i]; }
    const Cerebro& cerebro(int i) const { return cer_[i]; }
    int vivos(const Lutador* jogador) const;
    const Eliminacao& feed(int i) const { return feed_[(n_feed_ - 1 - i) % FEED]; }
    int n_feed() const { return n_feed_ < FEED ? n_feed_ : FEED; }
    u32 eliminacoes() const { return eliminacoes_; }
    u16 quem_eliminou_jogador = 0;
    char nome_quem_eliminou[24] = "";
    const BancoBots& config() const { return cfg_; }
    // Diagnóstico do passo (CPU): pensar e simular, em µs.
    u32 us_pensar = 0, us_simular = 0;
private:
    void eliminar(Lutador& vitima, Lutador& quem, int arma, Loot& loot, u32 tick);
    Loot* loot_ = nullptr;
    void reviver(Lutador& l, u32 tick);
    Vec3 ponto_inicio(u32 k) const;
    const RecursosLutador* rec_ = nullptr;
    const MapaBrisa* mapa_ = nullptr;
    Navegacao* nav_ = nullptr;
    BancoBots cfg_;
    Lutador lut_[MAX_BOTS];
    Cerebro cer_[MAX_BOTS];
    EventosCombate ev_[MAX_BOTS];
    ComandoLutador cmd_[MAX_BOTS];
    int n_bots_ = 0;
    Eliminacao feed_[FEED];
    int n_feed_ = 0;
    u32 eliminacoes_ = 0, spawns_ = 0;
    ContextoBots ctx_;
    Lutador* todos_[MAX_LUTADORES];
};
