// Movimento do personagem (portátil), com os números de gameplay/character/
// movement.gd e pawn.gd do Ventania: andar 4,5 m/s, correr 6,5 (só para a
// frente), agachado 2,5, gravidade 22, salto de 7,4 m/s (~1,4 m), aceleração 45
// no chão e 9 no ar, queda máxima 45, "grudar" a 0,45 m, cilindro de 0,35 m de
// raio e 1,8 m (agachado 1,15). Passo fixo de 30 Hz.
#pragma once
#include "mundo/colisao.h"

namespace mov {
const f32 ANDAR = 4.5f, CORRER = 6.5f, AGACHADO = 2.5f;
const f32 GRAVIDADE = 22.0f, SALTO = 7.4f, GRUDAR_VEL = -1.5f, GRUDAR_DIST = 0.45f;
const f32 ACEL_CHAO = 45.0f, ACEL_AR = 9.0f, QUEDA_MAX = 45.0f, CORRER_FRENTE_MIN = 0.5f;
const f32 RAIO = 0.35f, ALTURA = 1.8f, ALTURA_AGACHADO = 1.15f;
// Degrau: obstáculos com topo até pés + DEGRAU são subidos andando (o Ventania usa a
// cápsula da Godot; aqui é explícito). Rampas da construção (45°) também sobem por aqui.
const f32 DEGRAU = 0.35f;
// Brisa (BR-ADR-13): sem dano de queda; acima desta altura, só a aterrissagem pesada.
const f32 QUEDA_PESADA = 6.0f;
// Entrada na partida (movement.gd: queda depois de saltar do ônibus).
const f32 QUEDA_TERMINAL = 26.0f, QUEDA_HORIZONTAL = 10.0f;
const f32 MERGULHO_TERMINAL = 38.0f, MERGULHO_HORIZONTAL = 6.0f, MERGULHO_PITCH = -0.7854f;  // −45°
const f32 PLANADOR_DESCIDA = 6.5f, PLANADOR_HORIZONTAL = 12.0f, PLANADOR_ALTURA = 38.0f, QUEDA_ACEL = 12.0f;
}  // namespace mov

// Estado de entrada na partida (Pawn.queda do Ventania).
enum EstadoQueda { NO_CHAO_NORMAL = 0, QUEDA_LIVRE = 1, PLANADOR = 2, NO_ONIBUS = 3 };

struct ComandoMovimento {
    f32 x = 0, y = 0;   // −1..1 (y > 0 = para a frente)
    f32 yaw = 0;        // radianos (a direção da câmera)
    f32 pitch = 0;      // radianos (olhar para baixo < 0): mergulho na queda livre
    bool pular = false, correr = false, agachar = false;
};

struct Personagem {
    Vec3 pos;           // pés
    Vec3 vel;
    f32 yaw = 0;
    bool no_chao = false, agachado = false, correndo = false;
    // Para a animação e os testes:
    bool pulou = false;          // pulou neste passo
    bool aterrissou = false;     // tocou o chão neste passo
    f32 altura_queda = 0;        // altura da queda que acabou de terminar
    f32 pico = 0;                // y mais alto desde que saiu do chão
    int queda = NO_CHAO_NORMAL;  // EstadoQueda (entrada na partida)
    f32 altura() const { return agachado ? mov::ALTURA_AGACHADO : mov::ALTURA; }
};

// Mergulho: em queda livre, olhando bem para baixo e indo para a frente.
inline bool mergulhando(int queda, const ComandoMovimento& c) {
    return queda == QUEDA_LIVRE && c.pitch < mov::MERGULHO_PITCH && c.y > 0.3f;
}

// Um passo de simulação (dt = 1/30 s).
void passo_movimento(Personagem& p, const ComandoMovimento& c, const MundoColisao& mundo, f32 dt);
