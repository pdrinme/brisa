// O personagem controlado pelo jogador: movimento (personagem/movimento),
// câmera em 3ª pessoa, animador, traje, combate (jogo/combatente) e o item na mão
// (personagem/portador). Usado pelo campo de treino e pela ilha. Controles do
// Ventania no DualShock 2 (docs/TESTE_NO_CONSOLE.md):
//   ✕ pula · L3 corre · R3 agacha · L2 mira · R2 atira/usa · ■ recarrega ·
//   ▲ interage (segurar para abrir baú) · R1/L1 próximo/anterior item · ▶ picareta · ◀ acena.
#pragma once
#include "combatente.h"
#include "conteudo/animacao.h"
#include "conteudo/biblioteca.h"
#include "mundo/colisao.h"
#include "nucleo/controle.h"
#include "personagem/animador.h"
#include "personagem/camera3p.h"
#include "personagem/movimento.h"
#include "personagem/portador.h"
#include "render3d/cena3d.h"

class Jogador {
public:
    static const int N_TRAJES = 11;
    static const int MAX_ITENS = 16;
    // Carrega trajes, animações, os dados dos itens e os modelos dos itens na biblioteca.
    bool iniciar(const Pacote& pak, BibliotecaModelos& bib, char* erro, int n_erro);
    void reposicionar(Vec3 pes, f32 yaw_camera_graus, f32 pitch_graus = -10.0f);
    // 30 Hz. `bloquear` (inventário aberto) suspende tiro, uso e interação.
    void passo(const Controle& c, const MundoColisao& mundo, Loot& loot, AlvoCombate* alvos, int n_alvos, u32 tick,
               bool bloquear, EventosCombate& ev);
    // Personagem com o item na mão, sombra blob e câmera na cena.
    void montar(Cena3D& cena, const BibliotecaModelos& bib, const MundoColisao& mundo, const Loot* loot, f32 aspecto);
    const Personagem& personagem() const { return p_; }
    const Animador& animador() const { return anim_; }
    const Camera3P& camera() const { return cam_; }
    const BancoAnimacoes& animacoes() const { return anims_; }
    const BancoItens& itens() const { return itens_; }
    const Combatente& combate() const { return comb_; }
    Combatente& combate_mut() { return comb_; }
    const Portador& portador() const { return port_; }
    int modelo_do_item(int item) const { return item >= 0 && item < MAX_ITENS ? modelo_item_[item] : -1; }
    Vec3 cano() const { return cano_; }   // boca da arma no mundo (último quadro)
    Mat4 matriz() const;                  // personagem no mundo
    int traje() const { return traje_; }
    void definir_traje(int t) { traje_ = ((t % N_TRAJES) + N_TRAJES) % N_TRAJES; }
private:
    BancoAnimacoes anims_;
    BancoItens itens_;
    Animador anim_;
    Camera3P cam_;
    Personagem p_;
    Combatente comb_;
    Portador port_;
    int trajes_[N_TRAJES];
    int modelo_item_[MAX_ITENS];
    int traje_ = 0;
    bool agachar_ = false;
    const BibliotecaModelos* bib_ = nullptr;
    Vec3 cano_;
};
