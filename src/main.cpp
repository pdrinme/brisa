// Brisa — ponto de entrada (PS2 e PC). Etapa 4: campo de treino com o personagem
// jogável (padrão), a vitrine do kit (Etapa 3) e a cena de benchmark (Etapa 2).
// L1 + R1 + START alterna os modos.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conteudo/pak.h"
#include "jogo/cena_teste.h"
#include "jogo/roteiro_treino.h"
#include "jogo/treino.h"
#include "jogo/vitrine.h"
#include "nucleo/arena.h"
#include "nucleo/estatisticas.h"
#include "nucleo/passo_fixo.h"
#include "nucleo/versao.h"
#include "plataforma/plataforma.h"

#ifndef BRISA_AUTOTESTE
#define BRISA_AUTOTESTE 0  // > 0: roda o roteiro do treino e sai (no máximo N quadros)
#endif

static ListaDesenho2D g_hud;
static Cena3D g_cena;
alignas(64) static u8 g_memoria_cena[2 * 1024 * 1024];
static Vitrine g_vitrine;
static Treino g_treino;
static CenaTeste g_cena_teste;

enum Modo { TREINO, VITRINE, BENCH, N_MODOS };

static u32 argumento(int argc, char** argv, const char* nome, u32 padrao) {
    size_t n = strlen(nome);
    for (int i = 1; i < argc; i++)
        if (strncmp(argv[i], nome, n) == 0) return static_cast<u32>(atoi(argv[i] + n));
    return padrao;
}

static void log_marco(const char* marco, const Treino& t) {
    char msg[256];
    const Personagem& p = t.personagem();
    snprintf(msg, sizeof msg, "BRISA_MARCO %s passo=%u pos=%.2f,%.2f,%.2f no_chao=%d agachado=%d queda=%.2f anim=%s camera_yaw=%.1f\n",
             marco, static_cast<unsigned>(t.passos()), static_cast<double>(p.pos.x), static_cast<double>(p.pos.y),
             static_cast<double>(p.pos.z), p.no_chao ? 1 : 0, p.agachado ? 1 : 0, static_cast<double>(p.altura_queda),
             t.animador().estado(), static_cast<double>(t.camera().yaw));
    plataforma::log(msg);
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
    if (!g_cena_teste.iniciar(arena)) {
        plataforma::log("BRISA: falha ao criar as malhas da cena de teste\n");
        return 1;
    }
    u32 tam_pak = 0;
    const u8* dados_pak = plataforma::conteudo(&tam_pak);
    Pacote pak;
    char erro[160] = "";
    bool tem_pak = dados_pak && pak.abrir(dados_pak, tam_pak, erro, sizeof erro);
    bool tem_vitrine = tem_pak && g_vitrine.iniciar(pak, arena, erro, sizeof erro);
    if (tem_pak && !tem_vitrine) {
        snprintf(msg, sizeof msg, "BRISA: vitrine indisponível: %s\n", erro);
        plataforma::log(msg);
    }
    bool tem_treino = tem_pak && g_treino.iniciar(pak, arena, erro, sizeof erro);
    if (!tem_pak || !tem_treino) {
        snprintf(msg, sizeof msg, "BRISA: conteúdo indisponível: %s\n", dados_pak ? erro : "pacote ausente");
        plataforma::log(msg);
    } else {
        char e2[96] = "";
        int falhas = pak.verificar(e2, sizeof e2);
        snprintf(msg, sizeof msg, "BRISA_CONTEUDO arquivos=%u bytes=%u versao=%u crc_falhas=%d modelos=%u bytes_modelos=%u arena=%u colisores=%d\n",
                 static_cast<unsigned>(pak.n()), static_cast<unsigned>(pak.tamanho()), static_cast<unsigned>(pak.versao_conteudo()),
                 falhas, static_cast<unsigned>(g_vitrine.n_modelos()), static_cast<unsigned>(g_vitrine.bytes_modelos()),
                 static_cast<unsigned>(arena.usado()), g_treino.mundo().n());
        plataforma::log(msg);
    }
    bool vu1 = plataforma::caminho_disponivel(Caminho3D::VU1);
    snprintf(msg, sizeof msg, "BRISA: VU1 %s\n", vu1 ? "disponivel" : "indisponivel");
    plataforma::log(msg);
    if (!vu1) {
        g_vitrine.definir_caminho(Caminho3D::EE);
        g_treino.definir_caminho(Caminho3D::EE);
        g_cena_teste.definir_caminho(Caminho3D::EE);
    }

    int modo = static_cast<int>(argumento(argc, argv, "--modo=", tem_treino ? TREINO : BENCH));
    if ((modo == TREINO && !tem_treino) || (modo == VITRINE && !tem_vitrine)) modo = BENCH;
    const u32 max_quadros = argumento(argc, argv, "--quadros=", BRISA_AUTOTESTE ? BRISA_AUTOTESTE : 0);
    const bool roteiro = (BRISA_AUTOTESTE > 0 || argumento(argc, argv, "--roteiro=", 0)) && modo == TREINO;
    if (argumento(argc, argv, "--sem-hud=", 0) && modo == VITRINE) g_vitrine.mostrar_hud(false);
    g_treino.definir_traje(static_cast<int>(argumento(argc, argv, "--traje=", 0)));
    if (modo == BENCH && argumento(argc, argv, "--bench=", 0)) g_cena_teste.iniciar_bench();

    Controle controle;
    PassoFixo passo(30, 2);
    EstatisticasQuadro est;
    Estatisticas3D e3d;
    u64 anterior = plataforma::tempo_us();
    u32 quadro = 0;
    int faixa = 0;
    bool registrar_3d = false;  // depois de um marco do roteiro: registra o 3D do próximo quadro
    bool viu_conectado = false;

    for (;;) {
        u64 inicio = plataforma::tempo_us();
        u32 dt = static_cast<u32>(inicio - anterior);
        anterior = inicio;

        EstadoControle bruto;
        plataforma::ler_controle(bruto);
        if (controle.conectado() && !viu_conectado) {
            viu_conectado = true;
            plataforma::log("BRISA: controle 1 conectado\n");
        }
        u32 n = passo.avancar(quadro == 0 ? 0 : dt);
        for (u32 i = 0; i < n; i++) {
            if (roteiro) {
                // O roteiro substitui o controle, passo a passo (igual no PC e no PS2).
                u32 k = g_treino.passos();
                while (faixa < N_ROTEIRO_TREINO && k >= ROTEIRO_TREINO[faixa].fim) {
                    if (ROTEIRO_TREINO[faixa].marco) { log_marco(ROTEIRO_TREINO[faixa].marco, g_treino); registrar_3d = true; }
                    faixa++;
                }
                if (faixa >= N_ROTEIRO_TREINO) break;
                const FaixaRoteiro& f = ROTEIRO_TREINO[faixa];
                bruto = EstadoControle();
                bruto.modo = ModoControle::ANALOGICO;
                bruto.botoes = f.botoes;
                bruto.lx = f.lx; bruto.ly = f.ly; bruto.rx = f.rx; bruto.ry = f.ry;
            }
            controle.ler(bruto);
            controle.iniciar_passo();
            if (controle.segurando(BT_L1) && controle.segurando(BT_R1) && controle.pressionou(BT_START)) {
                do { modo = (modo + 1) % N_MODOS; } while ((modo == TREINO && !tem_treino) || (modo == VITRINE && !tem_vitrine));
                continue;
            }
            if (modo == TREINO) g_treino.passo(controle);
            else if (modo == VITRINE) g_vitrine.passo(controle);
            else g_cena_teste.passo(controle);
        }
        if (roteiro && faixa >= N_ROTEIRO_TREINO) {
            plataforma::log("BRISA_TREINO_OK\n");
            break;
        }
        Caminho3D caminho;
        if (modo == TREINO) { g_treino.montar(g_cena, g_hud, est, e3d, info); caminho = g_treino.caminho(); }
        else if (modo == VITRINE) { g_vitrine.montar(g_cena, g_hud, est, e3d, info); caminho = g_vitrine.caminho(); }
        else { g_cena_teste.montar(g_cena, g_hud, est, e3d, info); caminho = g_cena_teste.caminho(); }
        u32 cpu = static_cast<u32>(plataforma::tempo_us() - inicio);
        plataforma::desenhar_quadro(g_cena, caminho, g_hud, e3d);
        if (registrar_3d) {
            registrar_3d = false;
            snprintf(msg, sizeof msg, "BRISA_ROTEIRO caminho=%s objetos=%u/%u tris=%u desenhados_ee=%u lotes_vu1=%u cortados_perto=%u t3d_us=%u\n",
                     caminho == Caminho3D::VU1 ? "VU1" : "EE", static_cast<unsigned>(e3d.objetos_visiveis),
                     static_cast<unsigned>(e3d.objetos), static_cast<unsigned>(e3d.triangulos_enviados),
                     static_cast<unsigned>(e3d.triangulos_desenhados), static_cast<unsigned>(e3d.lotes_vu1),
                     static_cast<unsigned>(e3d.objetos_cortados_perto), static_cast<unsigned>(e3d.tempo_3d_us));
            plataforma::log(msg);
        }
        if (quadro > 0) {
            est.registrar(dt, cpu);
            if (modo == BENCH) g_cena_teste.medir(dt, cpu, e3d);
        }
        quadro++;
        if (quadro % 300 == 0) {
            snprintf(msg, sizeof msg, "BRISA: quadro %u | modo %d | caminho %s | tris %u | quadro %u us | cpu %u us | 3d %u us | prims2d %d\n",
                     static_cast<unsigned>(quadro), modo, caminho == Caminho3D::VU1 ? "VU1" : "EE",
                     static_cast<unsigned>(e3d.triangulos_enviados), static_cast<unsigned>(est.media_quadro_us()),
                     static_cast<unsigned>(est.media_trabalho_us()), static_cast<unsigned>(e3d.tempo_3d_us), g_hud.tamanho());
            plataforma::log(msg);
        }
        if (max_quadros && quadro >= max_quadros) break;
    }
    plataforma::log("BRISA: encerrando\n");
    plataforma::encerrar();
    plataforma::log("BRISA_FIM_OK\n");
    return 0;
}
