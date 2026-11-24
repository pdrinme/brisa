// A partida Battle Royale (gameplay/match/partida_br.gd do Ventania), offline:
//  CONTAGEM — 10 s, cada um num ponto de início, sem dano;
//  ONIBUS   — loot novo, inventários vazios (só a picareta), todos no Ônibus dos
//             Ventos; cada um salta quando quer (portas abertas sobre terra); quem
//             fica é ejetado antes de o ônibus sair da ilha. Queda livre → planador
//             → chão. Sem dano entre jogadores.
//  JOGO     — começa quando o ônibus sai: a Tormenta nasce aqui. Sem reaparecer;
//             3 s de proteção no início. Eliminado = espectador.
//  FIM      — vencedor anunciado; depois de fim_s, a tela de resultado.
// Vitória: sobra 1. O dano da Tormenta é por segundo, só na vida; se ele
// eliminaria todos os que restam, o de mais vida sobrevive (é o vencedor).
#pragma once
#include "conteudo/pak.h"
#include "lutador.h"
#include "mundo/mapa.h"
#include "partida/onibus.h"
#include "partida/tormenta.h"

class GrupoBots;

enum EstadoPartida { PART_FORA = 0, PART_CONTAGEM, PART_ONIBUS, PART_JOGO, PART_FIM };

struct Participante {
    u16 id = 0;
    char nome[24] = "";
    int colocacao = 0, eliminacoes = 0;
    u32 inicio = 0, fim = 0;
    int assento = 0;
    bool bot = true;
};

// A tela de resultado (tela_resultado.gd): o que aconteceu com um participante.
struct ResultadoPartida {
    int colocacao = 0, total = 0, eliminacoes = 0, tempo_s = 0, dano = 0, baus = 0, curas = 0;
    bool venceu = false;
    char vencedor[24] = "";
};

class PartidaBR {
public:
    static const int MAX = 16;
    bool iniciar(const Pacote& pak, const MapaBrisa* mapa, char* erro, int n_erro);
    // Nova partida com estes lutadores (o jogador primeiro, se houver): contagem.
    void comecar(u32 semente, Lutador** lutadores, int n, u32 tick, Loot& loot, GrupoBots* grupo);
    // Um passo (30 Hz), depois de todos se moverem: ônibus, saltos, Tormenta, dano, vitória.
    void passo(u32 tick);
    // Chamado pelo GrupoBots ao eliminar alguém (tiro ou Tormenta).
    void eliminado(u16 vitima, u16 quem, u32 tick);
    bool dano_ativo() const;          // tiros acertam (só no jogo, depois da proteção)
    int estado() const { return estado_; }
    bool ativa() const { return estado_ != PART_FORA; }
    void encerrar() { estado_ = PART_FORA; }
    bool acabou() const { return estado_ == PART_FIM && t_ >= ticks(cfg_.fim_s); }  // hora do resultado
    int vivos() const { return n_vivos_; }
    int total() const { return n_; }
    u16 vencedor() const { return vencedor_; }
    const char* nome_vencedor() const;
    int ticks_estado() const { return t_; }
    int ticks_restantes(u32 tick) const;  // contagem, ônibus (portas fecham), Tormenta, fim
    const Tormenta& tormenta() const { return tormenta_; }
    const Onibus& onibus() const { return onibus_; }
    bool tem_tormenta() const { return estado_ >= PART_JOGO; }
    const ConfigPartida& config() const { return cfg_; }
    const Participante* participante(u16 id) const;
    ResultadoPartida resultado(u16 id) const;
    bool vivo(u16 id) const;
    // A zona que importa agora para quem foge (a próxima se já fecha ou falta pouco).
    void zona_segura(f32* cx, f32* cz, f32* r) const;
    // Diagnóstico e testes.
    int saltaram = 0, ejetados = 0, mortes_tormenta = 0;
    u32 tick_jogo = 0;                 // tick em que a Tormenta nasceu
private:
    void mudar(int e) { estado_ = e; t_ = 0; }
    int ticks(f32 s) const { return static_cast<int>(s * 30.0f + 0.5f); }
    void saltar(Lutador& l, u32 tick);
    Vec3 assento(int k, u32 tick) const;
    void dano_tormenta(u32 tick);
    void checar_vitoria(u32 tick);
    Lutador* lutador(u16 id) const;
    ConfigPartida cfg_;
    const MapaBrisa* mapa_ = nullptr;
    GrupoBots* grupo_ = nullptr;
    Loot* loot_ = nullptr;
    Tormenta tormenta_;
    Onibus onibus_;
    Lutador* lut_[MAX] = {};
    Participante part_[MAX];
    int n_ = 0, n_vivos_ = 0;
    int estado_ = PART_FORA;
    int t_ = 0;
    u32 semente_ = 0;
    u16 vencedor_ = 0;
};
