// IA de um bot (bot_cerebro.gd do Ventania). Só produz ENTRADAS (ComandoLutador),
// como um jogador: movimento, combate e inventário passam pelos mesmos sistemas.
// Decisão por "utility scoring" (5 Hz perto do jogador; 1 Hz longe = LOD de IA):
//   combater · cobertura · recuar · perseguir · investigar · curar · baú · saquear · explorar
// Execução a cada passo: seguir o caminho (A* + desvio local + manobra quando
// trava), mirar com o erro da dificuldade (a mira assenta com o tempo de visada),
// rajadas, recarregar, curar. Percepção: cone de visão com raio de visibilidade
// (orçamento de raios por passo), audição de tiros e reação ao levar dano.
// Etapa 8: salta do ônibus perto de um destino (POI ou baú), guia a queda até ele e
// foge da Tormenta (urgência pela distância, pelo tempo e por já estar fora).
// Fica para depois: coleta e construção (Etapa 9).
#pragma once
#include "combate/loot.h"
#include "dados_bots.h"
#include "jogo/lutador.h"
#include "mundo/mapa.h"
#include "mundo/navegacao.h"

class PartidaBR;

// O que o cérebro enxerga do mundo (montado pela partida a cada passo).
struct ContextoBots {
    u32 tick = 0;
    Lutador** lutadores = nullptr;   // todos (o jogador incluído)
    int n = 0;
    const MundoColisao* mundo = nullptr;
    Navegacao* nav = nullptr;
    const Loot* loot = nullptr;
    const BancoBots* cfg = nullptr;
    int raios = 0;                   // orçamento de raios de percepção neste passo
    int planejamentos = 0;           // orçamento de A* neste passo
    bool tem_humano = true;          // há um jogador humano (LOD de IA)
    const PartidaBR* partida = nullptr;  // Etapa 8: ônibus, queda e Tormenta
    const MapaBrisa* mapa = nullptr;     // POIs (destino do salto)
    Vec3 humano;
};

enum EstadoBot { EB_EXPLORAR, EB_COMBATER, EB_RECUAR, EB_COBERTURA, EB_PERSEGUIR, EB_INVESTIGAR, EB_CURAR, EB_BAU,
                 EB_SAQUEAR, EB_TORMENTA, EB_ORDEM, N_ESTADOS_BOT };
const char* nome_estado_bot(int e);

struct EstatisticasBot {
    u32 ticks_estado[N_ESTADOS_BOT] = {};
    u32 vivo_ticks = 0, preso_ticks = 0, preso_episodios = 0, manobras = 0, recuperacoes = 0, abandonos = 0, sem_rota = 0;
    u32 caminhos = 0, tempo_us = 0;
};

class Cerebro {
public:
    void iniciar(const DificuldadeBot* dif, u32 semente, Lutador* eu);
    ComandoLutador pensar(ContextoBots& ctx);
    void levou_dano(u16 atacante, Vec3 pos_atacante, const ContextoBots& ctx);
    void ouviu(u16 atirador, Vec3 pos, u32 tick);
    int estado() const { return estado_; }
    const DificuldadeBot& dificuldade() const { return *dif_; }
    EstatisticasBot est;
    static bool depurar;  // imprime os travamentos (diagnóstico)
    Vec3 ordem = Vec3(1e9f, 0, 0);   // testes de navegação: destino fixo (ignora as decisões)
    bool tem_ordem() const { return ordem.x < 1e8f; }
    u16 inimigo() const { return inimigo_; }
private:
    // Entradas do passo, como o InputCmd do Ventania.
    struct Cmd {
        f32 yaw = 0, pitch = 0, mx = 0, my = 0;
        bool fogo = false, mira = false, recarga = false, interagir = false, correr = false, pular = false;
        int espaco = -2;
    };
    void decidir(ContextoBots& c);
    void perceber(ContextoBots& c);
    void definir_alvo(ContextoBots& c);
    void executar(ContextoBots& c, Cmd& k);
    void andar(ContextoBots& c, Cmd& k, bool correr);
    void monitorar(ContextoBots& c, Cmd& k, Vec3 pos, Vec3 prox);
    void abandonar(ContextoBots& c);
    void combater(ContextoBots& c, Cmd& k);
    void disparar(ContextoBots& c, Cmd& k, bool andando);
    void mirar(Cmd& k, Vec3 ponto, f32 erro_graus);
    void curar(ContextoBots& c, Cmd& k);
    void na_cobertura(ContextoBots& c, Cmd& k);
    void olhar_em_volta(ContextoBots& c, Cmd& k);
    int melhor_arma(f32 dist) const;
    int consumivel_util() const;
    int loot_perto(const ContextoBots& c) const;
    int bau_perto(const ContextoBots& c) const;
    f32 valor_loot(const ItemChao& l) const;
    Vec3 ponto_explorar(ContextoBots& c);
    void no_onibus(ContextoBots& c, Cmd& k);
    void caindo(ContextoBots& c, Cmd& k);
    Vec3 escolher_destino_salto(ContextoBots& c);
    void zona(const ContextoBots& c, f32* cx, f32* cz, f32* r) const;
    f32 urgencia_tormenta(const ContextoBots& c) const;
    Vec3 achar_cobertura(ContextoBots& c);
    const Lutador* lutador(const ContextoBots& c, u16 id) const;
    bool ignorado(u32 chave) const;
    void ignorar(u32 chave);
    bool visivel_raio(ContextoBots& c, Vec3 a, Vec3 b);  // gasta orçamento; true = livre
    Lutador* eu_ = nullptr;
    const DificuldadeBot* dif_ = nullptr;
    Aleatorio rng_;
    int estado_ = EB_EXPLORAR;
    Vec3 alvo_;
    bool tem_alvo_ = false;
    Navegacao::Caminho caminho_;
    int idx_ = 0;
    struct Evitar { u32 chave; i32 ate; };
    Evitar evitar_[Navegacao::MAX_EVITAR];
    int n_evitar_ = 0;
    int manobra_ = 0;
    f32 manobra_lado_ = 1;
    int recuperacoes_ = 0, sem_caminho_ = 0, prog_idx_ = -1;
    Vec3 memoria_pos_;
    bool tem_memoria_ = false;
    i32 memoria_tick_ = -99999;
    Vec3 ouviu_pos_;
    bool tem_ouviu_ = false;
    i32 ouviu_tick_ = -99999;
    i32 visada_ini_ = 0, cobertura_ate_ = 0, recuar_ate_ = 0, prox_decisao_ = 0, prog_tick_ = 0;
    Vec3 cobertura_ponto_;
    bool tem_cobertura_ = false;
    f32 cobertura_sorteio_ = 0.5f;
    int observar_ = 0, rajada_ = 0;
    f32 yaw_disparo_ = 0;
    u8 visitados_[32] = {};          // cômodos já explorados (bits)
    Vec3 alvo_rota_;
    bool tem_rota_ = false, alvo_3d_ = false, precisa_caminho_ = false;
    u16 inimigo_ = 0, atacado_por_ = 0;
    Vec3 inimigo_pos_;
    i32 visto_tick_ = -99999, reagir_tick_ = 0, atacado_tick_ = -99999;
    int loot_alvo_ = 0, bau_alvo_ = -1;
    u32 ignorar_[32] = {};
    int n_ignorar_ = 0;
    f32 prog_dist_ = 1e9f;
    int sem_progresso_ = 0;
    f32 yaw_ = 0, pitch_ = 0, desvio_ = 0;
    bool alterna_ = false, recarga_ant_ = false;
    int segurar_ = 0, tentando_ = 0;
    Vec3 destino_salto_;
    bool tem_destino_ = false;
    f32 dist_onibus_ = 1e9f;
    i32 replanejar_apos_ = -99999;  // depois de uma rota que falhou: espera (anda direto) antes do próximo A*
};
