// O personagem controlado pelo jogador: movimento (personagem/movimento),
// câmera em 3ª pessoa, animador e traje. Usado pelo campo de treino (Etapa 4) e
// pela ilha (Etapa 5); os controles são os do Ventania (ver docs/TESTE_NO_CONSOLE.md).
#pragma once
#include "conteudo/animacao.h"
#include "conteudo/biblioteca.h"
#include "mundo/colisao.h"
#include "nucleo/controle.h"
#include "personagem/animador.h"
#include "personagem/camera3p.h"
#include "personagem/movimento.h"
#include "render3d/cena3d.h"

class Jogador {
public:
    static const int N_TRAJES = 11;
    bool iniciar(const Pacote& pak, BibliotecaModelos& bib, char* erro, int n_erro);
    void reposicionar(Vec3 pes, f32 yaw_camera_graus, f32 pitch_graus = -10.0f);
    // 30 Hz: R3 agacha/levanta, L2 mira, analógicos, ✕ pula, L3 corre, ◀ acena.
    void passo(const Controle& c, const MundoColisao& mundo);
    // Personagem, sombra blob e câmera na cena.
    void montar(Cena3D& cena, const BibliotecaModelos& bib, const MundoColisao& mundo, f32 aspecto) const;
    const Personagem& personagem() const { return p_; }
    const Animador& animador() const { return anim_; }
    const Camera3P& camera() const { return cam_; }
    int traje() const { return traje_; }
    void definir_traje(int t) { traje_ = ((t % N_TRAJES) + N_TRAJES) % N_TRAJES; }
private:
    BancoAnimacoes anims_;
    Animador anim_;
    Camera3P cam_;
    Personagem p_;
    int trajes_[N_TRAJES];
    int traje_ = 0;
    bool agachar_ = false;
};
