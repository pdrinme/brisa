// Um personagem que luta: o jogador ou um bot (os bots do Ventania são
// "jogadores do servidor", com as mesmas regras, entradas e física). Junta o
// movimento, o combate, a animação e o item na mão; quem comanda é o Jogador
// (controle e câmera) ou o cérebro do bot (ia/cerebro).
#pragma once
#include "combatente.h"
#include "conteudo/animacao.h"
#include "conteudo/biblioteca.h"
#include "mundo/colisao.h"
#include "personagem/animador.h"
#include "personagem/movimento.h"
#include "personagem/portador.h"
#include "render3d/cena3d.h"

// Compartilhado por todos os lutadores: animações, itens e os modelos (trajes com LODs).
struct RecursosLutador {
    static const int N_TRAJES = 11;
    static const int MAX_ITENS = 16;
    BancoAnimacoes anims;
    BancoItens itens;
    const BibliotecaModelos* bib = nullptr;
    int trajes[N_TRAJES][3];   // LOD0, LOD1, LOD2
    int modelo_item[MAX_ITENS];
    bool iniciar(const Pacote& pak, BibliotecaModelos& bib, char* erro, int n_erro);
    int traje_por_id(const char* id) const;  // índice do traje ou 0
};

struct ComandoLutador {
    ComandoMovimento mov;
    EntradaCombate comb;
    bool acenar = false;
};

class Lutador {
public:
    void iniciar(const RecursosLutador* r, u16 id, int traje, const char* nome);
    void reposicionar(Vec3 pes, f32 yaw_rad);
    // Um passo de 30 Hz. `pitch_visual` e `mirando` alimentam o portador (a pose).
    void passo(const ComandoLutador& c, const MundoColisao& m, Loot& loot, AlvoCombate* alvos, int n_alvos, u32 tick,
               EventosCombate& ev, bool com_visual = true);
    // Na cena: lod 0..2 (o item na mão só no 0 e no 1).
    void montar(Cena3D& cena, const MundoColisao& m, int lod, bool sombra = true);
    Mat4 matriz() const;
    AlvoCombate como_alvo();
    Personagem p;
    Animador anim;
    Combatente comb;
    Portador port;
    int traje = 0;
    u16 id = 0;
    char nome[24] = "";
    bool vivo = true;
    u32 reviver = 0;          // tick para reaparecer (se a partida deixa)
    u32 ultimo_acerto = 0;
    Vec3 cano;                // boca da arma no mundo (último quadro desenhado)
    const RecursosLutador* rec() const { return r_; }
private:
    const RecursosLutador* r_ = nullptr;
};
