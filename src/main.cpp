// Brisa — ponto de entrada (PS2 e PC). Etapa 2: cena de teste do motor 3D e
// benchmark dos caminhos de renderização (EE e VU1).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jogo/cena_teste.h"
#include "nucleo/arena.h"
#include "nucleo/estatisticas.h"
#include "nucleo/passo_fixo.h"
#include "nucleo/versao.h"
#include "plataforma/plataforma.h"

#ifndef BRISA_AUTOTESTE
#define BRISA_AUTOTESTE 0  // > 0: roda o benchmark ao iniciar e sai depois de N quadros ou do fim do benchmark
#endif

static ListaDesenho2D g_hud;
static Cena3D g_cena;
alignas(64) static u8 g_memoria_cena[2 * 1024 * 1024];

static u32 argumento(int argc, char** argv, const char* nome, u32 padrao) {
    size_t n = strlen(nome);
    for (int i = 1; i < argc; i++)
        if (strncmp(argv[i], nome, n) == 0) return static_cast<u32>(atoi(argv[i] + n));
    return padrao;
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

    Arena arena(g_memoria_cena, sizeof g_memoria_cena);
    CenaTeste cena_teste;
    if (!cena_teste.iniciar(arena)) {
        plataforma::log("BRISA: falha ao criar as malhas da cena de teste\n");
        return 1;
    }
    snprintf(msg, sizeof msg, "BRISA: malhas da cena: %u KB | VU1 %s\n", static_cast<unsigned>(arena.usado() / 1024),
             plataforma::caminho_disponivel(Caminho3D::VU1) ? "disponivel" : "indisponivel");
    plataforma::log(msg);
    if (!plataforma::caminho_disponivel(cena_teste.caminho())) cena_teste.definir_caminho(Caminho3D::EE);

    const u32 max_quadros = argumento(argc, argv, "--quadros=", BRISA_AUTOTESTE);
    // Autoteste: a mesma cena parada pelo VU1 (quadros 0–119) e pelo EE (120–239),
    // para comparar as imagens, e depois o benchmark completo.
    const bool roteiro = BRISA_AUTOTESTE > 0 || argumento(argc, argv, "--roteiro=", 0);
    if (roteiro) {
        cena_teste.definir_nivel(3, false);
        cena_teste.pausar(true);
        if (plataforma::caminho_disponivel(Caminho3D::VU1)) cena_teste.definir_caminho(Caminho3D::VU1);
    } else if (argumento(argc, argv, "--bench=", 0)) {
        cena_teste.iniciar_bench();
    }
    int nivel = static_cast<int>(argumento(argc, argv, "--nivel=", 100));
    if (nivel != 100) cena_teste.definir_nivel(nivel - 1, argumento(argc, argv, "--carga=", 0) != 0);

    Controle controle;
    PassoFixo passo(30, 2);
    EstatisticasQuadro est;
    Estatisticas3D e3d;
    u64 anterior = plataforma::tempo_us();
    u32 quadro = 0;
    bool viu_conectado = false, bench_visto = cena_teste.bench_ativo();

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
            cena_teste.passo(controle);
        }
        cena_teste.montar(g_cena, g_hud, est, e3d, info);
        u32 cpu = static_cast<u32>(plataforma::tempo_us() - inicio);
        plataforma::desenhar_quadro(g_cena, cena_teste.caminho(), g_hud, e3d);
        if (quadro > 0) {
            est.registrar(dt, cpu);
            cena_teste.medir(dt, cpu, e3d);
        }
        quadro++;
        if (roteiro && (quadro == 120 || quadro == 240)) {
            snprintf(msg, sizeof msg, "BRISA_ROTEIRO caminho=%s objetos=%u/%u tris=%u desenhados_ee=%u lotes_vu1=%u cortados_perto=%u t3d_us=%u\n",
                     cena_teste.caminho() == Caminho3D::VU1 ? "VU1" : "EE", static_cast<unsigned>(e3d.objetos_visiveis),
                     static_cast<unsigned>(e3d.objetos), static_cast<unsigned>(e3d.triangulos_enviados),
                     static_cast<unsigned>(e3d.triangulos_desenhados), static_cast<unsigned>(e3d.lotes_vu1),
                     static_cast<unsigned>(e3d.objetos_cortados_perto), static_cast<unsigned>(e3d.tempo_3d_us));
            plataforma::log(msg);
            if (quadro == 120) cena_teste.definir_caminho(Caminho3D::EE);
            else cena_teste.iniciar_bench();
        }
        if (cena_teste.bench_ativo()) bench_visto = true;

        if (quadro % 300 == 0) {
            snprintf(msg, sizeof msg, "BRISA: quadro %u | caminho %s | tris %u | quadro %u us | cpu %u us | 3d %u us | prims2d %d | descartadas %d\n",
                     static_cast<unsigned>(quadro), cena_teste.caminho() == Caminho3D::VU1 ? "VU1" : "EE",
                     static_cast<unsigned>(e3d.triangulos_enviados), static_cast<unsigned>(est.media_quadro_us()),
                     static_cast<unsigned>(est.media_trabalho_us()), static_cast<unsigned>(e3d.tempo_3d_us),
                     g_hud.tamanho(), g_hud.descartadas());
            plataforma::log(msg);
        }
        if (max_quadros && (quadro >= max_quadros || (bench_visto && !cena_teste.bench_ativo()))) break;
    }
    plataforma::log("BRISA: encerrando\n");
    plataforma::encerrar();
    plataforma::log("BRISA_FIM_OK\n");
    return 0;
}
