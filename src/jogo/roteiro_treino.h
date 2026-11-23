// Roteiro de entrada do autoteste da Etapa 4 (o mesmo no PC e no PS2): o
// controle "segura" estes estados por faixa de passos (30 Hz). Partindo do
// ponto inicial (4, 0, 10) olhando para −z:
//   1. vai para x = 0 e sobe a torre de pedra pelas duas rampas até o piso a 8 m;
//   2. sai pela lateral e cai 8 m (aterrissagem pesada);
//   3. pula no chão (altura do salto);
//   4. gira a câmera 180° e anda agachado;
//   5. corre, para, troca o caminho de desenho (VU1 → EE) e acena.
// A passagem agachada debaixo dos pisos a 1,5 m fica nos testes do PC
// (tests/teste_personagem.cpp).
#pragma once
#include "nucleo/controle.h"

struct FaixaRoteiro {
    u32 inicio, fim;   // passos [inicio, fim)
    u16 botoes;
    u8 lx, ly, rx, ry;
    const char* marco; // registrado no log ao terminar a faixa (ou nullptr)
};

const FaixaRoteiro ROTEIRO_TREINO[] = {
    {0, 30, 0, 128, 128, 128, 128, "parado"},
    {30, 57, 0, 0, 128, 128, 128, "lado_esquerdo"},              // x: 4 → 0
    {57, 330, 0, 128, 0, 128, 128, "topo_da_torre"},             // frente: sobe as rampas até 8 m
    {330, 345, 0, 128, 128, 128, 128, nullptr},
    {345, 375, 0, 255, 128, 128, 128, "queda_da_torre"},         // direita: sai pela lateral e cai
    {375, 400, 0, 128, 128, 128, 128, "aterrissou"},
    {400, 401, BT_XIS, 128, 128, 128, 128, nullptr},             // pulo
    {401, 440, 0, 128, 128, 128, 128, "pulo"},
    {440, 480, 0, 128, 128, 0, 128, "virou_camera"},             // analógico direito: gira a câmera
    {480, 481, BT_R3, 128, 128, 128, 128, nullptr},              // agacha
    {481, 520, 0, 128, 0, 128, 128, "agachado_andando"},
    {520, 521, BT_R3, 128, 128, 128, 128, nullptr},              // levanta
    {521, 600, BT_L3, 128, 0, 128, 128, "correndo"},
    {600, 601, BT_L1 | BT_R1 | BT_SELECT, 128, 128, 128, 128, nullptr},  // troca o caminho VU1 → EE (atalho)
    {601, 630, 0, 128, 128, 128, 128, nullptr},
    {630, 631, BT_ESQUERDA, 128, 128, 128, 128, nullptr},       // acena
    {631, 680, 0, 128, 128, 128, 128, "acenou"},
};
const int N_ROTEIRO_TREINO = sizeof(ROTEIRO_TREINO) / sizeof(ROTEIRO_TREINO[0]);

// Etapa 6: roteiro de combate no campo de treino, logo depois do de movimento.
// `lugar`: 1 = em frente ao baú 4 (−7, 0, 3); 2 = mirando no boneco 2 a 8 m (o
// ombro alinhado com ele); 3 = ao lado do boneco 2, ao alcance da picareta.
struct FaixaCombate {
    u32 inicio, fim;
    u16 botoes;
    u8 lugar;
    const char* marco;
};
const FaixaCombate ROTEIRO_COMBATE[] = {
    {0, 1, BT_L1 | BT_R1 | BT_SELECT, 1, nullptr},  // volta para o VU1 e vai ao baú
    {1, 25, BT_TRIANGULO, 0, "bau_aberto"},         // segura ▲ (abre em 15 passos)
    {25, 45, 0, 0, nullptr},                        // os itens saltam e pousam
    {45, 46, BT_TRIANGULO, 0, nullptr},             // pega o mais perto
    {46, 48, 0, 0, nullptr},
    {48, 49, BT_TRIANGULO, 0, nullptr},             // e o seguinte
    {49, 52, 0, 0, "pegou"},
    {52, 53, BT_R1, 0, nullptr},                    // saca o espaço 1
    {53, 60, 0, 0, "na_mao"},
    {60, 70, 0, 2, nullptr},                        // de frente para o boneco 2
    {70, 130, BT_R2, 0, "atirou"},                  // 2 s de gatilho
    {130, 131, BT_QUADRADO, 0, nullptr},            // recarrega
    {131, 280, 0, 0, "recarregou"},
    {280, 281, BT_R1, 0, nullptr},                  // espaço 2 (o consumível, se houver)
    {281, 290, 0, 0, nullptr},
    {290, 400, BT_R2, 0, "consumivel"},             // usa (se puder)
    {400, 401, BT_DIREITA, 3, nullptr},             // picareta, ao lado do boneco
    {401, 410, 0, 0, nullptr},
    {410, 460, BT_R2, 0, "golpes"},                 // golpes seguidos
    {460, 461, BT_SELECT, 0, nullptr},              // abre o inventário
    {461, 465, 0, 0, nullptr},
    {465, 466, BT_QUADRADO, 0, nullptr},            // larga o espaço 1
    {466, 470, 0, 0, nullptr},
    {470, 471, BT_SELECT, 0, nullptr},              // fecha
    {471, 490, 0, 0, "inventario"},
};
const int N_ROTEIRO_COMBATE = sizeof(ROTEIRO_COMBATE) / sizeof(ROTEIRO_COMBATE[0]);
