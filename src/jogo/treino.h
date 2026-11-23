// Etapa 4: campo de treino do Ventania (world/arena/campo_treino.gd) com o
// personagem jogável: movimento, câmera em terceira pessoa, animações e colisão.
#pragma once
#include "conteudo/animacao.h"
#include "conteudo/biblioteca.h"
#include "mundo/colisao.h"
#include "nucleo/controle.h"
#include "nucleo/estatisticas.h"
#include "combate/loot.h"
#include "hud_combate.h"
#include "jogador.h"
#include "plataforma/plataforma.h"

class Treino {
public:
    static const int N_TRAJES = 11;
    static const int N_BONECOS = 3;
    struct Boneco { Vec3 pos; Vitais vit; u32 ultimo = 0; };
    bool iniciar(const Pacote& pak, Arena& arena, char* erro, int n_erro);
    void passo(const Controle& c);   // 30 Hz
    void montar(Cena3D& cena, ListaDesenho2D& hud, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                const InfoPlataforma& info);
    Caminho3D caminho() const { return caminho_; }
    void definir_caminho(Caminho3D c) { caminho_ = c; }
    const Personagem& personagem() const { return jogador_.personagem(); }
    const Animador& animador() const { return jogador_.animador(); }
    const Camera3P& camera() const { return jogador_.camera(); }
    const MundoColisao& mundo() const { return mundo_; }
    u32 passos() const { return passos_; }
    void definir_traje(int t) { jogador_.definir_traje(t); }
    static Vec3 ponto_inicial() { return Vec3(4.0f, 0.2f, 10.0f); }  // spawn_point(0) do Ventania
    // Roteiro e testes: põe o personagem num lugar, com a câmera olhando para `yaw` graus.
    void posicionar(Vec3 pes, f32 yaw, f32 pitch);
    const Jogador& jogador() const { return jogador_; }
    const Loot& loot() const { return loot_; }
    const Boneco& boneco(int i) const { return bonecos_[i]; }
    const EventosCombate& eventos() const { return ev_; }  // do último passo
    int dano_nos_bonecos() const { return dano_bonecos_; }
    int eliminacoes() const { return eliminacoes_; }
private:
    struct Inst { int modelo; Vec3 pos; f32 rot; int bau = -1; };
    void hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d, const InfoPlataforma& info) const;
    BibliotecaModelos bib_;
    MundoColisao mundo_;
    Jogador jogador_;
    Loot loot_;
    HudCombate hc_;
    Boneco bonecos_[N_BONECOS];
    EventosCombate ev_;
    int modelo_boneco_ = -1, clipe_idle_ = -1, dano_bonecos_ = 0, eliminacoes_ = 0;
    Inst inst_[80];
    int n_inst_ = 0;
    Malha chao_;
    int textura_sombra_ = -1;
    Caminho3D caminho_ = Caminho3D::VU1;
    bool hud_ = true;
    u32 passos_ = 0;
};
