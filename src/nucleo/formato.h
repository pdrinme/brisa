// Formatação de números para a interface (vírgula decimal, como no Ventania).
#pragma once
#include "tipos.h"

namespace formato {
// Escreve valor/divisor com 'casas' casas decimais e vírgula. Ex.: (16667, 1000, 1) -> "16,7".
// Devolve o número de caracteres escritos (sem o terminador).
int decimal(char* saida, int tamanho, u32 valor, u32 divisor, int casas);
// Concatena src ao fim de dst (tamanho total de dst). Devolve o novo comprimento.
int juntar(char* dst, int tamanho, const char* src);
}
