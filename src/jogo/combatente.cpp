#include "combatente.h"

#include <math.h>

void Combatente::iniciar(const BancoItens* b, u16 id) {
    b_ = b;
    id_ = id;
    inv.iniciar(b);
    vit = Vitais();
    passos_ = disparo_ = 0;
    cancelar();
    golpe_ant_ = -1;
    combo_ = 0;
    atirar_ant_ = interagir_ant_ = false;
    bau_ = -1;
    passos_bau_ = 0;
    seq_ = golpes_ = 0;
}

int Combatente::acao() const {
    if (recarga_fim_ >= 0) return ACAO_RECARGA;
    if (uso_fim_ >= 0) return ACAO_USO;
    if (golpe_ini_ >= 0) return combo_ == 1 ? ACAO_GOLPE_B : ACAO_GOLPE;
    if (passos_bau_ > 0 && bau_ >= 0) return ACAO_BAU;
    return ACAO_NADA;
}

f32 Combatente::progresso() const {
    f32 p = 0;
    if (recarga_fim_ >= 0 && recarga_total_ > 0) p = 1 - (recarga_fim_ - passos_) / recarga_total_;
    else if (uso_fim_ >= 0 && uso_total_ > 0) p = 1 - (uso_fim_ - passos_) / uso_total_;
    else if (golpe_ini_ >= 0) p = (passos_ - golpe_ini_) / tiro::passos_golpe(*b_);
    else if (passos_bau_ > 0) p = static_cast<f32>(passos_bau_) / Loot::ABRIR_BAU_PASSOS;
    return p < 0 ? 0 : (p > 1 ? 1 : p);
}

bool Combatente::pode_usar(int item) const {
    const ConsumivelDado& c = b_->consumivel(item);
    if (c.vida > 0) return vit.vida < static_cast<int>(c.limite);
    return vit.escudo < static_cast<int>(c.limite);
}

void Combatente::passo(const EntradaCombate& e, const Personagem& p, const MundoColisao& m, Loot& loot,
                       AlvoCombate* alvos, int n, u32 tick, EventosCombate& ev) {
    passos_ += 1;
    // Troca de item: R1/L1 percorrem o ciclo; ▶ vai direto para a picareta.
    int pedido = inv.selecionado;
    if (e.ciclo) pedido = Inventario::proximo_no_ciclo(inv.selecionado, e.ciclo);
    if (e.picareta) pedido = Inventario::PICARETA;
    if (pedido != inv.selecionado && inv.selecionar(pedido)) {
        cancelar();
        if (disparo_ < passos_ + PASSOS_TROCA) disparo_ = passos_ + PASSOS_TROCA;
        ev.trocou = true;
    }
    Espaco& s = inv.atual();
    if (recarga_fim_ >= 0 && passos_ >= recarga_fim_) {
        inv.concluir_recarga();
        recarga_fim_ = -1;
        ev.recarregou = true;
    }
    if (uso_fim_ >= 0 && passos_ >= uso_fim_) {
        // aplica o consumível (_aplicar_consumivel)
        if (b_->eh_consumivel(s.item)) {
            const ConsumivelDado& c = b_->consumivel(s.item);
            if (c.vida > 0) vit.curar(static_cast<int>(c.vida), static_cast<int>(c.limite));
            else vit.por_escudo(static_cast<int>(c.escudo), static_cast<int>(c.limite));
            if (--s.qtd == 0) inv.tirar_espaco(inv.selecionado);
            ev.usou = true;
        }
        uso_fim_ = -1;
    }
    bool fogo = e.atirar;
    if (b_->eh_arma(s.item)) {
        const ArmaDado& w = b_->arma(s.item);
        if (e.recarregar && recarga_fim_ < 0 && inv.recarga_possivel() > 0) {
            recarga_total_ = b_->recarga(s.item, s.raridade) * 30.0f;
            recarga_fim_ = passos_ + recarga_total_;
        }
        bool gatilho = w.automatica ? fogo : (fogo && !atirar_ant_);
        if (gatilho && recarga_fim_ < 0 && passos_ >= disparo_) {
            if (s.pente > 0) {
                s.pente--;
                disparo_ = passos_ + 30.0f / w.cadencia;
                disparar(e, p, m, alvos, n, tick, ev);
            } else if (inv.recarga_possivel() > 0) {
                recarga_total_ = b_->recarga(s.item, s.raridade) * 30.0f;
                recarga_fim_ = passos_ + recarga_total_;
            }
        }
    } else if (b_->eh_consumivel(s.item)) {
        if (fogo && uso_fim_ < 0 && pode_usar(s.item)) {
            uso_total_ = b_->consumivel(s.item).tempo_s * 30.0f;
            uso_fim_ = passos_ + uso_total_;
        }
    } else if (b_->eh_ferramenta(s.item)) {
        // Picareta: golpes enquanto segura; um impacto por golpe, no passo do impacto.
        int ng = tiro::passos_golpe(*b_);
        if (golpe_ini_ >= 0) {
            f32 k = passos_ - golpe_ini_;
            if (!golpe_acertou_ && k >= tiro::passo_impacto(*b_)) {
                golpe_acertou_ = true;
                golpe(e, p, m, alvos, n, tick, ev);
            }
            if (k >= ng) golpe_ini_ = -1;
        }
        if (fogo && golpe_ini_ < 0 && passos_ >= disparo_) {
            bool seguido = golpe_ant_ >= 0 && passos_ - golpe_ant_ <= ng + 1;
            combo_ = seguido ? 1 - combo_ : 0;
            golpe_ini_ = golpe_ant_ = passos_;
            golpe_acertou_ = false;
            disparo_ = passos_ + ng;
            golpes_++;
        }
    }
    interagir(e, p, m, loot, ev);
    if (loot.coletar_municao(inv, p.pos, id_, tick)) ev.municao = true;
    atirar_ant_ = fogo;
    interagir_ant_ = e.interagir;
}

void Combatente::disparar(const EntradaCombate& e, const Personagem& p, const MundoColisao& m, AlvoCombate* alvos, int n,
                          u32 tick, EventosCombate& ev) {
    const Espaco& s = inv.atual();
    const ArmaDado& w = b_->arma(s.item);
    tiro::Raio r = tiro::mira(p.pos, p.agachado, e.yaw, e.pitch);
    bool movendo = sqrtf(p.vel.x * p.vel.x + p.vel.z * p.vel.z) > 0.5f;
    f32 disp = tiro::dispersao(*b_, s.item, e.mirar, movendo, p.agachado, p.no_chao);
    int np = static_cast<int>(w.projeteis) > tiro::MAX_PROJETEIS ? tiro::MAX_PROJETEIS : static_cast<int>(w.projeteis);
    Vec3 dirs[tiro::MAX_PROJETEIS];
    tiro::projeteis(r.dir, disp, np, tiro::semente_tiro(id_, seq_++), dirs);
    struct Soma { int dano = 0; bool cabeca = false; Vec3 pos; };
    Soma soma[16];
    ev.atirou = true;
    ev.tiro_origem = r.origem;
    ev.n_fins = np;
    for (int k = 0; k < np; k++) {
        Vec3 fim = r.origem + dirs[k] * w.alcance;
        f32 t_parede = m.raio(r.origem, fim).t * w.alcance;
        int melhor = -1;
        tiro::Acerto mh;
        for (int a = 0; a < n && a < 16; a++) {
            tiro::Acerto h = tiro::testar(r.origem, dirs[k], alvos[a].pes, alvos[a].agachado);
            if (h.acertou() && h.t <= t_parede && (melhor < 0 || h.t < mh.t)) { melhor = a; mh = h; }
        }
        if (melhor < 0) {
            ev.fins[k] = r.origem + dirs[k] * t_parede;
            ev.fim_acertou[k] = false;
            continue;
        }
        Vec3 pt = r.origem + dirs[k] * mh.t;
        ev.fins[k] = pt;
        ev.fim_acertou[k] = true;
        soma[melhor].dano += tiro::dano_projetil(*b_, s.item, s.raridade, mh.cabeca);
        soma[melhor].cabeca = soma[melhor].cabeca || mh.cabeca;
        soma[melhor].pos = pt;
    }
    for (int a = 0; a < n && a < 16; a++)
        if (soma[a].dano > 0) dano(alvos[a], soma[a].dano, soma[a].cabeca, soma[a].pos, tick, ev);
}

void Combatente::golpe(const EntradaCombate& e, const Personagem& p, const MundoColisao& m, AlvoCombate* alvos, int n, u32 tick,
                       EventosCombate& ev) {
    tiro::Raio r = tiro::mira(p.pos, p.agachado, e.yaw, e.pitch);
    f32 alc = tiro::alcance_golpe(*b_);
    ImpactoRaio mundo = m.raio(r.origem, r.origem + r.dir * alc);
    f32 t_mundo = mundo.t * alc;
    int melhor = -1;
    tiro::Acerto mh;
    for (int a = 0; a < n; a++) {
        tiro::Acerto h = tiro::testar(r.origem, r.dir, alvos[a].pes, alvos[a].agachado);
        if (h.acertou() && h.t <= t_mundo && (melhor < 0 || h.t < mh.t)) { melhor = a; mh = h; }
    }
    if (melhor >= 0) {
        dano(alvos[melhor], static_cast<int>(b_->ferramenta(b_->picareta()).dano_jogador), false, r.origem + r.dir * mh.t, tick, ev);
        return;
    }
    if (mundo.t < 1.0f) {  // acertou o cenário (Etapa 9: dano nas construções e coleta)
        ev.golpe_mundo = true;
        ev.golpe_pos = r.origem + r.dir * t_mundo;
    }
}

void Combatente::dano(AlvoCombate& a, int d, bool cabeca, Vec3 pos, u32 tick, EventosCombate& ev) {
    if (!a.vit) return;
    Vitais::Dano r = a.vit->tomar_dano(d);
    if (a.ultimo_acerto) *a.ultimo_acerto = tick;
    if (ev.n_acertos < EventosCombate::MAX_ACERTOS)
        ev.acertos[ev.n_acertos++] = {pos, d, cabeca, r.no_escudo > 0, r.eliminou, a.id};
}

Vec3 Combatente::onde_largar(const Personagem& p, f32 yaw, const MundoColisao& m) const {
    // 1,6 m à frente (fora da coleta automática); parede no caminho → aos pés.
    Vec3 alvo = p.pos + Vec3(-sinf(yaw), 0, -cosf(yaw)) * 1.6f;
    Vec3 peito = p.pos + Vec3(0, 1, 0);
    if (m.raio(peito, alvo + Vec3(0, 1, 0)).t < 1.0f) alvo = p.pos;
    f32 y = alvo.y;
    if (m.chao(alvo.x, alvo.z, 0.1f, alvo.y + 1.2f, &y)) alvo.y = y + 0.02f;
    return alvo;
}

void Combatente::interagir(const EntradaCombate& e, const Personagem& p, const MundoColisao& m, Loot& loot, EventosCombate& ev) {
    if (!e.interagir) {
        if (bau_ >= 0 && loot.bau(bau_).abrindo == id_) loot.bau_mut(bau_).abrindo = 0;
        bau_ = -1;
        passos_bau_ = 0;
        return;
    }
    int b = loot.bau_perto(p.pos);
    if (b >= 0) {
        BauMundo& c = loot.bau_mut(b);
        if (c.abrindo != 0 && c.abrindo != id_) { passos_bau_ = 0; return; }  // um por vez
        c.abrindo = id_;
        passos_bau_ = b == bau_ ? passos_bau_ + 1 : 1;
        bau_ = b;
        if (passos_bau_ >= Loot::ABRIR_BAU_PASSOS) {
            loot.abrir_bau(b);
            c.abrindo = 0;
            passos_bau_ = 0;
            bau_ = -1;
            ev.bau_aberto = b;
        }
        return;
    }
    bau_ = -1;
    passos_bau_ = 0;
    if (!interagir_ant_) {
        int i = loot.mais_perto(p.pos);
        if (i >= 0) {
            // cheio: troca pelo item NA MÃO (com a picareta na mão, não troca e avisa)
            int troca = inv.picareta_na_mao() ? -1 : inv.selecionado;
            const ItemChao l = loot.item(i);
            Inventario::Erro r = loot.pegar(i, inv, troca, onde_largar(p, e.yaw, m));
            if (r == Inventario::OK) {
                ev.pegou_item = l.item;
                ev.pegou_raridade = l.raridade;
                ev.pegou_qtd = l.qtd;
                if (disparo_ < passos_ + PASSOS_TROCA) disparo_ = passos_ + PASSOS_TROCA;
            } else {
                ev.erro = r;
            }
        }
    }
}
