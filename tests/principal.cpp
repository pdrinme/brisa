#include <string.h>

#include "teste.h"

CasoTeste* g_casos = nullptr;
int g_falhas_caso = 0;
int g_verificacoes = 0;

int main(int argc, char** argv) {
    int total = 0, falhas = 0;
    for (CasoTeste* c = g_casos; c; c = c->prox) {
        if (argc > 1 && !strstr(c->nome, argv[1])) continue;
        g_falhas_caso = 0;
        c->fn();
        total++;
        printf("[%s] %s\n", g_falhas_caso ? "FALHOU" : "OK", c->nome);
        if (g_falhas_caso) falhas++;
    }
    printf("\n%d testes, %d verificações, %d falhas\n", total, g_verificacoes, falhas);
    if (!falhas) printf("TESTES_OK\n");
    return falhas ? 1 : 0;
}
