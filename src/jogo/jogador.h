// O lutador controlado pelo jogador: o Lutador (movimento, combate, animação e
// item na mão) mais a câmera em 3ª pessoa e o controle. Controles do Ventania no
// DualShock 2 (docs/TESTE_NO_CONSOLE.md):
//   ✕ pula · L3 corre · R3 agacha · L2 mira · R2 atira/usa · ■ recarrega ·
//   ▲ interage (segurar para abrir baú) · R1/L1 próximo/anterior item · ▶ picareta · ◀ acena.
#pragma once
#include "lutador.h"
#include "nucleo/controle.h"
#include "personagem/camera3p.h"

class Jogador {
public:
    static const int N_TRAJES = RecursosLutador::N_TRAJES;
    static const int MAX_ITENS = RecursosLutador::MAX_ITENS;
    static const u16 ID = 1;
    // Carrega trajes (com LODs), animações, os dados dos itens e os modelos dos itens.
    bool iniciar(const Pacote& pak, BibliotecaModelos& bib, char* erro, int n_erro);
    void reposicionar(Vec3 pes, f32 yaw_camera_graus, f32 pitch_graus = -10.0f);
    // 30 Hz. `bloquear` (inventário aberto) suspende tiro, uso e interação.
    void passo(const Controle& c, const MundoColisao& mundo, Loot& loot, AlvoCombate* alvos, int n_alvos, u32 tick,
               bool bloquear, EventosCombate& ev);
    // Personagem com o item na mão, sombra blob e câmera na cena.
    void montar(Cena3D& cena, const BibliotecaModelos& bib, const MundoColisao& mundo, const Loot* loot, f32 aspecto);
    const Personagem& personagem() const { return l_.p; }
    const Animador& animador() const { return l_.anim; }
    const Camera3P& camera() const { return cam_; }
    const BancoAnimacoes& animacoes() const { return rec_.anims; }
    const BancoItens& itens() const { return rec_.itens; }
    const Combatente& combate() const { return l_.comb; }
    Combatente& combate_mut() { return l_.comb; }
    const Portador& portador() const { return l_.port; }
    int modelo_do_item(int item) const { return item >= 0 && item < MAX_ITENS ? rec_.modelo_item[item] : -1; }
    Vec3 cano() const { return l_.cano; }
    Mat4 matriz() const { return l_.matriz(); }
    int traje() const { return l_.traje; }
    void definir_traje(int t) { l_.traje = ((t % N_TRAJES) + N_TRAJES) % N_TRAJES; }
    // Espectador: com o jogador eliminado, a câmera segue este lutador (ou fica parada).
    void espectar(const Lutador* alvo) { espectado_ = alvo; }
    const Lutador* espectado() const { return espectado_; }
    Lutador& lutador() { return l_; }
    const Lutador& lutador() const { return l_; }
    const RecursosLutador& recursos() const { return rec_; }
private:
    RecursosLutador rec_;
    Lutador l_;
    Camera3P cam_;
    bool agachar_ = false;
    const Lutador* espectado_ = nullptr;
};
