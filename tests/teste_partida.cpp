// Etapa 8: partida Battle Royale (portados de test_tormenta.gd, test_queda.gd e
// test_partida_br.gd do Ventania): dados, Tormenta, ônibus, queda e planador, a
// partida jogada pelo roteiro e o aceite (partida BR só de bots com vencedor).
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "conteudo/pak.h"
#include "jogo/ilha.h"
#include "jogo/roteiro_partida.h"
#include "partida/onibus.h"
#include "partida/tormenta.h"
#include "plataforma/plataforma.h"
#include "teste.h"

namespace {
const Pacote* pacote_p() {
    static Pacote p;
    static bool ok = false, tentou = false;
    if (!tentou) {
        tentou = true;
        u32 t = 0;
        const u8* d = plataforma::conteudo(&t);
        char e[128];
        ok = d && p.abrir(d, t, e, sizeof e);
    }
    return ok ? &p : nullptr;
}

Ilha* ilha_br() {
    alignas(64) static u8 memoria[3 * 1024 * 1024];
    static Ilha i;
    static bool ok = false, tentou = false;
    if (!tentou && pacote_p()) {
        tentou = true;
        Arena a(memoria, sizeof memoria);
        char e[160];
        ok = i.iniciar(*pacote_p(), a, e, sizeof e);
        if (!ok) printf("    ilha: %s\n", e);
    }
    return ok ? &i : nullptr;
}

// A configuração curta do test_tormenta.gd.
ConfigPartida cfg_curta() {
    ConfigPartida c;
    c.raio_inicial = 380;
    c.margem_terra = 200;
    c.n_fases = 3;
    c.fases[0] = {2, 3, 150, 1};
    c.fases[1] = {1, 2, 60, 5};
    c.fases[2] = {1, 1, 0, 10};
    return c;
}
}  // namespace

TESTE(partida_dados_com_os_valores_do_ventania) {
    VERIFICA(pacote_p() != nullptr);
    if (!pacote_p()) return;
    u32 t = 0;
    const u8* d = pacote_p()->arquivo("dados/partida.bdat", &t);
    ConfigPartida c;
    char e[96];
    VERIFICA(d && c.carregar(d, t, e, sizeof e));
    VERIFICA(c.n_fases == 7 && c.contagem_s == 10 && c.fim_s == 15 && c.protecao_inicio_s == 3);
    VERIFICA(c.raio_inicial == 380 && c.margem_terra == 230);
    VERIFICA(c.fases[0].espera_s == 60 && c.fases[0].fechamento_s == 45 && c.fases[0].raio == 190 && c.fases[0].dano == 1);
    VERIFICA(c.fases[3].raio == 45 && c.fases[3].dano == 5 && c.fases[6].raio == 0 && c.fases[6].dano == 15);
}

TESTE(tormenta_fases_e_zonas_aninhadas) {
    ConfigPartida c = cfg_curta();
    const u32 sementes[] = {1, 2, 3, 99, 12345};
    for (u32 s : sementes) {
        Tormenta t;
        t.iniciar(c, s);
        f32 raio_ant = 380, cx_ant = 0, cz_ant = 0;
        int mudancas = 0;
        bool vistas[4] = {};
        bool aninhadas = true, dentro = true;
        for (int i = 0; i < 30 * 12; i++) {
            vistas[t.fase < 4 ? t.fase : 3] = true;
            // a próxima zona cabe inteira dentro da atual
            f32 d = sqrtf((t.para_x - cx_ant) * (t.para_x - cx_ant) + (t.para_z - cz_ant) * (t.para_z - cz_ant));
            aninhadas = aninhadas && d + t.r_para <= raio_ant + 0.01f;
            if (t.passo()) {
                mudancas++;
                if (!t.fechando) { raio_ant = t.r_de; cx_ant = t.de_x; cz_ant = t.de_z; }
            }
            dentro = dentro && t.raio() <= 380.01f;
        }
        VERIFICA(aninhadas && dentro);
        VERIFICA(t.terminou && mudancas == 6 && t.raio() == 0);
        VERIFICA(vistas[0] && vistas[1] && vistas[2] && vistas[3]);
    }
    // determinística: mesma semente, mesmas zonas
    Tormenta a, b;
    a.iniciar(c, 77);
    b.iniciar(c, 77);
    VERIFICA(a.para_x == b.para_x && a.para_z == b.para_z);
}

TESTE(tormenta_fechamento_interpolado_e_dano) {
    ConfigPartida c = cfg_curta();
    Tormenta t;
    t.iniciar(c, 7);
    VERIFICA(t.dano_por_segundo() == 1 && t.ticks_restantes() == 60);
    for (int i = 0; i < 60; i++) t.passo();
    VERIFICA(t.fechando);
    for (int i = 0; i < 45; i++) t.passo();
    f32 meio = t.raio();
    VERIFICA(meio < 380 && meio > 150);
    VERIFICA(t.fora(Vec3(1000, 0, 0)) && !t.fora(Vec3(t.cx(), 0, t.cz())));
    // dano da Tormenta: só na vida, o escudo não protege
    Vitais v;
    v.escudo = 100;
    VERIFICA(!v.dano_tormenta(30) && v.vida == 70 && v.escudo == 100);
    VERIFICA(v.dano_tormenta(500) && v.vida == 0);
}

TESTE(onibus_rota_sobre_a_ilha_e_portas) {
    Ilha* il = ilha_br();
    if (!il) return;
    const MapaBrisa& m = il->mapa();
    auto terra = [](const void* ctx, f32 x, f32 z) { return static_cast<const MapaBrisa*>(ctx)->relevo().altura(x, z) > 0.4f; };
    for (u32 s = 1; s <= 20; s++) {
        Onibus o;
        o.sortear(s, 100, 296.0f, terra, &m);
        Vec3 a = o.inicio, b = o.fim;
        VERIFICA(a.y == 120 && b.y == 120);
        VERIFICA(!terra(&m, a.x, a.z) && !terra(&m, b.x, b.z));  // entra e sai sobre o mar
        // a reta passa a até 80 m do centro
        Vec3 d = o.direcao();
        f32 dist_centro = fabsf(-a.x * d.z + a.z * d.x);
        VERIFICA(dist_centro <= 80.5f);
        VERIFICA(o.tick0 == 100 && o.abre > o.tick0 && o.fecha > o.abre && o.fecha < o.tick0 + o.duracao);
        // portas: abertas sobre terra; o trecho aberto tem pelo menos ~240 m
        Vec3 pa = o.posicao(o.abre), pf = o.posicao(o.fecha);
        VERIFICA(terra(&m, pa.x, pa.z) || terra(&m, o.posicao(o.abre + 30).x, o.posicao(o.abre + 30).z));
        VERIFICA(sqrtf(ponto(pf - pa, pf - pa)) > 200.0f);
        VERIFICA(o.portas_abertas(o.abre) && !o.portas_abertas(o.abre - 1) && !o.portas_abertas(o.fecha) && o.terminou(o.fecha));
        VERIFICA(fabsf(Onibus::VELOCIDADE * o.duracao / 30.0f - sqrtf(ponto(b - a, b - a))) < 1.0f);
    }
}

// test_queda.gd: queda livre chega à terminal; o mergulho cai mais rápido e volta sem
// tranco; o planador abre perto do chão e pousa.
TESTE(queda_livre_mergulho_e_planador) {
    MundoColisao m;
    m.limpar(2000);
    m.definir_chao(true, 0);
    Personagem p;
    p.pos = Vec3(0, 400, 0);
    p.queda = QUEDA_LIVRE;
    ComandoMovimento c;
    const f32 dt = 1.0f / 30.0f;
    for (int i = 0; i < 90; i++) passo_movimento(p, c, m, dt);
    VERIFICA(fabsf(p.vel.y + mov::QUEDA_TERMINAL) < 0.5f);
    c.pitch = -70.0f * 3.14159265f / 180.0f;
    c.y = 1;
    for (int i = 0; i < 60; i++) passo_movimento(p, c, m, dt);
    VERIFICA(-p.vel.y > mov::QUEDA_TERMINAL + 8.0f);
    f32 antes = p.vel.y;
    c.pitch = 0;
    passo_movimento(p, c, m, dt);
    VERIFICA(fabsf(p.vel.y - antes) < 2.0f);  // sair do mergulho não dá tranco
    int k = 0;
    for (; k < 600 && p.queda == QUEDA_LIVRE; k++) passo_movimento(p, c, m, dt);
    VERIFICA(p.queda == PLANADOR && p.pos.y < mov::PLANADOR_ALTURA + 2.0f && p.pos.y > 20.0f);
    for (k = 0; k < 900 && p.queda != NO_CHAO_NORMAL; k++) passo_movimento(p, c, m, dt);
    VERIFICA(p.queda == NO_CHAO_NORMAL && p.no_chao && fabsf(p.pos.y) < 0.01f && p.aterrissou);
    VERIFICA(fabsf(p.vel.y + mov::PLANADOR_DESCIDA) < 0.01f || p.vel.y == 0);
    // no ônibus a física não mexe na posição
    p.queda = NO_ONIBUS;
    p.pos = Vec3(5, 120, 5);
    passo_movimento(p, c, m, dt);
    VERIFICA(p.pos.y == 120);
}

TESTE(animador_poses_e_inclinacao_no_ar) {
    Ilha* il = ilha_br();
    if (!il) return;
    Animador a;
    VERIFICA(a.iniciar(il->jogador().animacoes()));
    Personagem p;
    p.queda = QUEDA_LIVRE;
    p.vel = Vec3(0, -20, 0);
    for (int i = 0; i < 60; i++) a.atualizar(p, false, 1.0f / 30);
    VERIFICA(!strcmp(a.estado(), "queda_livre") && a.deitar() > 1.1f && a.deitar() < 1.5f);  // de bruços (~72°)
    p.vel = Vec3(0, -36, 0);
    for (int i = 0; i < 90; i++) a.atualizar(p, false, 1.0f / 30);
    VERIFICA(!strcmp(a.estado(), "mergulho") && a.deitar() > 2.3f);  // de cabeça para baixo
    p.queda = PLANADOR;
    p.vel = Vec3(0, -6.5f, -12);
    for (int i = 0; i < 60; i++) a.atualizar(p, false, 1.0f / 30);
    VERIFICA(!strcmp(a.estado(), "planador") && a.deitar() < 0.4f && a.peso_planador() > 0.9f);
    p.queda = NO_CHAO_NORMAL;
    p.no_chao = true;
    p.vel = Vec3();
    for (int i = 0; i < 60; i++) a.atualizar(p, false, 1.0f / 30);
    VERIFICA(fabsf(a.deitar()) < 0.05f && a.peso_planador() < 0.05f);
}

// A partida inteira jogada pelo roteiro do autoteste (o mesmo do PS2).
TESTE(partida_br_roteiro_do_jogador) {
    Ilha* il = ilha_br();
    if (!il) return;
    RoteiroPartida r;
    r.comecar(*il, 1);
    const PartidaBR& p = il->partida();
    VERIFICA(p.estado() == PART_CONTAGEM && p.total() == 16 && p.vivos() == 16);
    Controle c;
    bool protegido = true;
    for (int s = 0; s < 30 * 120 && !r.terminou(); s++) {
        EstadoControle e;
        r.controle(*il, e);
        c.ler(e);
        c.iniciar_passo();
        il->passo(c);
        // antes do jogo (e nos 3 s de proteção) ninguém leva tiro
        if (!p.dano_ativo()) {
            AlvoCombate al[GrupoBots::MAX_LUTADORES];
            protegido = protegido && il->bots().alvos(&il->bots().bot(0), &il->jogador_mut().lutador(), al) == 0;
        }
    }
    VERIFICA(r.terminou() && r.marcos == RoteiroPartida::TODOS);
    VERIFICA(protegido);
    VERIFICA(p.estado() == PART_JOGO || p.estado() == PART_FIM);
    ResultadoPartida res = p.resultado(Jogador::ID);
    VERIFICA(res.total == 16 && res.colocacao == 16 && !res.venceu && res.tempo_s > 0);
    VERIFICA(!il->jogador().lutador().vivo && il->resultado_visivel());
    VERIFICA(il->partida().tem_tormenta() && il->partida().tick_jogo > 0);
    // "jogar de novo" começa outra partida (contagem, todos vivos)
    EstadoControle e;
    e.modo = ModoControle::ANALOGICO;
    e.botoes = BT_XIS;
    c.ler(e);
    c.iniciar_passo();
    il->passo(c);
    VERIFICA(p.estado() == PART_CONTAGEM && p.vivos() == 16 && il->jogador().lutador().vivo && !il->resultado_visivel());
    VERIFICA(il->semente_partida() == 2);
}

TESTE(tormenta_tira_vida_de_quem_esta_fora) {
    Ilha* il = ilha_br();
    if (!il) return;
    il->comecar_partida(5, true, 1);
    const PartidaBR& p = il->partida();
    Controle c;
    EstadoControle e;
    e.modo = ModoControle::ANALOGICO;
    int s = 0;
    for (; s < 30 * 120 && p.estado() != PART_JOGO; s++) {
        e.botoes = (s & 1) && p.estado() == PART_ONIBUS && p.onibus().portas_abertas(il->passos() + 1) ? BT_XIS : 0;
        c.ler(e);
        c.iniciar_passo();
        il->passo(c);
    }
    VERIFICA(p.estado() == PART_JOGO);
    Lutador& j = il->jogador_mut().lutador();
    for (s = 0; s < 30 * 60 && j.p.queda != NO_CHAO_NORMAL; s++) {
        c.ler(EstadoControle());
        c.iniciar_passo();
        il->passo(c);
    }
    j.comb.vit.escudo = 50;
    int vida = j.comb.vit.vida;
    // fora da zona (além do raio, sobre o mar raso perto da costa): 1 de dano por segundo, só na vida
    for (s = 0; s < 30 * 3; s++) {
        const Tormenta& t = p.tormenta();
        j.p.pos = Vec3(t.cx() + t.raio() + 5, j.p.pos.y, t.cz());
        c.ler(EstadoControle());
        c.iniciar_passo();
        il->passo(c);
    }
    VERIFICA(j.vivo && j.comb.vit.vida >= vida - 3 && j.comb.vit.vida <= vida - 2 && j.comb.vit.escudo == 50);
}

// Critério de aceite: partida BR só de bots (15) termina com vencedor, todos
// saíram do ônibus, a Tormenta nasceu, e o tempo preso fica abaixo de 1%.
TESTE(partida_br_so_de_bots_tem_vencedor_e_preso_abaixo_de_1_por_cento) {
    Ilha* il = ilha_br();
    if (!il) return;
    for (u32 semente = 1; semente <= 3; semente++) {
        il->comecar_partida(semente, false);
        const PartidaBR& p = il->partida();
        GrupoBots& g = il->bots();
        u32 n = 0;
        for (; n < 40u * 60 * 30 && p.estado() != PART_FIM; n++) il->passo_sem_jogador();
        u32 vivo = 0, preso = 0;
        for (int i = 0; i < g.n_bots(); i++) { vivo += g.cerebro(i).est.vivo_ticks; preso += g.cerebro(i).est.preso_ticks; }
        f32 pct = 100.0f * preso / (vivo ? vivo : 1);
        printf("    semente %u: %s venceu em %.1f min (saltaram %d, ejetados %d, Tormenta %d), preso %.2f%%\n", semente,
               p.nome_vencedor(), n / 1800.0f, p.saltaram, p.ejetados, p.mortes_tormenta, pct);
        VERIFICA(p.estado() == PART_FIM && p.vivos() == 1 && p.vencedor() != 0);
        VERIFICA(p.saltaram + p.ejetados == 15 && p.tick_jogo > 0);
        VERIFICA(p.resultado(p.vencedor()).colocacao == 1 && p.resultado(p.vencedor()).venceu);
        VERIFICA(pct < 1.0f);
        // colocações de 1 a 15, sem repetir
        int vistos = 0;
        for (int i = 0; i < g.n_bots(); i++) {
            const Participante* q = p.participante(g.bot(i).id);
            if (q && q->colocacao >= 1 && q->colocacao <= 15) vistos |= 1 << q->colocacao;
        }
        VERIFICA(vistos == 0xFFFE);
    }
}

// Custo no EE: um destino fora da componente da origem (telhado sem escada, água) falha na
// hora, sem gastar as 7.000 expansões do A*.
TESTE(navegacao_componentes_recusam_destino_sem_ligacao_na_hora) {
    Ilha* il = ilha_br();
    if (!il) return;
    Navegacao& nav = il->navegacao();
    int principal = nav.componente_principal();
    VERIFICA(nav.n_componentes() > 1 && principal > 0);
    int fora = -1, nos_principal = 0;
    for (int i = 0; i < nav.n_nos(); i++) {
        if (nav.componente(i) == principal) nos_principal++;
        if (fora < 0 && i >= Navegacao::N * Navegacao::N && nav.componente(i) != principal) fora = i;
    }
    VERIFICA(nos_principal > 10000 && fora >= 0);
    // os pontos de início e o prédio com escada estão na principal
    for (int s = 0; s < il->mapa().n_spawns(); s++) VERIFICA(nav.componente(nav.no_de(il->mapa().spawn(s))) == principal);
    VERIFICA(nav.componente(nav.no_de(Vec3(-122, 8.1f, 70))) == principal);
    // um andar sem escada: recusa imediata
    Navegacao::Caminho c;
    u32 exp = nav.expansoes, recusas = nav.sem_ligacao;
    VERIFICA(!nav.caminho(il->mapa().spawn(0), nav.posicao(fora), nullptr, 0, c));
    VERIFICA(nav.expansoes == exp && nav.sem_ligacao == recusas + 1);
}
