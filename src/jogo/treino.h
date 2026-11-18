// Etapa 4: campo de treino do Ventania (world/arena/campo_treino.gd) com o
// personagem jogável: movimento, câmera em terceira pessoa, animações e colisão.
#pragma once
#include "conteudo/animacao.h"
#include "conteudo/biblioteca.h"
#include "mundo/colisao.h"
#include "nucleo/controle.h"
#include "nucleo/estatisticas.h"
#include "personagem/animador.h"
#include "personagem/camera3p.h"
#include "personagem/movimento.h"
#include "plataforma/plataforma.h"

class Treino {
public:
    static const int N_TRAJES = 11;
    bool iniciar(const Pacote& pak, Arena& arena, char* erro, int n_erro);
    void passo(const Controle& c);   // 30 Hz
    void montar(Cena3D& cena, ListaDesenho2D& hud, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                const InfoPlataforma& info);
    Caminho3D caminho() const { return caminho_; }
    void definir_caminho(Caminho3D c) { caminho_ = c; }
    const Personagem& personagem() const { return p_; }
    const Animador& animador() const { return anim_; }
    const Camera3P& camera() const { return cam_; }
    const MundoColisao& mundo() const { return mundo_; }
    u32 passos() const { return passos_; }
    void definir_traje(int t) { traje_ = ((t % N_TRAJES) + N_TRAJES) % N_TRAJES; }
    static Vec3 ponto_inicial() { return Vec3(4.0f, 0.2f, 10.0f); }  // spawn_point(0) do Ventania
private:
    struct Inst { int modelo; Vec3 pos; f32 rot; };
    void hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d, const InfoPlataforma& info) const;
    BibliotecaModelos bib_;
    BancoAnimacoes anims_;
    MundoColisao mundo_;
    Animador anim_;
    Camera3P cam_;
    Personagem p_;
    Inst inst_[80];
    int n_inst_ = 0;
    int trajes_[N_TRAJES];
    int traje_ = 0;
    Malha chao_;
    int textura_sombra_ = -1;
    Caminho3D caminho_ = Caminho3D::VU1;
    bool hud_ = true, acenar_ = false, agachar_ = false;
    u32 passos_ = 0;
};
