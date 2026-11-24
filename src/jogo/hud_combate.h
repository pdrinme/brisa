// Partes do combate que o treino e a ilha compartilham (Etapa 6):
// - loot no chão (loot_visual.gd: modelo girando sobre um anel na cor da raridade)
//   e a tampa dos baús (clipe "abrir");
// - efeitos: traçantes, marcador de acerto, números de dano, faísca da picareta;
// - HUD de combate (combat_hud.gd): vida e escudo, barra de itens com os ícones,
//   nome e munição, barra de ação, aviso de interação, mensagens e a mira;
// - tela de inventário (SELECT): trocar de lugar, largar e dividir.
#pragma once
#include "combate/loot.h"
#include "conteudo/biblioteca.h"
#include "jogador.h"
#include "nucleo/controle.h"
#include "nucleo/desenho2d.h"
#include "render3d/cena3d.h"

// Símbolos dos botões do DualShock e linhas (compartilhados com o HUD da partida).
enum BotaoHud { BOTAO_TRIANGULO, BOTAO_CIRCULO, BOTAO_XIS, BOTAO_QUADRADO };
void desenhar_botao(ListaDesenho2D& l, f32 x, f32 y, f32 tam, int botao);
void desenhar_linha(ListaDesenho2D& l, f32 x0, f32 y0, f32 x1, f32 y1, f32 espessura, Cor c);

class HudCombate {
public:
    bool iniciar(const Pacote& pak, BibliotecaModelos& bib, Arena& arena, const BancoItens& itens, char* erro, int n_erro);
    // Itens no chão (até `alcance` m da câmera) e os baús nas suas poses.
    void montar_loot(Cena3D& cena, const Loot& loot, const BibliotecaModelos& bib, const Jogador& j, f32 t) const;
    // Pose do baú com a tampa na fração dada (0 fechada, 1 aberta).
    const Pose* pose_bau(f32 tampa) const;
    int modelo_bau() const { return bau_; }
    // Eventos de um passo (efeitos e mensagens).
    void registrar(const EventosCombate& ev, const Jogador& j);
    void passo();  // 30 Hz: os efeitos somem
    // Inventário aberto? Os controles da tela vêm daqui (devolve true se consumiu o controle).
    bool inventario_aberto() const { return inv_aberto_; }
    bool controlar_inventario(const Controle& c, Jogador& j, Loot& loot, const MundoColisao& m);
    void mensagem(const char* texto);
    void desenhar(ListaDesenho2D& l, const Jogador& j, const Loot& loot, const Camera3D& cam) const;
    f32 alcance_loot = 50.0f;
private:
    void hotbar(ListaDesenho2D& l, const Jogador& j) const;
    void tela_inventario(ListaDesenho2D& l, const Jogador& j) const;
    void aviso(ListaDesenho2D& l, const Jogador& j, const Loot& loot) const;
    const BancoItens* itens_ = nullptr;
    int icone_[16];
    int icone_municao_ = -1;
    Malha aneis_[7];          // 6 raridades + cinza (munição)
    int municao_modelo_ = -1, bau_ = -1;
    const ModeloBrisa* bau_modelo_ = nullptr;
    BancoAnimacoes anim_bau_;
    int clipe_abrir_ = -1;
    mutable Pose pose_bau_;
    struct Tracante { Vec3 a, b; int vida; bool acertou; };
    Tracante trac_[24];
    int n_trac_ = 0;
    struct Numero { Vec3 pos; int dano, vida; bool cabeca, escudo; };
    Numero num_[16];
    int n_num_ = 0;
    int marcador_ = 0;        // passos restantes do marcador de acerto
    bool marcador_cabeca_ = false;
    char msg_[96] = "";
    int msg_vida_ = 0;
    bool inv_aberto_ = false;
    int cursor_ = 0, marcado_ = -1;
};
