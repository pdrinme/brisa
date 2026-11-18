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
    {600, 601, BT_BAIXO, 128, 128, 128, 128, nullptr},          // troca o caminho VU1 → EE
    {601, 630, 0, 128, 128, 128, 128, nullptr},
    {630, 631, BT_ESQUERDA, 128, 128, 128, 128, nullptr},       // acena
    {631, 680, 0, 128, 128, 128, 128, "acenou"},
};
const int N_ROTEIRO_TREINO = sizeof(ROTEIRO_TREINO) / sizeof(ROTEIRO_TREINO[0]);
