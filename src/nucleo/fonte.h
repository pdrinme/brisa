// Fonte bitmap própria 5×7 (original do Brisa), em caixa alta, com os acentos
// do português. Cada glifo tem 10 linhas: 2 para o acento, 7 para a letra e 1
// para a cedilha. Minúsculas são desenhadas como maiúsculas (estilo "adesivo",
// caixa alta). Usada até a Etapa 10, quando entra a Fredoka rasterizada.
#pragma once
#include "desenho2d.h"

namespace fonte {

const int LARGURA = 5;
const int ALTURA = 10;   // linhas por glifo (acento + corpo + cedilha)
const int AVANCO = 6;    // colunas por caractere (com o espaço)

// Bits de um glifo: linha r (0..9), bit 4 = coluna 0. Devolve false se o
// código não tem glifo (desenhado como '?').
bool glifo(u32 codigo, u8 linhas[ALTURA]);

// Largura em pixels de um texto UTF-8 na escala dada.
f32 largura(const char* utf8, f32 escala);

// Desenha o texto UTF-8 juntando pixels vizinhos da mesma linha num retângulo.
// y é o topo da área do acento. Devolve o x final.
f32 desenhar(ListaDesenho2D& lista, f32 x, f32 y, f32 escala, Cor cor, const char* utf8);

// Texto com contorno (8 direções) e sombra dura deslocada (estilo do Ventania).
void desenhar_adesivo(ListaDesenho2D& lista, f32 x, f32 y, f32 escala, Cor cor, Cor contorno,
                      f32 espessura, f32 sombra, const char* utf8);

// Próximo código Unicode de uma string UTF-8 (avança *s). 0 no fim.
u32 proximo_codigo(const char** s);

}  // namespace fonte
