// Brisa — ponto de entrada (PS2 e PC). Etapa 1: tela de prova de vida.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jogo/tela_boot.h"
#include "nucleo/estatisticas.h"
#include "nucleo/passo_fixo.h"
#include "nucleo/versao.h"
#include "plataforma/plataforma.h"

#ifndef BRISA_AUTOTESTE
#define BRISA_AUTOTESTE 0  // > 0: roda esse número de quadros, imprime o resumo e sai
#endif

static ListaDesenho2D g_lista;  // estática: ~320 KB fora da pilha

static u32 quadros_pedidos(int argc, char** argv) {
    for (int i = 1; i < argc; i++)
        if (strncmp(argv[i], "--quadros=", 10) == 0) return static_cast<u32>(atoi(argv[i] + 10));
    return BRISA_AUTOTESTE;
}

int main(int argc, char** argv) {
    InfoPlataforma info;
    if (!plataforma::iniciar(info, argc, argv)) {
        plataforma::log("BRISA: falha ao iniciar a plataforma\n");
        return 1;
    }
    char msg[320];
    snprintf(msg, sizeof msg, "BRISA %s (etapa %d) em %s | video %s | memoria livre %u KB | programa %u KB | boot '%s' | modulos %s\n",
             BRISA_VERSAO, BRISA_ETAPA, info.nome, info.video, static_cast<unsigned>(info.memoria_livre / 1024),
             static_cast<unsigned>(info.tamanho_programa / 1024), info.caminho_boot, info.modulos);
    plataforma::log(msg);
    if (info.memoria_detalhe[0]) {
        snprintf(msg, sizeof msg, "BRISA: memoria: %s\n", info.memoria_detalhe);
        plataforma::log(msg);
    }

    const u32 max_quadros = quadros_pedidos(argc, argv);
    TelaBoot tela;
    Controle controle;
    PassoFixo passo(30, 2);
    EstatisticasQuadro est;
    u64 anterior = plataforma::tempo_us();
    u32 quadro = 0;
    bool viu_conectado = false;

    for (;;) {
        u64 inicio = plataforma::tempo_us();
        u32 dt = static_cast<u32>(inicio - anterior);
        anterior = inicio;

        EstadoControle bruto;
        plataforma::ler_controle(bruto);
        controle.ler(bruto);
        if (controle.conectado() && !viu_conectado) {
            viu_conectado = true;
            plataforma::log("BRISA: controle 1 conectado\n");
        }

        u32 n = passo.avancar(quadro == 0 ? 0 : dt);
        for (u32 i = 0; i < n; i++) {
            controle.iniciar_passo();
            tela.passo(controle);
        }

        tela.montar(g_lista, controle, est, info);
        u32 trabalho = static_cast<u32>(plataforma::tempo_us() - inicio);
        plataforma::desenhar_e_apresentar(g_lista);
        if (quadro > 0) est.registrar(dt, trabalho);
        quadro++;

        if (quadro % 600 == 0 || (max_quadros && quadro == max_quadros)) {
            snprintf(msg, sizeof msg,
                     "BRISA: quadro %u | media %u us | pior %u us | cpu media %u us | cpu pior (total) %u us | prims %d | descartadas %d | passos %u\n",
                     static_cast<unsigned>(quadro), static_cast<unsigned>(est.media_quadro_us()),
                     static_cast<unsigned>(est.pior_quadro_us()), static_cast<unsigned>(est.media_trabalho_us()),
                     static_cast<unsigned>(est.pior_trabalho_total_us()), g_lista.tamanho(), g_lista.descartadas(),
                     static_cast<unsigned>(tela.passos()));
            plataforma::log(msg);
        }
        if (max_quadros && quadro >= max_quadros) break;
    }
    plataforma::log("BRISA: encerrando\n");
    plataforma::encerrar();
    plataforma::log("BRISA_FIM_OK\n");
    return 0;
}
