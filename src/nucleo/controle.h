// Estado do controle, independente da plataforma (o DualShock 2 é traduzido
// para isto em plataforma/ps2). Os nomes seguem os símbolos do controle.
#pragma once
#include "tipos.h"

enum Botao : u16 {
    BT_SELECT = 1 << 0, BT_L3 = 1 << 1, BT_R3 = 1 << 2, BT_START = 1 << 3,
    BT_CIMA = 1 << 4, BT_DIREITA = 1 << 5, BT_BAIXO = 1 << 6, BT_ESQUERDA = 1 << 7,
    BT_L2 = 1 << 8, BT_R2 = 1 << 9, BT_L1 = 1 << 10, BT_R1 = 1 << 11,
    BT_TRIANGULO = 1 << 12, BT_CIRCULO = 1 << 13, BT_XIS = 1 << 14, BT_QUADRADO = 1 << 15,
};

enum class ModoControle : u8 { DESCONECTADO, DIGITAL, ANALOGICO };

struct EstadoControle {
    ModoControle modo = ModoControle::DESCONECTADO;
    u16 botoes = 0;           // bits de Botao, 1 = pressionado
    u8 lx = 128, ly = 128;    // analógico esquerdo, 0..255 (128 = centro)
    u8 rx = 128, ry = 128;    // analógico direito
};

// Lê o controle a cada quadro (60 Hz) e entrega as bordas a cada passo da
// simulação (30 Hz): um toque que acontece num quadro sem passo não se perde,
// e um quadro com 2 passos não conta o mesmo toque duas vezes.
class Controle {
public:
    void ler(const EstadoControle& novo) {
        pendentes_ |= static_cast<u16>(novo.botoes & ~atual_.botoes);
        atual_ = novo;
    }
    void iniciar_passo() { do_passo_ = pendentes_; pendentes_ = 0; }
    const EstadoControle& estado() const { return atual_; }
    bool conectado() const { return atual_.modo != ModoControle::DESCONECTADO; }
    bool segurando(u16 b) const { return (atual_.botoes & b) != 0; }
    bool pressionou(u16 b) const { return (do_passo_ & b) != 0; }
    // Eixo em -1..1 com zona morta (fração de 0..1 do curso).
    static f32 eixo(u8 v, f32 zona_morta = 0.18f);
private:
    EstadoControle atual_;
    u16 pendentes_ = 0;
    u16 do_passo_ = 0;
};
