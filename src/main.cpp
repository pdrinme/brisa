// Brisa — ponto de entrada (PS2 e PC). Etapa 5: a Ilha Ventania (padrão), o
// campo de treino (Etapa 4), a vitrine do kit (Etapa 3) e a cena de benchmark
// (Etapa 2). L1 + R1 + START alterna os modos.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conteudo/pak.h"
#include "jogo/cena_teste.h"
#include "jogo/ilha.h"
#include "jogo/roteiro_ilha.h"
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
alignas(64) static u8 g_memoria_cena[4 * 1024 * 1024];
static Vitrine g_vitrine;
static Treino g_treino;
static Ilha g_ilha;
static CenaTeste g_cena_teste;

enum Modo { ILHA, TREINO, VITRINE, BENCH, N_MODOS };

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

// Estado do combate no log (roteiro da Etapa 6): inventário, munição, bonecos e baús.
static void log_combate(const char* marco, const Treino& t, u32 passo) {
    char msg[400];
    const Jogador& j = t.jogador();
    const Inventario& inv = j.combate().inv;
    const BancoItens& b = j.itens();
    int k = snprintf(msg, sizeof msg, "BRISA_COMBATE %s passo=%u na_mao=%s espacos=", marco, static_cast<unsigned>(passo),
                     b.id(inv.atual().item));
    for (int i = 0; i < Inventario::ESPACOS; i++) {
        const Espaco& e = inv.espacos[i];
        if (e.vazio()) k += snprintf(msg + k, sizeof msg - k, "%s-", i ? "," : "");
        else k += snprintf(msg + k, sizeof msg - k, "%s%s:%u:%u:%u", i ? "," : "", b.id(e.item), static_cast<unsigned>(e.raridade),
                           static_cast<unsigned>(e.qtd), static_cast<unsigned>(e.pente));
    }
    k += snprintf(msg + k, sizeof msg - k, " municao=");
    for (int i = 0; i < b.n_municoes(); i++) k += snprintf(msg + k, sizeof msg - k, "%s%u", i ? "," : "", static_cast<unsigned>(inv.municao[i]));
    int abertos = 0;
    for (int i = 0; i < t.loot().n_baus(); i++) abertos += t.loot().bau(i).aberto;
    snprintf(msg + k, sizeof msg - k, " vida=%d escudo=%d dano_bonecos=%d boneco2=%d/%d baus_abertos=%d golpes=%u acao=%d\n",
             j.combate().vit.vida, j.combate().vit.escudo, t.dano_nos_bonecos(), t.boneco(1).vit.vida, t.boneco(1).vit.escudo,
             abertos, static_cast<unsigned>(j.combate().golpes()), j.combate().acao());
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
    u32 arena_antes_ilha = static_cast<u32>(arena.usado());
    u64 t_ilha = plataforma::tempo_us();
    bool tem_ilha = tem_treino && g_ilha.iniciar(pak, arena, erro, sizeof erro);
    t_ilha = plataforma::tempo_us() - t_ilha;
    if (tem_treino && !tem_ilha) {
        snprintf(msg, sizeof msg, "BRISA: ilha indisponível: %s\n", erro);
        plataforma::log(msg);
    } else if (tem_ilha) {
        snprintf(msg, sizeof msg, "BRISA_ILHA carregada_us=%u instancias=%d modelos=%d colisores=%d refs=%d blocos=%d "
                 "relevo_bytes=%u arena_ilha=%u bytes_modelos=%u\n",
                 static_cast<unsigned>(t_ilha), g_ilha.mapa().n_instancias(), g_ilha.mapa().n_modelos(), g_ilha.mundo().n(),
                 g_ilha.mundo().referencias(), g_ilha.terreno().blocos(), static_cast<unsigned>(g_ilha.terreno().bytes()),
                 static_cast<unsigned>(arena.usado() - arena_antes_ilha), static_cast<unsigned>(g_ilha.biblioteca().bytes()));
        plataforma::log(msg);
    }
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
        g_ilha.definir_caminho(Caminho3D::EE);
        g_cena_teste.definir_caminho(Caminho3D::EE);
    }

    auto disponivel = [&](int m) { return (m != ILHA || tem_ilha) && (m != TREINO || tem_treino) && (m != VITRINE || tem_vitrine); };
    int modo = static_cast<int>(argumento(argc, argv, "--modo=", tem_ilha ? ILHA : BENCH));
    if (modo < 0 || modo >= N_MODOS || !disponivel(modo)) modo = BENCH;
    const u32 max_quadros = argumento(argc, argv, "--quadros=", BRISA_AUTOTESTE ? BRISA_AUTOTESTE : 0);
    // Roteiro: o do treino (Etapa 4) e depois o da ilha (Etapa 5). --roteiro=2 começa na ilha.
    const u32 arg_roteiro = argumento(argc, argv, "--roteiro=", BRISA_AUTOTESTE > 0 ? 1 : 0);
    bool roteiro = arg_roteiro > 0 && (tem_treino || tem_ilha);
    if (roteiro) modo = (arg_roteiro == 2 || !tem_treino) ? ILHA : TREINO;
    static VistaIlha vistas[32];
    const int n_vistas = tem_ilha ? vistas_da_ilha(g_ilha.mapa(), vistas, 32) : 0;
    int vista = -1;           // −1: ainda não começou o roteiro da ilha
    u32 passo_vista = 0;
    bool medir_vista = false, caminhando = false;
    u32 passos_caminhada = 0;
    if (argumento(argc, argv, "--sem-hud=", 0) && modo == VITRINE) g_vitrine.mostrar_hud(false);
    g_treino.definir_traje(static_cast<int>(argumento(argc, argv, "--traje=", 0)));
    g_ilha.definir_traje(static_cast<int>(argumento(argc, argv, "--traje=", 0)));
    if (argumento(argc, argv, "--sem-hud=", 0) && modo == ILHA) g_ilha.mostrar_hud(false);
    if (argumento(argc, argv, "--mapa-cheio=", 0)) g_ilha.mostrar_mapa(true);
    if (modo == BENCH && argumento(argc, argv, "--bench=", 0)) g_cena_teste.iniciar_bench();

    Controle controle;
    PassoFixo passo(30, 2);
    EstatisticasQuadro est;
    Estatisticas3D e3d;
    u64 anterior = plataforma::tempo_us();
    u32 quadro = 0;
    int faixa = 0, faixa_combate = 0;
    u32 inicio_combate = 0;
    bool fim_treino = false;
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
            if (roteiro && modo == ILHA) {
                // Roteiro da ilha: vistas fixas (mede o desenho) e depois uma caminhada.
                bruto = EstadoControle();
                bruto.modo = ModoControle::ANALOGICO;
                if (vista < 0 || (!caminhando && passo_vista >= PASSOS_POR_VISTA)) {
                    vista++;
                    passo_vista = 0;
                    if (vista < n_vistas) {
                        g_ilha.ir_para(vistas[vista].x, vistas[vista].z, vistas[vista].yaw, vistas[vista].pitch);
                    } else if (!caminhando) {
                        caminhando = true;
                        Vec3 s0 = g_ilha.mapa().spawn(0);
                        g_ilha.ir_para(s0.x, s0.z, atan2f(s0.x, s0.z) * 57.29578f);
                    }
                }
                if (caminhando) {
                    bruto.botoes = BT_L3;
                    bruto.ly = 0;  // frente, correndo
                    if (++passos_caminhada > PASSOS_CAMINHADA) break;
                } else if (++passo_vista == PASSOS_POR_VISTA) {
                    medir_vista = true;
                }
            } else if (roteiro) {
                // O roteiro substitui o controle, passo a passo (igual no PC e no PS2).
                u32 k = g_treino.passos();
                while (faixa < N_ROTEIRO_TREINO && k >= ROTEIRO_TREINO[faixa].fim) {
                    if (ROTEIRO_TREINO[faixa].marco) { log_marco(ROTEIRO_TREINO[faixa].marco, g_treino); registrar_3d = true; }
                    faixa++;
                }
                if (faixa >= N_ROTEIRO_TREINO) {
                    // Etapa 6: o roteiro de combate, no mesmo campo.
                    if (inicio_combate == 0) {
                        plataforma::log("BRISA_TREINO_OK\n");
                        inicio_combate = k;
                    }
                    u32 kc = k - inicio_combate;
                    while (faixa_combate < N_ROTEIRO_COMBATE && kc >= ROTEIRO_COMBATE[faixa_combate].fim) {
                        if (ROTEIRO_COMBATE[faixa_combate].marco) { log_combate(ROTEIRO_COMBATE[faixa_combate].marco, g_treino, kc); registrar_3d = true; }
                        faixa_combate++;
                    }
                    if (faixa_combate >= N_ROTEIRO_COMBATE) {
                        plataforma::log("BRISA_COMBATE_OK\n");
                        if (!tem_ilha) { fim_treino = true; break; }
                        modo = ILHA;  // segue para o roteiro da ilha
                        continue;
                    }
                    const FaixaCombate& fc = ROTEIRO_COMBATE[faixa_combate];
                    if (kc == fc.inicio && fc.lugar == 1) g_treino.posicionar(Vec3(-6.2f, 0, 4.386f), 30.0f, -15.0f);  // em frente ao baú 4
                    if (kc == fc.inicio && fc.lugar == 2) g_treino.posicionar(Vec3(-0.55f, 0, 4), 0, -3.9f);         // ombro alinhado ao boneco 2
                    if (kc == fc.inicio && fc.lugar == 3) g_treino.posicionar(Vec3(-0.55f, 0, -2.2f), 0, -10.0f);    // ao alcance da picareta
                    bruto = EstadoControle();
                    bruto.modo = ModoControle::ANALOGICO;
                    bruto.botoes = fc.botoes;
                } else {
                    const FaixaRoteiro& f = ROTEIRO_TREINO[faixa];
                    bruto = EstadoControle();
                    bruto.modo = ModoControle::ANALOGICO;
                    bruto.botoes = f.botoes;
                    bruto.lx = f.lx; bruto.ly = f.ly; bruto.rx = f.rx; bruto.ry = f.ry;
                }
            }
            controle.ler(bruto);
            controle.iniciar_passo();
            if (controle.segurando(BT_L1) && controle.segurando(BT_R1) && controle.pressionou(BT_START)) {
                do { modo = (modo + 1) % N_MODOS; } while (!disponivel(modo));
                continue;
            }
            if (modo == ILHA) g_ilha.passo(controle);
            else if (modo == TREINO) g_treino.passo(controle);
            else if (modo == VITRINE) g_vitrine.passo(controle);
            else g_cena_teste.passo(controle);
        }
        if (roteiro && caminhando && passos_caminhada > PASSOS_CAMINHADA) {
            const Personagem& p = g_ilha.jogador().personagem();
            f32 chao = g_ilha.mapa().relevo().altura(p.pos.x, p.pos.z);
            snprintf(msg, sizeof msg, "BRISA_MARCO ilha_caminhada passo=%u pos=%.2f,%.2f,%.2f no_chao=%d acima_do_relevo=%.2f anim=%s\n",
                     static_cast<unsigned>(g_ilha.passos()), static_cast<double>(p.pos.x), static_cast<double>(p.pos.y),
                     static_cast<double>(p.pos.z), p.no_chao ? 1 : 0, static_cast<double>(p.pos.y - chao),
                     g_ilha.jogador().animador().estado());
            plataforma::log(msg);
            plataforma::log("BRISA_ILHA_OK\n");
            break;
        }
        if (roteiro && fim_treino) break;
        Caminho3D caminho;
        if (modo == ILHA) { g_ilha.montar(g_cena, g_hud, est, e3d, info); caminho = g_ilha.caminho(); }
        else if (modo == TREINO) { g_treino.montar(g_cena, g_hud, est, e3d, info); caminho = g_treino.caminho(); }
        else if (modo == VITRINE) { g_vitrine.montar(g_cena, g_hud, est, e3d, info); caminho = g_vitrine.caminho(); }
        else { g_cena_teste.montar(g_cena, g_hud, est, e3d, info); caminho = g_cena_teste.caminho(); }
        u32 cpu = static_cast<u32>(plataforma::tempo_us() - inicio);
        plataforma::desenhar_quadro(g_cena, caminho, g_hud, e3d);
        if (medir_vista && modo == ILHA) {
            medir_vista = false;
            const EstatisticasIlha& ei = g_ilha.estatisticas();
            const Personagem& p = g_ilha.jogador().personagem();
            snprintf(msg, sizeof msg, "BRISA_ILHA_VISTA %s caminho=%s pos=%.1f,%.1f,%.1f tris=%u objetos=%u/%u instancias=%u/%u "
                     "lod1=%u setores=%u relevo_lod0=%u relevo_lod1=%u tris_relevo=%u cpu_corte_us=%u t3d_us=%u desenhados_ee=%u "
                     "por_categoria=%u,%u,%u,%u,%u,%u,%u\n",
                     vistas[vista].nome, caminho == Caminho3D::VU1 ? "VU1" : "EE", static_cast<double>(p.pos.x),
                     static_cast<double>(p.pos.y), static_cast<double>(p.pos.z), static_cast<unsigned>(e3d.triangulos_enviados),
                     static_cast<unsigned>(e3d.objetos_visiveis), static_cast<unsigned>(e3d.objetos), static_cast<unsigned>(ei.desenhadas),
                     static_cast<unsigned>(ei.instancias), static_cast<unsigned>(ei.em_lod1), static_cast<unsigned>(ei.setores),
                     static_cast<unsigned>(ei.terreno.blocos_lod0), static_cast<unsigned>(ei.terreno.blocos_lod1),
                     static_cast<unsigned>(ei.terreno.triangulos), static_cast<unsigned>(ei.cpu_corte_us),
                     static_cast<unsigned>(e3d.tempo_3d_us), static_cast<unsigned>(e3d.triangulos_desenhados),
                     static_cast<unsigned>(ei.tris_categoria[0]), static_cast<unsigned>(ei.tris_categoria[1]),
                     static_cast<unsigned>(ei.tris_categoria[2]), static_cast<unsigned>(ei.tris_categoria[3]),
                     static_cast<unsigned>(ei.tris_categoria[4]), static_cast<unsigned>(ei.tris_categoria[5]),
                     static_cast<unsigned>(ei.tris_categoria[6]));
            plataforma::log(msg);
        }
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
