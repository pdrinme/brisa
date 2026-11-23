// Regras de combate de um jogador (combat_server.gd do Ventania), por passo de
// 30 Hz: troca de item, tiro (com dispersão e projéteis), recarga, consumíveis,
// golpe da picareta, abrir baú (segurando), pegar itens e munição automática.
// Os tempos contam em passos, como no Ventania.
#pragma once
#include "combate/inventario.h"
#include "combate/loot.h"
#include "combate/tiro.h"
#include "mundo/colisao.h"
#include "personagem/movimento.h"
#include "personagem/portador.h"

// Algo que pode levar dano (bonecos do treino; jogadores e bots nas etapas seguintes).
struct AlvoCombate {
    u16 id = 0;
    Vec3 pes;
    bool agachado = false;
    Vitais* vit = nullptr;
    u32* ultimo_acerto = nullptr;
};

struct EntradaCombate {
    bool atirar = false;          // R2 (segurado)
    bool mirar = false;           // L2
    bool recarregar = false;      // ■ (borda)
    bool interagir = false;       // ▲ (segurado)
    int ciclo = 0;                // R1 = +1, L1 = −1 (borda)
    bool picareta = false;        // ▶ (borda)
    f32 yaw = 0, pitch = 0;       // rad, da câmera
};

struct EventosCombate {
    static const int MAX_ACERTOS = 16;
    bool atirou = false;
    Vec3 tiro_origem;
    int n_fins = 0;
    Vec3 fins[tiro::MAX_PROJETEIS];
    bool fim_acertou[tiro::MAX_PROJETEIS];
    struct Acerto { Vec3 pos; int dano; bool cabeca, escudo, eliminou; u16 alvo; };
    Acerto acertos[MAX_ACERTOS];
    int n_acertos = 0;
    bool golpe_mundo = false;
    Vec3 golpe_pos;
    int bau_aberto = -1;
    int pegou_item = 0, pegou_raridade = 0, pegou_qtd = 0;
    Inventario::Erro erro = Inventario::OK;
    bool recarregou = false, usou = false, trocou = false, municao = false;
};

class Combatente {
public:
    static const int PASSOS_TROCA = 6;  // 0,2 s para sacar ao trocar de espaço
    void iniciar(const BancoItens* b, u16 id);
    Inventario inv;
    Vitais vit;
    u16 id() const { return id_; }
    void passo(const EntradaCombate& e, const Personagem& p, const MundoColisao& mundo, Loot& loot,
               AlvoCombate* alvos, int n_alvos, u32 tick, EventosCombate& ev);
    // Ação em andamento para o visual e o HUD: ACAO_* e progresso 0..1.
    int acao() const;
    f32 progresso() const;
    int bau_interagindo() const { return bau_ >= 0 && passos_bau_ > 0 ? bau_ : -1; }
    u32 golpes() const { return golpes_; }
    u32 seq() const { return seq_; }
private:
    void cancelar() { recarga_fim_ = uso_fim_ = golpe_ini_ = -1; }
    void disparar(const EntradaCombate& e, const Personagem& p, const MundoColisao& m, AlvoCombate* alvos, int n, u32 tick, EventosCombate& ev);
    void golpe(const EntradaCombate& e, const Personagem& p, const MundoColisao& m, AlvoCombate* alvos, int n, u32 tick, EventosCombate& ev);
    void dano(AlvoCombate& a, int dano, bool cabeca, Vec3 pos, u32 tick, EventosCombate& ev);
    bool pode_usar(int item) const;
    void interagir(const EntradaCombate& e, const Personagem& p, const MundoColisao& m, Loot& loot, EventosCombate& ev);
    Vec3 onde_largar(const Personagem& p, f32 yaw, const MundoColisao& m) const;
    const BancoItens* b_ = nullptr;
    u16 id_ = 0;
    f32 passos_ = 0, disparo_ = 0;
    f32 recarga_fim_ = -1, recarga_total_ = 0, uso_fim_ = -1, uso_total_ = 0;
    f32 golpe_ini_ = -1, golpe_ant_ = -1;
    bool golpe_acertou_ = false;
    int combo_ = 0;
    bool atirar_ant_ = false, interagir_ant_ = false;
    int bau_ = -1, passos_bau_ = 0;
    u32 seq_ = 0, golpes_ = 0;
};
