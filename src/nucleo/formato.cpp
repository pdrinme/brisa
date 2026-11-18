#include "formato.h"

namespace formato {

int juntar(char* dst, int tamanho, const char* src) {
    int n = 0;
    while (n < tamanho - 1 && dst[n]) n++;
    while (n < tamanho - 1 && *src) dst[n++] = *src++;
    if (tamanho > 0) dst[n] = 0;
    return n;
}

int decimal(char* saida, int tamanho, u32 valor, u32 divisor, int casas) {
    if (tamanho <= 0) return 0;
    u32 escala = 1;
    for (int i = 0; i < casas; i++) escala *= 10;
    u64 v = divisor ? (static_cast<u64>(valor) * escala + divisor / 2) / divisor : 0;
    u64 inteiro = v / escala;
    u64 frac = v % escala;
    char tmp[32];
    int n = 0;
    do { tmp[n++] = static_cast<char>('0' + inteiro % 10); inteiro /= 10; } while (inteiro && n < 20);
    int k = 0;
    while (n > 0 && k < tamanho - 1) saida[k++] = tmp[--n];
    if (casas > 0 && k < tamanho - 1) {
        saida[k++] = ',';
        char f[12];
        for (int i = casas - 1; i >= 0; i--) { f[i] = static_cast<char>('0' + frac % 10); frac /= 10; }
        for (int i = 0; i < casas && k < tamanho - 1; i++) saida[k++] = f[i];
    }
    saida[k] = 0;
    return k;
}

}  // namespace formato
