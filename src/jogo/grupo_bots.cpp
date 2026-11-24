#include "grupo_bots.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "partida_br.h"
#include "plataforma/plataforma.h"

bool GrupoBots::iniciar(const Pacote& pak, const RecursosLutador* rec, const MapaBrisa* mapa, Navegacao* nav, char* erro, int ne) {
    rec_ = rec;
    mapa_ = mapa;
    nav_ = nav;
    u32 t = 0;
    char e[96];
    const u8* d = pak.arquivo("dados/bots.bdat", &t);
    if (!d || !cfg_.carregar(d, t, e, sizeof e)) { snprintf(erro, ne, "bots: %s", d ? e : "dados/bots.bdat ausente"); return false; }
    n_bots_ = 0;
    n_feed_ = 0;
    eliminacoes_ = spawns_ = 0;
    return true;
}

Vec3 GrupoBots::ponto_inicio(u32 k) const {
    Vec3 s = mapa_->spawn(static_cast<int>(k % static_cast<u32>(mapa_->n_spawns())));
    s.y = mapa_->relevo().altura(s.x, s.z) + 0.05f;
    return s;
}

void GrupoBots::povoar(int n, u32 semente, int primeiro) {
    Aleatorio r;
    r.semear(semente);
    if (n > MAX_BOTS) n = MAX_BOTS;
    n_bots_ = n;
    spawns_ = static_cast<u32>(primeiro);
    n_feed_ = 0;
    eliminacoes_ = 0;
    quem_eliminou_jogador = 0;
    nome_quem_eliminou[0] = 0;
    // Nomes sorteados da lista do Ventania, sem repetir (a dificuldade não aparece no nome).
    int usados[MAX_BOTS];
    for (int i = 0; i < n; i++) {
        int k = 0;
        for (int tentativa = 0; tentativa < 50; tentativa++) {
            k = r.ate(cfg_.n_nomes());
            bool repetido = false;
            for (int j = 0; j < i; j++) repetido = repetido || usados[j] == k;
            if (!repetido) break;
        }
        usados[i] = k;
        u16 id = static_cast<u16>(2 + i);
        int traje = rec_->traje_por_id(cfg_.traje(r.ate(cfg_.n_trajes())));
        lut_[i].iniciar(rec_, id, traje, cfg_.nome(k));
        lut_[i].reposicionar(ponto_inicio(spawns_++), r.unitario() * 6.2831853f);
        const DificuldadeBot* dif = &cfg_.dificuldade(cfg_.mistura(i));
        cer_[i].iniciar(dif, id * 7919u + semente, &lut_[i]);
    }
}

int GrupoBots::vivos(const Lutador* j) const {
    int n = j && j->vivo ? 1 : 0;
    for (int i = 0; i < n_bots_; i++) n += lut_[i].vivo;
    return n;
}

int GrupoBots::alvos(const Lutador* a, Lutador* j, AlvoCombate* s) {
    int n = 0;
    if (partida && partida->ativa() && !partida->dano_ativo()) return 0;  // contagem, ônibus, proteção
    if (j && j != a && j->vivo) s[n++] = j->como_alvo();
    for (int i = 0; i < n_bots_; i++)
        if (&lut_[i] != a && lut_[i].vivo) s[n++] = lut_[i].como_alvo();
    return n;
}

void GrupoBots::pensar(Lutador* j, const MundoColisao& m, const Loot& loot, u32 tick) {
    u64 t0 = plataforma::tempo_us();
    int n = 0;
    if (j) todos_[n++] = j;
    for (int i = 0; i < n_bots_; i++) todos_[n++] = &lut_[i];
    ctx_.tick = tick;
    ctx_.lutadores = todos_;
    ctx_.n = n;
    ctx_.mundo = &m;
    ctx_.nav = nav_;
    ctx_.loot = &loot;
    ctx_.cfg = &cfg_;
    ctx_.raios = cfg_.raios_visao_por_tick;
    ctx_.planejamentos = 2;
    ctx_.partida = partida;
    ctx_.mapa = mapa_;
    ctx_.tem_humano = j && j->vivo;
    if (j) ctx_.humano = j->p.pos;
    for (int i = 0; i < n_bots_; i++) cmd_[i] = lut_[i].vivo ? cer_[i].pensar(ctx_) : ComandoLutador();
    us_pensar = static_cast<u32>(plataforma::tempo_us() - t0);
}

void GrupoBots::simular(Lutador* j, const MundoColisao& m, Loot& loot, u32 tick) {
    u64 t1 = plataforma::tempo_us();
    loot_ = &loot;
    AlvoCombate al[MAX_LUTADORES];
    for (int i = 0; i < n_bots_; i++) {
        ev_[i] = EventosCombate();
        if (!lut_[i].vivo) continue;
        int na = alvos(&lut_[i], j, al);
        lut_[i].passo(cmd_[i], m, loot, al, na, tick, ev_[i]);
    }
    for (int i = 0; i < n_bots_; i++) processar(lut_[i], ev_[i], j, loot, tick);
    // Reaparecer (fora da partida BR, como no treino do Ventania).
    if (reaparecer)
        for (int i = 0; i < n_bots_; i++)
            if (!lut_[i].vivo && tick >= lut_[i].reviver) reviver(lut_[i], tick);
    us_simular = static_cast<u32>(plataforma::tempo_us() - t1);
}

void GrupoBots::processar(Lutador& a, const EventosCombate& ev, Lutador* j, Loot& loot, u32 tick) {
    // tiro: os bots perto ouvem (só a posição)
    if (ev.atirou)
        for (int i = 0; i < n_bots_; i++) {
            if (&lut_[i] == &a || !lut_[i].vivo) continue;
            Vec3 d = lut_[i].p.pos - a.p.pos;
            if (ponto(d, d) < cfg_.audicao_tiro_m * cfg_.audicao_tiro_m) cer_[i].ouviu(a.id, a.p.pos, tick);
        }
    int arma = a.comb.inv.atual().item;
    for (int k = 0; k < ev.n_acertos; k++) {
        const EventosCombate::Acerto& h = ev.acertos[k];
        Lutador* v = nullptr;
        int vb = -1;
        if (j && h.alvo == j->id) v = j;
        for (int i = 0; i < n_bots_; i++)
            if (lut_[i].id == h.alvo) { v = &lut_[i]; vb = i; }
        if (!v || !v->vivo) continue;
        if (vb >= 0) cer_[vb].levou_dano(a.id, a.p.pos, ctx_);
        if (h.eliminou) eliminar(*v, a, arma, loot, tick);
    }
}

void GrupoBots::eliminar(Lutador& v, Lutador& quem, int arma, Loot& loot, u32 tick) {
    if (!v.vivo) return;
    v.vivo = false;
    if (&quem != &v) quem.stats.eliminacoes++;
    if (partida) partida->eliminado(v.id, quem.id, tick);
    v.reviver = tick + REAPARECER;
    loot.largar_inventario(v.comb.inv, v.p.pos);  // a picareta fica com ele
    Eliminacao& e = feed_[n_feed_ % FEED];
    snprintf(e.quem, sizeof e.quem, "%s", quem.nome);
    snprintf(e.vitima, sizeof e.vitima, "%s", v.nome);
    e.arma = arma;
    e.tick = tick;
    n_feed_++;
    eliminacoes_++;
    if (v.id == 1) {  // o jogador
        quem_eliminou_jogador = quem.id;
        snprintf(nome_quem_eliminou, sizeof nome_quem_eliminou, "%s", quem.nome);
    }
}

void GrupoBots::eliminar_pela_tormenta(Lutador& v, u32 tick) {
    if (!v.vivo) return;
    v.vivo = false;
    v.reviver = tick + REAPARECER;
    if (loot_) loot_->largar_inventario(v.comb.inv, v.p.pos);
    Eliminacao& e = feed_[n_feed_ % FEED];
    snprintf(e.quem, sizeof e.quem, "TORMENTA");
    snprintf(e.vitima, sizeof e.vitima, "%s", v.nome);
    e.arma = 0;
    e.tick = tick;
    n_feed_++;
    eliminacoes_++;
    if (v.id == 1) {
        quem_eliminou_jogador = v.id;
        snprintf(nome_quem_eliminou, sizeof nome_quem_eliminou, "A TORMENTA");
    }
    if (partida) partida->eliminado(v.id, v.id, tick);
}

void GrupoBots::reviver(Lutador& l, u32 tick) {
    const Cerebro* c = nullptr;
    for (int i = 0; i < n_bots_; i++)
        if (&lut_[i] == &l) c = &cer_[i];
    l.vivo = true;
    l.comb.iniciar(&rec_->itens, l.id);
    l.reposicionar(ponto_inicio(spawns_++), 0);
    if (c) {
        int i = static_cast<int>(c - cer_);
        EstatisticasBot est = cer_[i].est;
        const DificuldadeBot* dif = &cer_[i].dificuldade();
        cer_[i].iniciar(dif, l.id * 7919u + tick, &l);
        cer_[i].est = est;  // as estatísticas somam as vidas
    }
}
