// Etapa 6: itens, inventário, vida/escudo, tiro e loot (portados dos testes do
// Ventania: test_inventario.gd, test_combat.gd, test_loot.gd, test_picareta.gd).
#include <math.h>
#include <string.h>

#include "combate/inventario.h"
#include "combate/itens.h"
#include "combate/loot.h"
#include "combate/tiro.h"
#include "conteudo/pak.h"
#include "plataforma/plataforma.h"
#include "teste.h"

namespace {
const BancoItens* banco() {
    static BancoItens b;
    static bool ok = false, tentou = false;
    if (!tentou) {
        tentou = true;
        u32 t = 0;
        const u8* d = plataforma::conteudo(&t);
        static Pacote p;
        char e[128];
        if (d && p.abrir(d, t, e, sizeof e)) {
            const u8* x = p.arquivo("dados/itens.bdat", &t);
            ok = x && b.carregar(x, t, e, sizeof e);
        }
    }
    return ok ? &b : nullptr;
}
int I(const char* id) { return banco()->indice(id); }
bool perto(f32 a, f32 b, f32 e = 1e-3f) { return fabsf(a - b) <= e; }
}  // namespace

TESTE(itens_carregam_com_os_valores_do_ventania) {
    const BancoItens* b = banco();
    VERIFICA(b != nullptr);
    if (!b) return;
    VERIFICA(b->n_armas() == 5 && b->n_consumiveis() == 4 && b->n_raridades() == 6 && b->n_municoes() == 4);
    // Índices como o ItemDB: 0 vazio, armas, consumíveis, ferramentas.
    VERIFICA(I("rajada") == 1 && I("gaviao") == 5 && I("curativo") == 6 && I("celula_g") == 9 && I("picareta") == 10);
    const ArmaDado& r = b->arma(I("rajada"));
    VERIFICA(perto(r.dano, 30) && perto(r.cadencia, 5.5f) && r.automatica && r.pente == 30 && perto(r.alcance, 150));
    VERIFICA(strcmp(b->municao(r.municao).id, "media") == 0);
    VERIFICA(b->arma(I("trovao")).projeteis == 10 && b->pose(I("gaviao")) == POSE_PRECISAO && b->pose(I("faisca")) == POSE_PISTOLA);
    VERIFICA(perto(b->arma(I("gaviao")).dispersao_mirando, 0) && b->arma(I("rajada")).dispersao_mirando < 0);
    VERIFICA(b->pilha(I("curativo")) == 15 && b->pilha(I("kit_cura")) == 3 && b->pilha(I("rajada")) == 1);
    char n[64];
    b->nome_completo(I("rajada"), 2, n, sizeof n);
    VERIFICA(strcmp(n, "Rifle de Assalto Rajada (Raro)") == 0);
    b->nome_completo(I("gaviao"), -1, n, sizeof n);
    VERIFICA(strcmp(n, "Rifle de Precisão Gavião") == 0);
    VERIFICA(perto(b->dano(I("rajada"), 4), 30 * 1.22f) && perto(b->recarga(I("rajada"), 4), 2.3f * 0.8f));
    VERIFICA(tiro::passos_golpe(*b) == 19 && tiro::passo_impacto(*b) == 8 && perto(tiro::alcance_golpe(*b), 2.4f));
}

TESTE(mitico_nunca_e_sorteado_e_sorteio_e_deterministico) {
    const BancoItens* b = banco();
    if (!b) return;
    Aleatorio r;
    r.semear(7);
    int conta[6] = {};
    for (int i = 0; i < 20000; i++) conta[b->sortear_raridade(r, true)]++;
    VERIFICA(conta[5] == 0);                       // Mítico (peso 0)
    VERIFICA(conta[0] > conta[3] && conta[3] > conta[4] && conta[4] > 0);
    Aleatorio a, c;
    a.semear(42);
    c.semear(42);
    for (int i = 0; i < 100; i++) VERIFICA(b->sortear_arma(a) == b->sortear_arma(c));
}

TESTE(inventario_picareta_permanente_e_ciclo_da_barra) {
    const BancoItens* b = banco();
    if (!b) return;
    Inventario inv;
    inv.iniciar(b);
    VERIFICA(inv.picareta_na_mao() && inv.atual().item == I("picareta"));
    Espaco s;
    VERIFICA(inv.tirar_de(Inventario::PICARETA, 1, &s) == Inventario::NAO_DESCARTAVEL);
    VERIFICA(inv.dividir(Inventario::PICARETA, 1) == Inventario::NAO_DESCARTAVEL);
    VERIFICA(Inventario::proximo_no_ciclo(Inventario::PICARETA, 1) == 0);
    VERIFICA(Inventario::proximo_no_ciclo(4, 1) == Inventario::PICARETA);
    VERIFICA(Inventario::proximo_no_ciclo(Inventario::PICARETA, -1) == 4);
    VERIFICA(Inventario::proximo_no_ciclo(0, -1) == Inventario::PICARETA);
    Espaco tudo[5];
    VERIFICA(inv.largar_tudo(tudo) == 0 && inv.atual().item == I("picareta"));  // a picareta não cai
}

TESTE(inventario_pilhas_cheio_e_troca_sem_perder) {
    const BancoItens* b = banco();
    if (!b) return;
    Inventario inv;
    inv.iniciar(b);
    // consumíveis empilham até o limite e transbordam para outro espaço
    Inventario::Adicao a = inv.adicionar(I("curativo"), 0, 20);
    VERIFICA(a.pegou == 20 && inv.espacos[0].qtd == 15 && inv.espacos[1].qtd == 5);
    for (int i = 0; i < 3; i++) inv.adicionar(I("rajada") + i, 1);
    VERIFICA(inv.livres() == 0);
    // cheio: coleta parcial do que cabe na pilha existente, sem perder o resto
    a = inv.adicionar(I("curativo"), 0, 15);
    VERIFICA(a.pegou == 10 && a.erro == Inventario::OK);
    a = inv.adicionar(I("gaviao"), 3);
    VERIFICA(a.pegou == 0 && a.erro == Inventario::CHEIO);
    // troca explícita: o trocado volta em `largado`
    a = inv.adicionar(I("gaviao"), 3, 1, -1, 2);
    VERIFICA(a.pegou == 1 && a.tem_largado && a.largado.item == I("rajada"));
    VERIFICA(inv.espacos[2].item == I("gaviao") && inv.espacos[2].pente == 1);
    // trocar posições: a seleção acompanha o item
    inv.selecionar(2);
    VERIFICA(inv.trocar(2, 4) == Inventario::OK && inv.selecionado == 4 && inv.espacos[4].item == I("gaviao"));
    // dividir e largar quantidades
    Inventario d;
    d.iniciar(b);
    d.adicionar(I("curativo"), 0, 10);
    VERIFICA(d.dividir(0, 4) == Inventario::OK && d.espacos[0].qtd == 6 && d.espacos[1].qtd == 4);
    VERIFICA(d.dividir(0, 6) == Inventario::INVALIDO);
    Espaco s;
    VERIFICA(d.tirar_de(0, 2, &s) == Inventario::OK && s.qtd == 2 && d.espacos[0].qtd == 4);
    VERIFICA(d.total(I("curativo")) == 8);
}

TESTE(inventario_municao_e_recarga) {
    const BancoItens* b = banco();
    if (!b) return;
    Inventario inv;
    inv.iniciar(b);
    int media = static_cast<int>(b->arma(I("rajada")).municao);
    VERIFICA(inv.por_municao(media, 500) == 360);  // máximo 360
    inv.adicionar(I("rajada"), 0, 1, 5);
    inv.selecionar(0);
    VERIFICA(inv.recarga_possivel() == 25);
    VERIFICA(inv.concluir_recarga() == 25 && inv.atual().pente == 30 && inv.municao[media] == 335);
    VERIFICA(inv.recarga_possivel() == 0);
}

TESTE(vitais_escudo_primeiro_e_cura_com_limite) {
    Vitais v;
    v.escudo = 30;
    Vitais::Dano d = v.tomar_dano(50);
    VERIFICA(d.no_escudo == 30 && d.na_vida == 20 && v.vida == 80 && v.escudo == 0 && !d.eliminou);
    d = v.tomar_dano(200);
    VERIFICA(d.na_vida == 80 && d.eliminou && !v.vivo());
    Vitais c;
    c.vida = 60;
    VERIFICA(c.curar(15, 75) == 15 && c.curar(15, 75) == 0 && c.vida == 75);
    VERIFICA(c.por_escudo(25, 50) == 25 && c.por_escudo(50, 50) == 25 && c.escudo == 50);
}

TESTE(tiro_mira_hitbox_e_dispersao) {
    const BancoItens* b = banco();
    if (!b) return;
    // o raio passa pelo ombro direito, na altura dos olhos, na direção da câmera
    tiro::Raio r = tiro::mira(Vec3(0, 0, 0), false, 0, 0);
    VERIFICA(perto(r.origem.x, 0.55f) && perto(r.origem.y, 1.55f) && perto(r.dir.z, -1));
    tiro::Raio a = tiro::mira(Vec3(0, 0, 0), true, 3.14159265f / 2, 0);
    VERIFICA(perto(a.origem.y, 1.05f) && perto(a.dir.x, -1, 1e-4f) && perto(a.origem.z, -0.55f, 1e-4f));
    // cabeça, corpo e erro
    Vec3 alvo(0, 0, -10);
    tiro::Acerto h = tiro::testar(Vec3(0, 1.62f, 0), Vec3(0, 0, -1), alvo, false);
    VERIFICA(h.acertou() && h.cabeca && perto(h.t, 9.75f, 0.01f));
    h = tiro::testar(Vec3(0, 0.9f, 0), Vec3(0, 0, -1), alvo, false);
    VERIFICA(h.acertou() && !h.cabeca && perto(h.t, 9.6f, 0.02f));
    VERIFICA(!tiro::testar(Vec3(2, 1, 0), Vec3(0, 0, -1), alvo, false).acertou());
    VERIFICA(!tiro::testar(Vec3(0, 1.62f, 0), Vec3(0, 0, -1), alvo, true).cabeca);  // agachado: cabeça mais baixa
    // dano com raridade e cabeça
    VERIFICA(tiro::dano_projetil(*b, I("rajada"), 0, false) == 30 && tiro::dano_projetil(*b, I("rajada"), 0, true) == 45);
    VERIFICA(tiro::dano_projetil(*b, I("gaviao"), 4, true) == 305);  // 100 × 1,22 × 2,5
    // dispersão: mirando reduz, no ar aumenta; o Gavião mirando é exato
    f32 base = tiro::dispersao(*b, I("rajada"), false, false, false, true);
    VERIFICA(perto(base, 1.6f) && perto(tiro::dispersao(*b, I("rajada"), true, false, false, true), 0.8f));
    VERIFICA(perto(tiro::dispersao(*b, I("rajada"), false, true, false, false), 1.6f * 1.6f * 2.2f));
    VERIFICA(perto(tiro::dispersao(*b, I("gaviao"), true, true, false, false), 0));
    // projéteis determinísticos e dentro do cone
    Vec3 p1[16], p2[16];
    tiro::projeteis(Vec3(0, 0, -1), 6.0f, 10, tiro::semente_tiro(3, 9), p1);
    tiro::projeteis(Vec3(0, 0, -1), 6.0f, 10, tiro::semente_tiro(3, 9), p2);
    for (int i = 0; i < 10; i++) {
        VERIFICA(perto(p1[i].x, p2[i].x, 0) && perto(p1[i].y, p2[i].y, 0));
        VERIFICA(-p1[i].z >= cosf(6.0f * 3.14159265f / 180.0f) - 1e-4f);
    }
}

TESTE(loot_pontos_bau_pegar_trocar_e_municao) {
    const BancoItens* b = banco();
    if (!b) return;
    static Loot l;
    l.iniciar(b, 1234);
    for (int i = 0; i < 40; i++) l.adicionar_ponto(Vec3(i * 10.0f, 0, 0));
    l.sortear_pontos();
    int com_item = 0;
    for (int i = 0; i < l.n_itens(); i++) com_item += l.item(i).id && !l.item(i).reserva();
    VERIFICA(com_item > 20 && com_item < 40);  // ~75%
    // baú: arma + munição dela + consumível + munição qualquer
    Loot k;
    k.iniciar(b, 5);
    k.adicionar_bau(Vec3(0, 0, 0), 0);
    VERIFICA(k.bau_perto(Vec3(1, 0, 1)) == 0 && k.bau_perto(Vec3(3, 0, 0)) < 0);
    k.abrir_bau(0);
    VERIFICA(k.bau(0).aberto && k.bau_perto(Vec3(1, 0, 1)) < 0);
    int armas = 0, cons = 0, mun = 0;
    for (int i = 0; i < k.n_itens(); i++) {
        if (!k.item(i).id) continue;
        armas += b->eh_arma(k.item(i).item);
        cons += b->eh_consumivel(k.item(i).item);
        mun += k.item(i).municao != 0;
    }
    VERIFICA(armas == 1 && cons == 1 && mun == 2);
    VERIFICA(k.mais_perto(Vec3(0, 0, 1.2f)) < 0);  // ainda saltando do baú
    k.atualizar(1, 1.0f);
    int i_arma = k.mais_perto(Vec3(0, 0, 1.2f));
    VERIFICA(i_arma >= 0 && b->eh_arma(k.item(i_arma).item));
    // pegar e trocar sem perder: inventário cheio, troca o espaço 0
    Inventario inv;
    inv.iniciar(b);
    for (int i = 0; i < 5; i++) inv.adicionar(I("faisca"), 0);
    int arma = k.item(i_arma).item;
    int antes = k.contar(I("faisca")) + inv.total(I("faisca"));
    VERIFICA(k.pegar(i_arma, inv, -1, Vec3()) == Inventario::CHEIO);
    VERIFICA(k.pegar(i_arma, inv, 0, Vec3(0, 0, 2)) == Inventario::OK);
    VERIFICA(inv.espacos[0].item == arma);
    VERIFICA(k.contar(I("faisca")) + inv.total(I("faisca")) == antes - (arma == I("faisca") ? 0 : 0));
    // munição automática a até 1,3 m
    Inventario m;
    m.iniciar(b);
    int idx = k.criar_municao(0, Vec3(5, 0, 0), 30);
    VERIFICA(!k.coletar_municao(m, Vec3(7, 0, 0), 0, 0));
    VERIFICA(k.coletar_municao(m, Vec3(5.5f, 0, 0), 0, 0) && m.municao[0] == 30 && k.item(idx).id == 0);
    // quem largou não recoleta na hora
    k.criar_municao(0, Vec3(9, 0, 0), 10, 7, 100);
    VERIFICA(!k.coletar_municao(m, Vec3(9, 0, 0), 7, 120));
    VERIFICA(k.coletar_municao(m, Vec3(9, 0, 0), 7, 100 + Loot::BLOQUEIO_DONO));
    // eliminado: tudo cai (menos a picareta), nada some
    Inventario e;
    e.iniciar(b);
    e.adicionar(I("rajada"), 2);
    e.adicionar(I("curativo"), 0, 7);
    e.por_municao(1, 90);
    int antes_cur = k.contar(I("curativo"));
    k.largar_inventario(e, Vec3(20, 0, 0));
    VERIFICA(k.contar(I("curativo")) == antes_cur + 7 && e.livres() == 5 && e.municao[1] == 0);
    VERIFICA(e.atual().item == I("picareta"));
}

// --- Combatente (combat_server.gd) e o roteiro de combate do campo de treino ---
#include "jogo/combatente.h"
#include "jogo/roteiro_treino.h"
#include "jogo/treino.h"
#include "nucleo/arena.h"

namespace {
struct Cenario {
    MundoColisao* mundo;
    Loot loot;
    Combatente c;
    Personagem p;
    Vitais alvo_vit;
    u32 alvo_ultimo = 0;
    AlvoCombate alvo;
    u32 tick = 0;
    Cenario() {
        mundo = new MundoColisao();
        mundo->limpar(64);
        mundo->definir_chao(true, 0);
        loot.iniciar(banco(), 3);
        c.iniciar(banco(), 1);
        p.pos = Vec3(0, 0, 0);
        p.no_chao = true;
        alvo.id = 60002;
        alvo.pes = Vec3(0.55f, 0, -1.6f);  // na linha do ombro, a 1,6 m
        alvo.vit = &alvo_vit;
        alvo.ultimo_acerto = &alvo_ultimo;
    }
    ~Cenario() { delete mundo; }
    EventosCombate passo(const EntradaCombate& e, bool com_alvo = true) {
        EventosCombate ev;
        c.passo(e, p, *mundo, loot, com_alvo ? &alvo : nullptr, com_alvo ? 1 : 0, ++tick, ev);
        loot.atualizar(tick, 1.0f / 30.0f);
        return ev;
    }
};
}  // namespace

TESTE(picareta_um_impacto_por_golpe_no_quadro_certo) {
    if (!banco()) return;
    Cenario s;
    EntradaCombate e;
    e.atirar = true;
    int impactos = 0, passo_do_impacto = -1;
    for (int i = 1; i <= tiro::passos_golpe(*banco()); i++) {
        EventosCombate ev = s.passo(e);
        if (ev.n_acertos) { impactos += ev.n_acertos; if (passo_do_impacto < 0) passo_do_impacto = i; }
    }
    VERIFICA(impactos == 1 && s.alvo_vit.vida == 80);  // 20 de dano no jogador/boneco
    VERIFICA(passo_do_impacto == tiro::passo_impacto(*banco()) + 1);
    // golpes seguidos alternam o lado (combo A/B) e respeitam a cadência (19 passos)
    VERIFICA(s.c.acao() == ACAO_GOLPE_B || s.c.acao() == ACAO_GOLPE);
    for (int i = 0; i < 19 * 3; i++) s.passo(e);
    VERIFICA(s.c.golpes() == 4 && s.alvo_vit.vida == 20);
    // fora do alcance (2,4 m) não registra
    Cenario f;
    f.alvo.pes = Vec3(0.55f, 0, -4.0f);
    for (int i = 0; i < 20; i++) f.passo(e);
    VERIFICA(f.alvo_vit.vida == 100);
}

TESTE(combatente_tiro_cadencia_recarga_e_troca) {
    if (!banco()) return;
    Cenario s;
    s.c.inv.adicionar(I("rajada"), 0, 1, 3);  // 3 balas no pente
    s.c.inv.por_municao(static_cast<int>(banco()->arma(I("rajada")).municao), 60);
    EntradaCombate e;
    e.pitch = -0.39f;  // no corpo (a cabeça daria 45 por tiro)
    e.ciclo = 1;  // R1: picareta → espaço 1
    s.passo(e);
    VERIFICA(s.c.inv.selecionado == 0);
    e.ciclo = 0;
    e.atirar = true;
    int tiros = 0, primeiro = -1;
    for (int i = 0; i < 30; i++) {
        EventosCombate ev = s.passo(e);
        if (ev.atirou) { tiros++; if (primeiro < 0) primeiro = i; }
    }
    VERIFICA(primeiro >= Combatente::PASSOS_TROCA - 1);  // 0,2 s para sacar
    VERIFICA(tiros == 3);                                // pente acabou
    VERIFICA(s.c.acao() == ACAO_RECARGA);                // e começou a recarregar sozinho
    VERIFICA(s.alvo_vit.vida + s.alvo_vit.escudo == 100 - 90);
    for (int i = 0; i < 80; i++) s.passo(EntradaCombate());
    VERIFICA(s.c.inv.atual().pente == 30 && s.c.inv.municao[banco()->arma(I("rajada")).municao] == 30);  // 60 − 30 (o pente zerou)
    // semi-automática: um tiro por toque
    Cenario p;
    p.c.inv.adicionar(I("faisca"), 0);
    p.c.inv.selecionar(0);
    EntradaCombate g;
    g.atirar = true;
    int t2 = 0;
    for (int i = 0; i < 40; i++) t2 += p.passo(g).atirou;
    VERIFICA(t2 == 1);
    // a parede bloqueia o tiro
    Cenario w;
    w.mundo->adicionar_caixa(Vec3(0.55f, 0, -0.8f), 0, Vec3(-1, 0, -0.1f), Vec3(1, 3, 0.1f));
    w.c.inv.adicionar(I("rajada"), 0);
    w.c.inv.selecionar(0);
    for (int i = 0; i < 20; i++) w.passo(g);
    VERIFICA(w.alvo_vit.vida == 100 && w.alvo_vit.escudo == 0);
}

TESTE(combatente_consumivel_bau_e_pegar) {
    if (!banco()) return;
    Cenario s;
    s.c.inv.adicionar(I("celula_p"), 0, 2);
    s.c.inv.selecionar(0);
    EntradaCombate e;
    e.atirar = true;
    for (int i = 0; i < 61; i++) s.passo(e, false);  // 2 s de uso
    VERIFICA(s.c.vit.escudo == 25 && s.c.inv.espacos[0].qtd == 1);
    // o curativo não serve com a vida acima do limite (75)
    Cenario k;
    k.c.inv.adicionar(I("curativo"), 0, 5);
    k.c.inv.selecionar(0);
    for (int i = 0; i < 100; i++) k.passo(e, false);
    VERIFICA(k.c.inv.espacos[0].qtd == 5 && k.c.acao() != ACAO_USO);
    // baú: segurar ▲ por 15 passos abre; soltar antes não
    Cenario b;
    b.loot.adicionar_bau(Vec3(0, 0, -1.5f), 180);
    EntradaCombate i;
    i.interagir = true;
    for (int n = 0; n < 10; n++) b.passo(i, false);
    b.passo(EntradaCombate(), false);
    VERIFICA(!b.loot.bau(0).aberto);
    int aberto = -1;
    for (int n = 0; n < 15; n++) { EventosCombate ev = b.passo(i, false); if (ev.bau_aberto >= 0) aberto = n; }
    VERIFICA(aberto == 14 && b.loot.bau(0).aberto);
    // pegar: toque em ▲ perto do item (depois de ele pousar)
    for (int n = 0; n < 20; n++) b.passo(EntradaCombate(), false);
    b.p.pos = b.loot.bau(0).pos + Vec3(0, 0, -1.2f);  // onde a arma caiu (frente do baú girado 180°)
    EventosCombate ev = b.passo(i, false);
    VERIFICA(ev.pegou_item > 0 && b.c.inv.livres() == 4);
}

TESTE(treino_roteiro_de_combate) {
    alignas(64) static u8 memoria[2 * 1024 * 1024];
    static Treino t;
    u32 tam = 0;
    const u8* d = plataforma::conteudo(&tam);
    static Pacote pak;
    char e[160];
    if (!d || !pak.abrir(d, tam, e, sizeof e)) return;
    Arena arena(memoria, sizeof memoria);
    VERIFICA(t.iniciar(pak, arena, e, sizeof e));
    Controle ctl;
    auto passo = [&](u16 botoes) {
        EstadoControle s;
        s.modo = ModoControle::ANALOGICO;
        s.botoes = botoes;
        ctl.ler(s);
        ctl.iniciar_passo();
        t.passo(ctl);
    };
    int marca_pegou = -1, marca_atirou = -1, marca_inv = -1, golpes = -1, cons = -1, recarga = -1;
    for (int f = 0; f < N_ROTEIRO_COMBATE; f++) {
        const FaixaCombate& fc = ROTEIRO_COMBATE[f];
        for (u32 k = fc.inicio; k < fc.fim; k++) {
            if (k == fc.inicio && fc.lugar == 1) t.posicionar(Vec3(-6.2f, 0, 4.386f), 30.0f, -15.0f);
            if (k == fc.inicio && fc.lugar == 2) t.posicionar(Vec3(-0.55f, 0, 4), 0, -3.9f);
            if (k == fc.inicio && fc.lugar == 3) t.posicionar(Vec3(-0.55f, 0, -2.2f), 0, -10.0f);
            passo(fc.botoes);
        }
        const Inventario& inv = t.jogador().combate().inv;
        if (!fc.marco) continue;
        if (!strcmp(fc.marco, "bau_aberto")) VERIFICA(t.loot().bau(3).aberto);
        if (!strcmp(fc.marco, "pegou")) marca_pegou = inv.livres();
        if (!strcmp(fc.marco, "atirou")) marca_atirou = t.dano_nos_bonecos();
        if (!strcmp(fc.marco, "recarregou")) recarga = inv.espacos[0].pente;
        if (!strcmp(fc.marco, "consumivel")) cons = t.jogador().combate().vit.escudo + t.jogador().combate().vit.vida;
        if (!strcmp(fc.marco, "golpes")) golpes = static_cast<int>(t.jogador().combate().golpes());
        if (!strcmp(fc.marco, "inventario")) marca_inv = inv.espacos[0].vazio() ? 1 : 0;
    }
    // as mesmas expectativas de tools/emulador/verificar_saida.sh
    VERIFICA(marca_pegou == 3);                   // arma + consumível do baú
    VERIFICA(marca_atirou >= 300);                // 10 tiros da Rajada rara no boneco 2
    VERIFICA(recarga == 30);
    VERIFICA(cons == 125);                        // célula de escudo: 100 de vida + 25 de escudo
    VERIFICA(golpes == 3);
    VERIFICA(marca_inv == 1);                     // largou o espaço 1 pelo inventário
}
