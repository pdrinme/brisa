#include "loot.h"

#include <math.h>

namespace {
f32 dist(Vec3 a, Vec3 b) {
    Vec3 d = a - b;
    return sqrtf(ponto(d, d));
}
}  // namespace

void Loot::iniciar(const BancoItens* b, u32 semente) {
    b_ = b;
    rng_.semear(semente);
    n_itens_ = n_baus_ = n_pontos_ = 0;
    proximo_id_ = 1;
}

void Loot::adicionar_bau(Vec3 pos, f32 rot) {
    if (n_baus_ >= MAX_BAUS) return;
    BauMundo& c = baus_[n_baus_++];
    c = BauMundo();
    c.pos = pos;
    c.rot = rot;
}

void Loot::adicionar_ponto(Vec3 p) {
    if (n_pontos_ < MAX_PONTOS) pontos_[n_pontos_++] = p;
}

void Loot::sortear_pontos() {
    for (int i = 0; i < n_pontos_; i++)
        if (rng_.unitario() < 0.75f) sortear_chao(pontos_[i]);
}

void Loot::sortear_chao(Vec3 p) {
    if (rng_.unitario() < 0.6f) {
        int arma = b_->sortear_arma(rng_);
        criar_item(arma, b_->sortear_raridade(rng_, false), 1, -1, p);
        criar_municao(static_cast<int>(b_->arma(arma).municao), p + Vec3(0.6f, 0, 0.3f));
    } else {
        int c = b_->sortear_consumivel(rng_);
        criar_item(c, 0, static_cast<int>(b_->consumivel(c).quantidade), -1, p);
    }
}

int Loot::criar(const ItemChao& base) {
    int i = 0;
    while (i < n_itens_ && itens_[i].id != 0) i++;
    if (i >= MAX_ITENS) return -1;
    if (i == n_itens_) n_itens_++;
    itens_[i] = base;
    itens_[i].id = proximo_id_;
    proximo_id_ = static_cast<u16>(proximo_id_ % 65000 + 1);
    itens_[i].de = base.pos;
    itens_[i].salto = 1.0f;
    return i;
}

int Loot::criar_item(int item, int r, int qtd, int pente, Vec3 pos, u16 dono, u32 tick) {
    if (!b_->vai_no_espaco(item)) return -1;
    ItemChao l;
    l.item = static_cast<u8>(item);
    l.raridade = static_cast<u8>(r);
    l.qtd = static_cast<u16>(qtd);
    l.pente = static_cast<i16>(pente);
    l.pos = pos;
    l.dono = dono;
    l.ate = dono ? tick + BLOQUEIO_DONO : 0;
    return criar(l);
}

int Loot::criar_municao(int tipo, Vec3 pos, int qtd, u16 dono, u32 tick) {
    ItemChao l;
    l.municao = static_cast<u8>(tipo + 1);
    l.qtd = static_cast<u16>(qtd > 0 ? qtd : static_cast<int>(b_->municao(tipo).caixa));
    l.pos = pos;
    l.dono = dono;
    l.ate = dono ? tick + BLOQUEIO_DONO : 0;
    return criar(l);
}

void Loot::remover(int i) {
    if (i < 0 || i >= n_itens_) return;
    itens_[i].id = 0;
    while (n_itens_ > 0 && itens_[n_itens_ - 1].id == 0) n_itens_--;
}

int Loot::mais_perto(Vec3 pos) const {
    int melhor = -1;
    f32 d = PEGAR_DIST;
    for (int i = 0; i < n_itens_; i++) {
        const ItemChao& l = itens_[i];
        if (l.id == 0 || l.reserva() || l.salto < 1.0f) continue;
        f32 k = dist(pos, l.pos);
        if (k < d) { d = k; melhor = i; }
    }
    return melhor;
}

int Loot::bau_perto(Vec3 pos) const {
    for (int i = 0; i < n_baus_; i++)
        if (!baus_[i].aberto && dist(pos, baus_[i].pos) < BAU_DIST) return i;
    return -1;
}

Inventario::Erro Loot::pegar(int i, Inventario& inv, int troca, Vec3 pos_troca) {
    if (i < 0 || i >= n_itens_ || itens_[i].id == 0) return Inventario::SUMIU;
    ItemChao& l = itens_[i];
    if (l.reserva()) return Inventario::INVALIDO;
    Inventario::Adicao a = inv.adicionar(l.item, l.raridade, l.qtd, l.pente, troca);
    if (a.pegou <= 0) return a.erro;
    if (a.pegou < l.qtd) l.qtd = static_cast<u16>(l.qtd - a.pegou);
    else remover(i);
    if (a.tem_largado)
        criar_item(a.largado.item, a.largado.raridade, a.largado.qtd < 1 ? 1 : a.largado.qtd,
                   b_->eh_arma(a.largado.item) ? a.largado.pente : -1, pos_troca);
    return Inventario::OK;
}

bool Loot::coletar_municao(Inventario& inv, Vec3 pos, u16 jogador, u32 tick) {
    bool mudou = false;
    for (int i = 0; i < n_itens_; i++) {
        ItemChao& l = itens_[i];
        if (l.id == 0 || !l.reserva() || l.salto < 1.0f || dist(pos, l.pos) > MUNICAO_AUTO_DIST) continue;
        if (jogador && l.dono == jogador && tick < l.ate) continue;
        int coube = l.municao ? inv.por_municao(l.municao - 1, l.qtd) : inv.por_material(l.material - 1, l.qtd);
        if (coube <= 0) continue;
        mudou = true;
        if (coube < l.qtd) l.qtd = static_cast<u16>(l.qtd - coube);
        else remover(i);
    }
    return mudou;
}

void Loot::abrir_bau(int i) {
    BauMundo& c = baus_[i];
    if (c.aberto) return;
    c.aberto = true;
    f32 a = c.rot * 3.14159265f / 180.0f;
    Vec3 frente(sinf(a) * 1.2f, 0, cosf(a) * 1.2f);                 // Basis(UP, rot) · (0, 0, 1,2)
    Vec3 lado = normalizar(vetorial(frente, Vec3(0, 1, 0))) * 0.8f;
    Vec3 base = c.pos + frente;
    Vec3 tampa = c.pos + Vec3(0, 0.6f, 0);
    int arma = b_->sortear_arma(rng_);
    int k[4];
    k[0] = criar_item(arma, b_->sortear_raridade(rng_, true), 1, -1, base);
    k[1] = criar_municao(static_cast<int>(b_->arma(arma).municao), base + lado);
    int cons = b_->sortear_consumivel(rng_);
    k[2] = criar_item(cons, 0, static_cast<int>(b_->consumivel(cons).quantidade), -1, base - lado);
    k[3] = criar_municao(rng_.ate(b_->n_municoes()), base + lado * 2.0f);
    for (int j : k)
        if (j >= 0) { itens_[j].de = tampa; itens_[j].salto = 0; }  // saltam do baú
}

void Loot::atualizar(u32 tick, f32 dt) {
    for (int i = 0; i < n_baus_; i++) {
        BauMundo& c = baus_[i];
        f32 alvo = c.aberto ? 1.0f : 0.0f;
        c.tampa += (alvo - c.tampa) * (dt * 6.0f > 1 ? 1 : dt * 6.0f);
        if (!reabastecer || !c.aberto) continue;
        if (c.reabrir == 0) c.reabrir = tick + REABASTECER;
        else if (tick >= c.reabrir) { c.aberto = false; c.reabrir = 0; }
    }
    for (int i = 0; i < n_itens_; i++)
        if (itens_[i].id && itens_[i].salto < 1.0f) {
            itens_[i].salto += dt / 0.45f;  // DURACAO_SALTO do loot_visual.gd
            if (itens_[i].salto > 1.0f) itens_[i].salto = 1.0f;
        }
}

void Loot::largar_inventario(Inventario& inv, Vec3 pos) {
    Espaco saiu[Inventario::ESPACOS];
    int n = inv.largar_tudo(saiu), k = 0;
    for (int i = 0; i < n; i++, k++) {
        f32 a = k * 2 * 3.14159265f / 6.0f;
        criar_item(saiu[i].item, saiu[i].raridade, saiu[i].qtd < 1 ? 1 : saiu[i].qtd,
                   b_->eh_arma(saiu[i].item) ? saiu[i].pente : -1, pos + Vec3(cosf(a), 0.05f, sinf(a)) * 1.2f);
    }
    for (int t = 0; t < b_->n_municoes(); t++)
        if (inv.municao[t] > 0) {
            f32 a = k++ * 2 * 3.14159265f / 6.0f;
            criar_municao(t, pos + Vec3(cosf(a), 0.05f, sinf(a)) * 1.6f, inv.municao[t]);
            inv.municao[t] = 0;
        }
}

int Loot::contar(int item) const {
    int n = 0;
    for (int i = 0; i < n_itens_; i++)
        if (itens_[i].id && itens_[i].item == item) n += itens_[i].qtd;
    return n;
}

int Loot::contar_municao(int tipo) const {
    int n = 0;
    for (int i = 0; i < n_itens_; i++)
        if (itens_[i].id && itens_[i].municao == tipo + 1) n += itens_[i].qtd;
    return n;
}
