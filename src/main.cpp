// Brisa — ponto de entrada (PS2 e PC). Etapa 3: vitrine do kit do Ventania com
// os assets convertidos (padrão) e a cena de benchmark da Etapa 2 (L3 + R3 alterna).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conteudo/pak.h"
#include "jogo/cena_teste.h"
#include "jogo/vitrine.h"
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
static Vitrine g_vitrine;
static CenaTeste g_cena_teste;

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
    CenaTeste& cena_teste = g_cena_teste;
    if (!cena_teste.iniciar(arena)) {
        plataforma::log("BRISA: falha ao criar as malhas da cena de teste\n");
        return 1;
    }
    // Conteúdo: o pacote (BRISA.PAK) e a vitrine montada com ele.
    u32 tam_pak = 0;
    const u8* dados_pak = plataforma::conteudo(&tam_pak);
    Pacote pak;
    char erro[160] = "";
    bool tem_vitrine = dados_pak && pak.abrir(dados_pak, tam_pak, erro, sizeof erro) &&
                       g_vitrine.iniciar(pak, arena, erro, sizeof erro);
    if (!tem_vitrine) {
        snprintf(msg, sizeof msg, "BRISA: conteúdo indisponível: %s\n", dados_pak ? erro : "pacote ausente");
        plataforma::log(msg);
    } else {
        char e2[96] = "";
        int falhas = pak.verificar(e2, sizeof e2);
        snprintf(msg, sizeof msg, "BRISA_CONTEUDO arquivos=%u bytes=%u versao=%u crc_falhas=%d modelos=%u bytes_modelos=%u arena=%u\n",
                 static_cast<unsigned>(pak.n()), static_cast<unsigned>(pak.tamanho()), static_cast<unsigned>(pak.versao_conteudo()),
                 falhas, static_cast<unsigned>(g_vitrine.n_modelos()), static_cast<unsigned>(g_vitrine.bytes_modelos()),
                 static_cast<unsigned>(arena.usado()));
        plataforma::log(msg);
        if (!plataforma::caminho_disponivel(g_vitrine.caminho())) g_vitrine.definir_caminho(Caminho3D::EE);
    }
    snprintf(msg, sizeof msg, "BRISA: VU1 %s\n", plataforma::caminho_disponivel(Caminho3D::VU1) ? "disponivel" : "indisponivel");
    plataforma::log(msg);
    if (!plataforma::caminho_disponivel(cena_teste.caminho())) cena_teste.definir_caminho(Caminho3D::EE);
    bool modo_vitrine = tem_vitrine && argumento(argc, argv, "--cena=", 0) == 0;

    const u32 max_quadros = argumento(argc, argv, "--quadros=", BRISA_AUTOTESTE);
    // Autoteste (Etapa 3): a vitrine parada pelo VU1 (quadros 0–119) e pelo EE
    // (120–239), depois cada traje em cada LOD e cada animação, e sai.
    const bool roteiro = BRISA_AUTOTESTE > 0 || argumento(argc, argv, "--roteiro=", 0);
    const int traje_inicial = static_cast<int>(argumento(argc, argv, "--traje=", 0));
    if (modo_vitrine) {
        g_vitrine.definir_traje(traje_inicial);
        g_vitrine.definir_lod(static_cast<int>(argumento(argc, argv, "--lod=", 0)));
        if (argumento(argc, argv, "--sem-hud=", 0)) g_vitrine.mostrar_hud(false);
    }
    if (roteiro && modo_vitrine) g_vitrine.pausar(true);  // a mesma imagem nos dois caminhos
    if (roteiro && modo_vitrine && plataforma::caminho_disponivel(Caminho3D::VU1)) g_vitrine.definir_caminho(Caminho3D::VU1);
    if (!modo_vitrine && argumento(argc, argv, "--bench=", 0)) cena_teste.iniciar_bench();
    int nivel = static_cast<int>(argumento(argc, argv, "--nivel=", 100));
    if (nivel != 100) cena_teste.definir_nivel(nivel - 1, argumento(argc, argv, "--carga=", 0) != 0);
    const u32 Q_TRAJES = 240 + Vitrine::N_TRAJES * 3 * 6;

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
            if (tem_vitrine && controle.segurando(BT_L3) && controle.pressionou(BT_R3)) modo_vitrine = !modo_vitrine;
            if (modo_vitrine) g_vitrine.passo(controle); else cena_teste.passo(controle);
        }
        if (roteiro && modo_vitrine) g_vitrine.mostrar_hud(quadro >= 60);  // 0–59: imagem limpa para comparar
        if (modo_vitrine) g_vitrine.montar(g_cena, g_hud, est, e3d, info);
        else cena_teste.montar(g_cena, g_hud, est, e3d, info);
        u32 cpu = static_cast<u32>(plataforma::tempo_us() - inicio);
        Caminho3D caminho = modo_vitrine ? g_vitrine.caminho() : cena_teste.caminho();
        plataforma::desenhar_quadro(g_cena, caminho, g_hud, e3d);
        if (quadro > 0) {
            est.registrar(dt, cpu);
            if (!modo_vitrine) cena_teste.medir(dt, cpu, e3d);
        }
        quadro++;
        if (roteiro && modo_vitrine && quadro >= 240) {
            // Percorre os trajes × LOD (6 quadros cada) e depois os clipes (4 quadros cada).
            u32 k = quadro - 240;
            if (quadro < Q_TRAJES) {
                g_vitrine.definir_traje(static_cast<int>(k / 18));
                g_vitrine.definir_lod(1 + static_cast<int>((k / 6) % 3));
            } else {
                u32 c = (quadro - Q_TRAJES) / 4;
                if (c >= static_cast<u32>(g_vitrine.n_clipes())) {
                    snprintf(msg, sizeof msg, "BRISA_VITRINE_OK trajes=%d lods=3 clipes=%d\n", Vitrine::N_TRAJES, g_vitrine.n_clipes());
                    plataforma::log(msg);
                    break;
                }
                g_vitrine.definir_traje(0);
                g_vitrine.definir_lod(Vitrine::LOD_0);
                g_vitrine.definir_clipe(static_cast<int>(c));
            }
        }
        if (roteiro && (quadro == 120 || quadro == 240)) {
            snprintf(msg, sizeof msg, "BRISA_ROTEIRO caminho=%s objetos=%u/%u tris=%u desenhados_ee=%u lotes_vu1=%u cortados_perto=%u t3d_us=%u\n",
                     caminho == Caminho3D::VU1 ? "VU1" : "EE", static_cast<unsigned>(e3d.objetos_visiveis),
                     static_cast<unsigned>(e3d.objetos), static_cast<unsigned>(e3d.triangulos_enviados),
                     static_cast<unsigned>(e3d.triangulos_desenhados), static_cast<unsigned>(e3d.lotes_vu1),
                     static_cast<unsigned>(e3d.objetos_cortados_perto), static_cast<unsigned>(e3d.tempo_3d_us));
            plataforma::log(msg);
            if (modo_vitrine) {
                if (quadro == 120) g_vitrine.definir_caminho(Caminho3D::EE);
                else g_vitrine.pausar(false);
            } else if (quadro == 120) {
                cena_teste.definir_caminho(Caminho3D::EE);
            } else {
                cena_teste.iniciar_bench();
            }
        }
        if (cena_teste.bench_ativo()) bench_visto = true;

        if (quadro % 300 == 0) {
            snprintf(msg, sizeof msg, "BRISA: quadro %u | caminho %s | tris %u | quadro %u us | cpu %u us | 3d %u us | prims2d %d | descartadas %d\n",
                     static_cast<unsigned>(quadro), caminho == Caminho3D::VU1 ? "VU1" : "EE",
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
