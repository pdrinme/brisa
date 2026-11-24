// Etapa 7: bots (bot_cerebro.gd, nav_grade.gd e o papel do game_server.gd do
// Ventania). Grafo de navegação, escadas, decisão de combate, dano no jogador e o
// critério de aceite do GAME_DESIGN §8: partida só de bots termina com vencedor e
// o tempo "preso" fica abaixo de 1% do tempo vivo.
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "conteudo/pak.h"
#include "jogo/ilha.h"
#include "nucleo/controle.h"
#include "plataforma/plataforma.h"
#include "teste.h"

namespace {
Ilha* ilha_bots() {
    alignas(64) static u8 memoria[3 * 1024 * 1024];
    static Ilha i;
    static Pacote pak;
    static bool ok = false, tentou = false;
    if (!tentou) {
        tentou = true;
        u32 t = 0;
        const u8* d = plataforma::conteudo(&t);
        char e[160];
        if (d && pak.abrir(d, t, e, sizeof e)) {
            Arena a(memoria, sizeof memoria);
            ok = i.iniciar(pak, a, e, sizeof e);
            if (!ok) printf("    ilha: %s\n", e);
        }
    }
    return ok ? &i : nullptr;
}

f32 dist_h(Vec3 a, Vec3 b) { return sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }

// Um bot sozinho com destino fixo; devolve a posição final.
Vec3 levar(Ilha* il, Vec3 de, Vec3 para, int passos, u32* preso) {
    il->ativar_bots(1, 1, false, false);
    Lutador& b = il->bots().bot(0);
    b.reposicionar(de, 0);
    il->bots().cerebro(0).ordem = para;
    for (int s = 0; s < passos && (dist_h(b.p.pos, para) > 0.6f || fabsf(b.p.pos.y - para.y) > 0.5f); s++) il->passo_sem_jogador();
    *preso = il->bots().cerebro(0).est.preso_ticks;
    return b.p.pos;
}

void armar(Lutador& l, const char* arma) {
    const BancoItens& b = l.rec()->itens;
    int item = b.indice(arma);
    l.comb.inv.adicionar(item, 2);
    l.comb.inv.por_municao(b.arma(item).municao, 300);
}
}  // namespace

TESTE(navegacao_grafo_do_mapa_e_escadas) {
    Ilha* il = ilha_bots();
    VERIFICA(il != nullptr);
    if (!il) return;
    Navegacao& nav = il->navegacao();
    VERIFICA(nav.n_andares() == 111 && nav.n_ligacoes() == 30 && nav.n_comodos() == 149);
    // fora da ilha é bloqueado; o centro do mapa é chão livre
    VERIFICA(nav.bloqueada(0, 0) && !nav.bloqueada(75, 75));
    // um ponto no andar de cima do prédio cai num nó de andar; no chão, num nó de chão
    VERIFICA(nav.no_de(Vec3(-122, 8.1f, 70)) >= Navegacao::N * Navegacao::N);
    VERIFICA(nav.no_de(Vec3(-130, 4, 62)) < Navegacao::N * Navegacao::N);
    // A* chão → andar passa pela escada (pontos precisos entre os dois pisos), e volta
    Navegacao::Caminho c;
    VERIFICA(nav.caminho(Vec3(-130, 4, 62), Vec3(-122, 8.1f, 70), nullptr, 0, c));
    int degraus = 0;
    for (int i = 0; i < c.n; i++) degraus += c.precisos[i] && c.pontos[i].y > 4.5f && c.pontos[i].y < 7.5f;
    VERIFICA(degraus > 0 && fabsf(c.pontos[c.n - 1].y - 8.1f) < 0.1f);
    VERIFICA(nav.caminho(Vec3(-122, 8.1f, 70), Vec3(-130, 4, 62), nullptr, 0, c) && c.n > 2);
    // no patamar do meio da escada do galpão: a rota começa pelo resto da escada (não pula do patamar)
    VERIFICA(nav.caminho(Vec3(-51.5f, 10.1f, 161), Vec3(-60, 8, 170), nullptr, 0, c));
    VERIFICA(c.n > 1 && c.precisos[0] && c.pontos[0].y > 9.5f);
    // evitar todas as arestas de saída de um nó isolado: sem rota (o cérebro esvazia a lista e replaneja)
    u32 evitar[8];
    int a = nav.no_de(Vec3(2, 0, 2));
    int ci = a % Navegacao::N, cj = a / Navegacao::N, n = 0;
    static const int D[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    for (auto& d : D) evitar[n++] = Navegacao::chave_aresta(a, (cj + d[1]) * Navegacao::N + ci + d[0]);
    Vec3 pa = nav.posicao(a);
    VERIFICA(!nav.caminho(pa, pa + Vec3(40, 0, 0), evitar, n, c));
}

TESTE(bot_sobe_e_desce_escada_com_destino_fixo) {
    Ilha* il = ilha_bots();
    if (!il) return;
    u32 preso = 0;
    // escada empilhada do prédio (lance 2 sob o patamar de cima: cápsula)
    Vec3 f = levar(il, Vec3(-122, 8.1f, 70), Vec3(-130, 4, 62), 600, &preso);
    VERIFICA(dist_h(f, Vec3(-130, 4, 62)) < 1.0f && fabsf(f.y - 4) < 0.5f && preso == 0);
    f = levar(il, Vec3(-130, 4, 62), Vec3(-122, 8.1f, 70), 600, &preso);
    VERIFICA(dist_h(f, Vec3(-122, 8.1f, 70)) < 1.0f && fabsf(f.y - 8.1f) < 0.5f && preso == 0);
    // galpão: do patamar do meio para cima e para baixo
    f = levar(il, Vec3(-51.5f, 10.1f, 161), Vec3(-46, 12.1f, 161), 600, &preso);
    VERIFICA(dist_h(f, Vec3(-46, 12.1f, 161)) < 1.0f && fabsf(f.y - 12.1f) < 0.5f && preso == 0);
    f = levar(il, Vec3(-51.5f, 10.1f, 161), Vec3(-60, 8, 170), 600, &preso);
    VERIFICA(dist_h(f, Vec3(-60, 8, 170)) < 1.0f && fabsf(f.y - 8) < 0.5f && preso == 0);
}

TESTE(bots_nomes_sem_repetir_trajes_e_mistura_de_dificuldades) {
    Ilha* il = ilha_bots();
    if (!il) return;
    il->ativar_bots(15, 7, false, false);
    GrupoBots& g = il->bots();
    const BancoBots& cfg = g.config();
    VERIFICA(g.n_bots() == 15 && cfg.n_dificuldades() == 3 && cfg.n_nomes() == 1000 && cfg.n_trajes() == 2);
    VERIFICA(strcmp(cfg.dificuldade(0).nome, "Fácil") == 0 && strcmp(cfg.dificuldade(2).nome, "Difícil") == 0);
    VERIFICA(cfg.raios_visao_por_tick == 24 && fabsf(cfg.audicao_tiro_m - 70) < 1e-3f);
    // os valores do Ventania: a dificuldade muda reação, mira e percepção
    VERIFICA(cfg.dificuldade(0).reacao_s > cfg.dificuldade(1).reacao_s && cfg.dificuldade(1).reacao_s > cfg.dificuldade(2).reacao_s);
    VERIFICA(cfg.dificuldade(0).erro_mira_graus > cfg.dificuldade(2).erro_mira_graus);
    bool repetido = false;
    int por_dif[3] = {};
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < i; j++) repetido = repetido || strcmp(g.bot(i).nome, g.bot(j).nome) == 0;
        VERIFICA(g.bot(i).id == 2 + i && g.bot(i).vivo);
        const RecursosLutador* r = g.bot(i).rec();
        VERIFICA(g.bot(i).traje == r->traje_por_id("recruta_base_01") || g.bot(i).traje == r->traje_por_id("recruta_brisa_01"));
        int d = static_cast<int>(&g.cerebro(i).dificuldade() - &cfg.dificuldade(0));
        VERIFICA(d == cfg.mistura(i));
        if (d >= 0 && d < 3) por_dif[d]++;
    }
    VERIFICA(!repetido && por_dif[0] > 0 && por_dif[1] > 0 && por_dif[2] > 0);
    VERIFICA(g.vivos(nullptr) == 15);
}

TESTE(bot_que_ve_um_inimigo_entra_em_combate_e_acerta) {
    Ilha* il = ilha_bots();
    if (!il) return;
    il->ativar_bots(2, 3, false, false);
    GrupoBots& g = il->bots();
    const BancoBots& cfg = g.config();
    Lutador &a = g.bot(0), &b = g.bot(1);
    // campo aberto perto do centro, frente a frente a 14 m, os dois Difíceis e armados
    Vec3 pa(2, 0, 2), pb(2, 0, 16);
    pa.y = il->mapa().relevo().altura(pa.x, pa.z) + 0.05f;
    pb.y = il->mapa().relevo().altura(pb.x, pb.z) + 0.05f;
    a.reposicionar(pa, 0);
    b.reposicionar(pb, 3.14159f);
    for (int i = 0; i < 2; i++) g.cerebro(i).iniciar(&cfg.dificuldade(2), 11u + i, &g.bot(i));
    armar(a, "rajada");
    armar(b, "rajada");
    bool combateu = false;
    for (int s = 0; s < 90; s++) {
        il->passo_sem_jogador();
        combateu = combateu || g.cerebro(0).estado() == EB_COMBATER || g.cerebro(1).estado() == EB_COMBATER;
    }
    VERIFICA(combateu && (g.cerebro(0).inimigo() == b.id || g.cerebro(1).inimigo() == a.id));
    for (int s = 0; s < 30 * 30 && a.vivo && b.vivo; s++) il->passo_sem_jogador();
    VERIFICA(!a.vivo || !b.vivo);
    VERIFICA(g.eliminacoes() == 1 && g.n_feed() == 1 && g.vivos(nullptr) == 1);
}

TESTE(jogador_leva_dano_dos_bots_e_reaparece) {
    Ilha* il = ilha_bots();
    if (!il) return;
    il->ativar_bots(1, 5, true, true);
    GrupoBots& g = il->bots();
    Lutador& b = g.bot(0);
    Lutador& j = il->jogador_mut().lutador();
    VERIFICA(j.vivo && j.comb.vit.vida == 100);
    il->ir_para(2, 16, 180);
    Vec3 pb(2, 0, 4);
    pb.y = il->mapa().relevo().altura(pb.x, pb.z) + 0.05f;
    b.reposicionar(pb, 0);
    g.cerebro(0).iniciar(&g.config().dificuldade(2), 99, &b);
    armar(b, "rajada");
    Controle c;
    bool levou = false;
    int s = 0;
    for (; s < 30 * 40 && j.vivo; s++) {
        il->passo(c);
        levou = levou || j.comb.vit.vida < 100;
    }
    VERIFICA(levou && !j.vivo);
    VERIFICA(g.quem_eliminou_jogador == b.id && strcmp(g.nome_quem_eliminou, b.nome) == 0);
    VERIFICA(g.n_feed() >= 1 && strcmp(g.feed(0).vitima, j.nome) == 0);
    // volta em 4 s, com vida cheia
    for (int k = 0; k < static_cast<int>(GrupoBots::REAPARECER) + 2 && !j.vivo; k++) il->passo(c);
    VERIFICA(j.vivo && j.comb.vit.vida == 100);
}

// Critério de aceite (GAME_DESIGN §8): partida só de bots termina com um vencedor e
// os bots ficam presos menos de 1% do tempo vivos. Três sementes, até 25 minutos cada.
TESTE(partida_so_de_bots_tem_vencedor_e_preso_abaixo_de_1_por_cento) {
    Ilha* il = ilha_bots();
    if (!il) return;
    for (u32 semente = 1; semente <= 3; semente++) {
        il->ativar_bots(15, semente, false, false);
        GrupoBots& g = il->bots();
        u32 passos = 0;
        for (; passos < 25u * 60 * 30 && g.vivos(nullptr) > 1; passos++) il->passo_sem_jogador();
        u32 vivo = 0, preso = 0, combate = 0;
        for (int i = 0; i < g.n_bots(); i++) {
            vivo += g.cerebro(i).est.vivo_ticks;
            preso += g.cerebro(i).est.preso_ticks;
            combate += g.cerebro(i).est.ticks_estado[EB_COMBATER];
        }
        f32 pct = 100.0f * static_cast<f32>(preso) / static_cast<f32>(vivo ? vivo : 1);
        printf("    semente %u: vencedor em %.1f min, preso %.2f%%, %u eliminações\n", semente, passos / 1800.0f, pct,
               g.eliminacoes());
        VERIFICA(g.vivos(nullptr) == 1 && g.eliminacoes() == 14);
        VERIFICA(pct < 1.0f && combate > 0);
    }
}
