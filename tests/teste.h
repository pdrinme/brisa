// Mini-framework de testes do núcleo (roda no PC, sem dependências).
#pragma once
#include <stdio.h>

struct CasoTeste {
    const char* nome;
    void (*fn)();
    CasoTeste* prox;
};
extern CasoTeste* g_casos;
extern int g_falhas_caso;
extern int g_verificacoes;

struct RegistroTeste {
    CasoTeste caso;
    RegistroTeste(const char* nome, void (*fn)()) : caso{nome, fn, nullptr} {
        CasoTeste** p = &g_casos;
        while (*p) p = &(*p)->prox;
        *p = &caso;
    }
};

#define TESTE(nome)                                   \
    static void nome();                               \
    static RegistroTeste registro_##nome(#nome, nome); \
    static void nome()

#define VERIFICA(cond)                                                              \
    do {                                                                            \
        g_verificacoes++;                                                           \
        if (!(cond)) {                                                              \
            g_falhas_caso++;                                                        \
            printf("    FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
        }                                                                           \
    } while (0)
