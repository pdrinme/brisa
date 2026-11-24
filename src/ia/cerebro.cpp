#include "cerebro.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "jogo/partida_br.h"
#include "plataforma/plataforma.h"

namespace {
const int TR = 30;  // passos por segundo (Protocol.TICK_RATE do Ventania)
const f32 PI = 3.14159265f;
const f32 OLHO = 1.55f;

f32 dist2(Vec3 a, Vec3 b) { return sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }
f32 dist3(Vec3 a, Vec3 b) {
    Vec3 d = a - b;
    return sqrtf(ponto(d, d));
}
f32 clampf(f32 v, f32 a, f32 b) { return v < a ? a : (v > b ? b : v); }
f32 mover_ate(f32 v, f32 alvo, f32 passo) { return v < alvo ? fminf(v + passo, alvo) : fmaxf(v - passo, alvo); }
int arred(f32 v) { return static_cast<int>(floorf(v + 0.5f)); }
f32 faixa(Aleatorio& r, f32 a, f32 b) { return a + (b - a) * r.unitario(); }
// Distância preferida de cada arma (bot_cerebro.gd _combater).
f32 distancia_preferida(const char* id) {
    if (!strcmp(id, "trovao")) return 6;
    if (!strcmp(id, "zumbido")) return 13;
    if (!strcmp(id, "faisca")) return 16;
    if (!strcmp(id, "rajada")) return 24;
    if (!strcmp(id, "gaviao")) return 50;
    return 18;
}
const char* const NOMES_ESTADO[N_ESTADOS_BOT] = {"explorar", "combater", "recuar", "cobertura", "perseguir",
                                                 "investigar", "curar", "bau", "saquear", "tormenta", "ordem"};
}  // namespace

bool Cerebro::depurar = false;

const char* nome_estado_bot(int e) { return e >= 0 && e < N_ESTADOS_BOT ? NOMES_ESTADO[e] : "?"; }

void Cerebro::iniciar(const DificuldadeBot* dif, u32 semente, Lutador* eu) {
    *this = Cerebro();
    dif_ = dif;
    eu_ = eu;
    rng_.semear(semente);
}

const Lutador* Cerebro::lutador(const ContextoBots& c, u16 id) const {
    for (int i = 0; i < c.n; i++)
        if (c.lutadores[i]->id == id) return c.lutadores[i];
    return nullptr;
}

bool Cerebro::ignorado(u32 k) const {
    for (int i = 0; i < n_ignorar_ && i < 32; i++)
        if (ignorar_[i] == k) return true;
    return false;
}

void Cerebro::ignorar(u32 k) {
    ignorar_[n_ignorar_ % 32] = k;
    n_ignorar_++;
}

bool Cerebro::visivel_raio(ContextoBots& c, Vec3 a, Vec3 b) {
    c.raios--;
    return c.mundo->raio(a, b).t >= 1.0f;
}

// ---------------- eventos ----------------

void Cerebro::levou_dano(u16 atacante, Vec3 pos, const ContextoBots& c) {
    // quem leva tiro sabe de onde veio: reage, mas ainda precisa VER o atacante
    i32 t = static_cast<i32>(c.tick);
    atacado_por_ = atacante;
    atacado_tick_ = t;
    memoria_pos_ = pos;
    tem_memoria_ = true;
    memoria_tick_ = t;
    if (inimigo_ == 0 || inimigo_ == atacante) {
        inimigo_ = atacante;
        inimigo_pos_ = pos;
        visto_tick_ = t - TR;  // ainda não viu: só sabe a direção
        i32 r = t + arred(dif_->reacao_s * 0.5f * TR);
        if (r > reagir_tick_) reagir_tick_ = r;
    }
    if (t >= prox_decisao_ + 2) prox_decisao_ = t;  // reavalia na hora
}

void Cerebro::ouviu(u16 atirador, Vec3 pos, u32 tick) {
    // ouvir um tiro NÃO revela o atirador: só um ponto para investigar
    if (atirador == eu_->id || rng_.unitario() > dif_->investiga) return;
    ouviu_pos_ = pos;
    tem_ouviu_ = true;
    ouviu_tick_ = static_cast<i32>(tick);
}

// ---------------- passo ----------------

ComandoLutador Cerebro::pensar(ContextoBots& c) {
    Cmd k;
    k.yaw = yaw_;
    k.pitch = pitch_;
    ComandoLutador out;
    if (!eu_->vivo) return out;
    u64 t0 = plataforma::tempo_us();
    i32 t = static_cast<i32>(c.tick);
    if (eu_->p.queda == NO_ONIBUS) {
        no_onibus(c, k);
    } else if (eu_->p.queda == QUEDA_LIVRE || eu_->p.queda == PLANADOR) {
        caindo(c, k);
    } else if (tem_ordem()) {
        if (estado_ != EB_ORDEM || !tem_alvo_ || dist3(alvo_, ordem) > 0.01f) {
            estado_ = EB_ORDEM;
            alvo_ = ordem;
            tem_alvo_ = true;
            precisa_caminho_ = true;
        }
        andar(c, k, true);
    } else if (t >= prox_decisao_) {
        decidir(c);
        bool perto = c.tem_humano && dist3(c.humano, eu_->p.pos) < c.cfg->lod_distancia_m;
        f32 hz = perto ? dif_->decisao_hz : c.cfg->decisao_longe_hz;
        int passo = arred(TR / hz);
        prox_decisao_ = t + (passo < 1 ? 1 : passo) + rng_.ate(3);
        executar(c, k);
    } else {
        executar(c, k);
    }
    if (eu_->p.queda == NO_CHAO_NORMAL) {  // no ônibus e no ar não conta (nem como preso)
        est.ticks_estado[estado_]++;
        est.vivo_ticks++;
    }
    est.tempo_us += static_cast<u32>(plataforma::tempo_us() - t0);
    yaw_ = k.yaw;
    pitch_ = k.pitch;
    out.mov.x = clampf(k.mx, -1, 1);
    out.mov.y = clampf(k.my, -1, 1);
    out.mov.yaw = k.yaw;
    out.mov.pular = k.pular;
    out.mov.correr = k.correr;
    out.comb.yaw = k.yaw;
    out.comb.pitch = k.pitch;
    out.comb.atirar = k.fogo;
    out.comb.mirar = k.mira;
    out.comb.recarregar = k.recarga && !recarga_ant_;  // o Ventania recarrega na borda do botão
    recarga_ant_ = k.recarga;
    out.comb.interagir = k.interagir;
    out.comb.selecionar = k.espaco;
    return out;
}

// ---------------- decisão (utility scoring) ----------------

void Cerebro::decidir(ContextoBots& c) {
    perceber(c);
    i32 t = static_cast<i32>(c.tick);
    Vec3 pos = eu_->p.pos;
    const Inventario& inv = eu_->comb.inv;
    const BancoItens& it = inv.banco();
    bool visivel = inimigo_ != 0 && t - visto_tick_ < TR * 3 / 2;
    f32 dist_ini = inimigo_ ? dist3(pos, inimigo_pos_) : 1e9f;
    int arma = melhor_arma(inimigo_ ? dist_ini : 30.0f);
    f32 vida = static_cast<f32>(eu_->comb.vit.vida + eu_->comb.vit.escudo);
    bool atacado = t - atacado_tick_ < TR * 2;
    f32 notas[N_ESTADOS_BOT];
    for (f32& n : notas) n = -1;
    if (visivel) {
        if (arma >= 0 && dist_ini < dif_->engaja_m) notas[EB_COMBATER] = 0.85f + (atacado ? 0.1f : 0);
        else if (arma < 0 && dist_ini < 5.0f) notas[EB_COMBATER] = 0.7f;  // picareta, de perto
        else if (arma < 0) notas[EB_RECUAR] = 0.75f;                      // sem arma: foge e procura loot
        // cobertura: sob fogo ou recarregando, se a dificuldade sabe usar
        bool recarga = arma >= 0 && inv.espacos[arma].pente <= static_cast<int>(it.arma(inv.espacos[arma].item).pente) / 4;
        if (arma >= 0 && (atacado || recarga) && cobertura_sorteio_ < dif_->usa_cobertura && vida < dif_->cobertura_vida)
            notas[EB_COBERTURA] = (estado_ != EB_COBERTURA || t < cobertura_ate_) ? 0.9f : 0;
        if (vida < dif_->recua_vida && atacado && dist_ini < 35.0f) notas[EB_RECUAR] = 0.93f;  // em desvantagem
    } else if (tem_memoria_ && arma >= 0 && t - memoria_tick_ < dif_->memoria_s * TR) {
        notas[EB_PERSEGUIR] = 0.55f + (atacado ? 0.15f : 0);  // vai até a última posição conhecida
    }
    if (!visivel && tem_ouviu_ && arma >= 0 && t - ouviu_tick_ < TR * 12) notas[EB_INVESTIGAR] = 0.42f;
    int util = consumivel_util();
    if (vida < dif_->cura_abaixo && util >= 0 && !(visivel && dist_ini < 22.0f))
        notas[EB_CURAR] = 0.8f - vida / 400.0f + (estado_ == EB_COBERTURA ? 0.12f : 0);
    f32 urg = urgencia_tormenta(c);
    if (urg > 0) notas[EB_TORMENTA] = 0.45f + urg * 0.55f;
    if (arma < 0 || inv.livres() >= 1) {
        int l = loot_perto(c);
        if (l) {
            loot_alvo_ = l;
            const ItemChao* li = nullptr;
            for (int i = 0; i < c.loot->n_itens(); i++)
                if (c.loot->item(i).id == l) li = &c.loot->item(i);
            notas[EB_SAQUEAR] = arma < 0 ? 0.75f : 0.4f + 0.1f * fminf(li ? valor_loot(*li) : 0, 2.0f);
        }
        int b = bau_perto(c);
        if (b >= 0) {
            bau_alvo_ = b;
            notas[EB_BAU] = arma < 0 ? 0.62f : 0.52f;
        }
    }
    // (coleta com a picareta: Etapa 9, com os materiais)
    notas[EB_EXPLORAR] = 0.2f;
    // inércia: o estado atual ganha um pouco; recuar tem compromisso mínimo de tempo
    if (notas[estado_] >= 0) notas[estado_] += 0.08f;
    if (estado_ == EB_RECUAR && t < recuar_ate_) notas[EB_RECUAR] = fmaxf(notas[EB_RECUAR], 0.95f);
    int melhor = EB_EXPLORAR;
    for (int k = 0; k < N_ESTADOS_BOT; k++)
        if (notas[k] > notas[melhor]) melhor = k;
    if (melhor != estado_) {
        if (melhor == EB_RECUAR) recuar_ate_ = t + arred(TR * 2.5f);
        if (melhor == EB_COBERTURA) {
            cobertura_ate_ = t + arred(TR * faixa(rng_, 1.5f, 3.0f));
            tem_cobertura_ = false;
        }
        if (estado_ == EB_COBERTURA || melhor == EB_COMBATER) cobertura_sorteio_ = rng_.unitario();
        estado_ = melhor;
        tem_alvo_ = false;
        segurar_ = tentando_ = recuperacoes_ = observar_ = 0;
    }
    definir_alvo(c);
}

void Cerebro::perceber(ContextoBots& c) {
    i32 t = static_cast<i32>(c.tick);
    Vec3 meu = eu_->p.pos, olho = meu + Vec3(0, OLHO, 0);
    Vec3 frente(-sinf(yaw_), 0, -cosf(yaw_));
    u16 melhor = 0;
    f32 melhor_d = dif_->alcance_visao_m;
    Vec3 melhor_pos;
    for (int i = 0; i < c.n; i++) {
        const Lutador* o = c.lutadores[i];
        if (o == eu_ || !o->vivo || o->p.queda == NO_ONIBUS) continue;
        Vec3 p = o->p.pos;
        f32 d = dist3(meu, p);
        if (d > melhor_d) continue;
        Vec3 dir = p - meu;
        dir.y = 0;
        f32 l = sqrtf(ponto(dir, dir)), ang = 0;
        if (l > 0.1f) ang = acosf(clampf(ponto(frente, dir * (1.0f / l)), -1, 1)) * 180.0f / PI;
        if (ang > dif_->campo_visao_graus * 0.5f && d > 6.0f && o->id != atacado_por_) continue;
        if (c.raios <= 0) break;
        if (!visivel_raio(c, olho, p + Vec3(0, 1.2f, 0))) continue;
        melhor = o->id;
        melhor_d = d;
        melhor_pos = p;
    }
    if (melhor) {
        if (melhor != inimigo_ || t - visto_tick_ > TR * 2) {
            reagir_tick_ = t + arred(dif_->reacao_s * TR);
            visada_ini_ = t;  // a mira começa a assentar agora
        }
        inimigo_ = melhor;
        inimigo_pos_ = melhor_pos;
        visto_tick_ = t;
        memoria_pos_ = melhor_pos;  // memória: última posição VISTA
        tem_memoria_ = true;
        memoria_tick_ = t;
        tem_ouviu_ = false;
    } else if (inimigo_ && t - visto_tick_ > dif_->memoria_s * TR) {
        inimigo_ = 0;  // esqueceu
    }
    if (inimigo_) {
        const Lutador* o = lutador(c, inimigo_);
        if (!o || !o->vivo) inimigo_ = 0;
    }
}

void Cerebro::definir_alvo(ContextoBots& c) {
    alvo_3d_ = estado_ != EB_RECUAR;
    Vec3 pos = eu_->p.pos;
    switch (estado_) {
    case EB_COMBATER: alvo_ = inimigo_pos_; tem_alvo_ = true; break;
    case EB_SAQUEAR: {
        tem_alvo_ = false;
        for (int i = 0; i < c.loot->n_itens(); i++)
            if (c.loot->item(i).id == loot_alvo_) { alvo_ = c.loot->item(i).pos; tem_alvo_ = true; }
        break;
    }
    case EB_BAU:
        tem_alvo_ = bau_alvo_ >= 0;
        if (tem_alvo_) alvo_ = c.loot->bau(bau_alvo_).pos;
        break;
    case EB_EXPLORAR:
        if (!tem_alvo_ || dist3(pos, alvo_) < 3.0f) { alvo_ = ponto_explorar(c); tem_alvo_ = true; }
        break;
    case EB_TORMENTA: {
        f32 zx, zz, zr;
        zona(c, &zx, &zz, &zr);
        f32 dx = alvo_.x - zx, dz = alvo_.z - zz;
        if (!tem_alvo_ || sqrtf(dx * dx + dz * dz) > zr * 0.6f) {
            f32 ox = faixa(rng_, -1, 1) * zr * 0.35f, oz = faixa(rng_, -1, 1) * zr * 0.35f;
            alvo_ = Vec3(zx + ox, 0, zz + oz);
            alvo_3d_ = false;
        }
        tem_alvo_ = true;
        break;
    }
    case EB_PERSEGUIR: tem_alvo_ = tem_memoria_; alvo_ = memoria_pos_; break;
    case EB_INVESTIGAR: tem_alvo_ = tem_ouviu_; alvo_ = ouviu_pos_; break;
    case EB_COBERTURA:
        if (!tem_cobertura_) { cobertura_ponto_ = achar_cobertura(c); tem_cobertura_ = true; }
        alvo_ = cobertura_ponto_;
        tem_alvo_ = true;
        break;
    case EB_RECUAR: {
        Vec3 fuga = pos - inimigo_pos_;
        fuga.y = 0;
        f32 zx, zz, zr;
        zona(c, &zx, &zz, &zr);
        Vec3 dz = Vec3(zx, 0, zz) - pos;  // foge puxando para o centro da zona segura
        dz.y = 0;
        f32 lf = sqrtf(ponto(fuga, fuga)), lz = sqrtf(ponto(dz, dz));
        Vec3 nz = lz > 1e-3f ? dz * (1.0f / lz) : Vec3(1, 0, 0);
        Vec3 dir = lf > 0.1f ? normalizar(fuga * (0.7f / lf) + nz * 0.3f) : nz;
        tem_alvo_ = false;
        for (int k = 0; k < 6; k++) {
            f32 a = (k / 2) * 0.6f * (k % 2 == 0 ? 1 : -1);
            // Vector3.rotated(UP, a)
            Vec3 r(dir.x * cosf(a) + dir.z * sinf(a), 0, -dir.x * sinf(a) + dir.z * cosf(a));
            Vec3 q = pos + r * 18.0f;
            if (c.mundo->relevo()->altura(q.x, q.z) > 0.8f) { alvo_ = q; tem_alvo_ = true; break; }
        }
        break;
    }
    default: tem_alvo_ = false; break;
    }
    if (tem_alvo_) {
        if (!alvo_3d_) alvo_.y = c.mundo->relevo()->altura(alvo_.x, alvo_.z);
        // só recalcula a rota se o destino mudou de verdade
        if (caminho_.vazio() || !tem_rota_ || dist3(alvo_rota_, alvo_) > 3.0f) {
            precisa_caminho_ = true;
            alvo_rota_ = alvo_;
            tem_rota_ = true;
        }
    }
}

// ---------------- execução ----------------

void Cerebro::executar(ContextoBots& c, Cmd& k) {
    alterna_ = !alterna_;
    i32 t = static_cast<i32>(c.tick);
    Vec3 pos = eu_->p.pos;
    switch (estado_) {
    case EB_COMBATER: combater(c, k); return;
    case EB_CURAR: curar(c, k); return;
    case EB_COBERTURA:
        // chegou na cobertura: recarrega/cura; quem sabe atirar andando revida no caminho
        if (tem_cobertura_ && dist2(pos, cobertura_ponto_) < 0.9f) { na_cobertura(c, k); return; }
        if (dif_->atira_andando > 0.5f && inimigo_ && t - visto_tick_ < 10) disparar(c, k, true);
        break;
    case EB_RECUAR:
        andar(c, k, true);
        if (dif_->atira_andando > 0.5f && inimigo_ && t - visto_tick_ < 10) {
            disparar(c, k, false);
            k.yaw = yaw_disparo_;
        }
        return;
    case EB_PERSEGUIR:
    case EB_INVESTIGAR: {
        bool tem = estado_ == EB_PERSEGUIR ? tem_memoria_ : tem_ouviu_;
        Vec3 p = estado_ == EB_PERSEGUIR ? memoria_pos_ : ouviu_pos_;
        if (!tem || dist2(pos, p) < 1.5f || observar_ > 0) { olhar_em_volta(c, k); return; }
        andar(c, k, true);
        return;
    }
    case EB_SAQUEAR: {
        const ItemChao* l = nullptr;
        for (int i = 0; i < c.loot->n_itens(); i++)
            if (c.loot->item(i).id == loot_alvo_) l = &c.loot->item(i);
        if (!l) { prox_decisao_ = t; return; }
        if (dist2(pos, l->pos) < 1.7f) {
            if (++tentando_ > 45) { ignorar(static_cast<u32>(loot_alvo_)); prox_decisao_ = t; }
            if (alterna_) k.interagir = true;
            return;
        }
        break;
    }
    case EB_BAU:
        if (bau_alvo_ < 0 || c.loot->bau(bau_alvo_).aberto) { prox_decisao_ = t; return; }
        if (dist3(pos, c.loot->bau(bau_alvo_).pos) < Loot::BAU_DIST - 0.5f) {
            k.interagir = true;
            if (++segurar_ > Loot::ABRIR_BAU_PASSOS * 3) { ignorar(0x10000u + static_cast<u32>(bau_alvo_)); prox_decisao_ = t; }
            return;
        }
        break;
    default: break;
    }
    andar(c, k, true);
}

void Cerebro::andar(ContextoBots& c, Cmd& k, bool correr) {
    if (!tem_alvo_) return;
    i32 t = static_cast<i32>(c.tick);
    Vec3 pos = eu_->p.pos;
    if (precisa_caminho_ && c.planejamentos > 0 && c.nav && t >= replanejar_apos_) {
        c.planejamentos--;
        precisa_caminho_ = false;
        int n = 0;  // tira as arestas evitadas que venceram
        for (int i = 0; i < n_evitar_; i++)
            if (evitar_[i].ate >= t) evitar_[n++] = evitar_[i];
        n_evitar_ = n;
        u32 chaves[Navegacao::MAX_EVITAR];
        for (int i = 0; i < n_evitar_; i++) chaves[i] = evitar_[i].chave;
        bool ok = c.nav->caminho(pos, alvo_, chaves, n_evitar_, caminho_);
        est.caminhos++;
        idx_ = 0;
        prog_dist_ = 1e9f;
        prog_idx_ = -1;
        if ((!ok || caminho_.vazio()) && n_evitar_ > 0) {
            // as arestas evitadas fecharam a única saída (sala com uma porta): esquece e replaneja
            n_evitar_ = 0;
            precisa_caminho_ = true;
            caminho_.n = 0;
            return;
        }
        if (!ok || caminho_.vazio()) {
            caminho_.n = 0;
            // Etapa 8 (custo no EE): meio segundo até o próximo A*; enquanto isso anda direto para o
            // alvo, o que tira do grafo quem pousou num telhado ou na água.
            replanejar_apos_ = t + TR / 2;
            if (depurar)
                printf("SEMROTA %s estado=%s pos=%.1f,%.1f,%.1f alvo=%.1f,%.1f,%.1f\n", eu_->nome, nome_estado_bot(estado_), pos.x, pos.y,
                       pos.z, alvo_.x, alvo_.y, alvo_.z);
            if (estado_ == EB_EXPLORAR || estado_ == EB_RECUAR || estado_ == EB_COBERTURA || estado_ == EB_TORMENTA) {
                // destino de movimento sem rota (água, área fechada): escolhe outro, sem "abandono"
                est.sem_rota++;
                tem_alvo_ = tem_rota_ = tem_cobertura_ = false;
                prox_decisao_ = t;
                return;
            }
            if (++sem_caminho_ >= 2) { abandonar(c); return; }
        } else {
            sem_caminho_ = 0;
        }
    }
    // avança os pontos de passagem (porta/escada/rampa: só bem perto)
    Vec3 prox = alvo_;
    while (idx_ < caminho_.n) {
        prox = caminho_.pontos[idx_];
        f32 raio = caminho_.precisos[idx_] ? 0.7f : 1.7f;
        if (dist2(prox, pos) < raio && fabsf(prox.y - pos.y) < 1.9f) { idx_++; continue; }
        break;
    }
    if (idx_ >= caminho_.n) prox = alvo_;
    f32 dx = prox.x - pos.x, dz = prox.z - pos.z, dl = sqrtf(dx * dx + dz * dz);
    if (idx_ >= caminho_.n && dl < 0.6f && fabsf(alvo_.y - pos.y) < 1.9f) { sem_progresso_ = 0; return; }  // chegou
    k.yaw = atan2f(-dx, -dz) + desvio_;
    k.pitch = 0;
    k.my = 1;
    k.correr = correr;
    // desvio local de árvores, postes, mobília (perto de porta/escada vai reto)
    bool preciso = idx_ < caminho_.n && caminho_.precisos[idx_];
    if (c.tick % 4 == eu_->id % 4u && c.raios > 0) {
        Vec3 frente(-sinf(k.yaw), 0, -cosf(k.yaw));
        bool bateu = !visivel_raio(c, pos + Vec3(0, 0.8f, 0), pos + Vec3(0, 0.8f, 0) + frente * 1.8f);
        if (!preciso && bateu) desvio_ = clampf(desvio_ + (eu_->id % 2 == 0 ? 0.6f : -0.6f), -1.4f, 1.4f);
        else desvio_ = mover_ate(desvio_, 0, 0.3f);
    }
    monitorar(c, k, pos, prox);
}

// Monitor de progresso: querendo andar e sem se aproximar do próximo ponto.
// 1,5 s: manobra (recua, pula, passo de lado); 3 s: evita a aresta onde travou e
// recalcula; 3 recuperações no mesmo alvo: desiste dele.
void Cerebro::monitorar(ContextoBots& c, Cmd& k, Vec3 pos, Vec3 prox) {
    i32 t = static_cast<i32>(c.tick);
    if (manobra_ > 0) {
        manobra_--;
        k.mx = manobra_lado_;
        k.my = -0.6f;
        if (manobra_ == 19) k.pular = true;
        return;
    }
    if (t - prog_tick_ < 15) return;
    prog_tick_ = t;
    f32 d = dist2(prox, pos) + fabsf(prox.y - pos.y);
    if (idx_ != prog_idx_ || prog_dist_ - d > 0.35f) {
        prog_dist_ = d;
        prog_idx_ = idx_;
        sem_progresso_ = 0;
        return;
    }
    sem_progresso_ += 15;
    if (sem_progresso_ >= TR * 3) {
        est.preso_ticks += 15;  // > 3 s sem progresso
        if (sem_progresso_ == TR * 3) {
            est.preso_episodios++;
            if (depurar)
                printf("PRESO %s estado=%s pos=%.1f,%.1f,%.1f prox=%.1f,%.1f,%.1f alvo=%.1f,%.1f,%.1f idx=%d/%d preciso=%d\n", eu_->nome,
                       nome_estado_bot(estado_), eu_->p.pos.x, eu_->p.pos.y, eu_->p.pos.z, prox.x, prox.y, prox.z, alvo_.x, alvo_.y, alvo_.z,
                       idx_, caminho_.n, idx_ < caminho_.n ? caminho_.precisos[idx_] : -1);
        }
    }
    if (sem_progresso_ == 45) {
        manobra_ = 20;
        manobra_lado_ = rng_.unitario() < 0.5f ? 1.0f : -1.0f;
        est.manobras++;
    } else if (sem_progresso_ >= 90 && sem_progresso_ % 45 == 0) {
        // travou de novo: evita a aresta atual por 25 s e recalcula
        if (c.nav) {
            int de = c.nav->no_de(pos), para = c.nav->no_de(prox);
            if (de == para) {
                para = -1;
                for (int i = 0; i + 1 < caminho_.n_nos; i++)
                    if (caminho_.nos[i] == de) para = caminho_.nos[i + 1];
            }
            if (de >= 0 && para >= 0 && de != para) {
                if (n_evitar_ >= Navegacao::MAX_EVITAR) n_evitar_ = Navegacao::MAX_EVITAR - 1;
                evitar_[n_evitar_++] = {Navegacao::chave_aresta(de, para), t + TR * 25};
            }
        }
        precisa_caminho_ = true;
        recuperacoes_++;
        est.recuperacoes++;
        if (recuperacoes_ >= 3) abandonar(c);
    }
}

void Cerebro::abandonar(ContextoBots& c) {
    est.abandonos++;
    recuperacoes_ = sem_progresso_ = 0;
    switch (estado_) {
    case EB_SAQUEAR: ignorar(static_cast<u32>(loot_alvo_)); break;
    case EB_BAU: ignorar(0x10000u + static_cast<u32>(bau_alvo_)); break;
    case EB_COMBATER:
    case EB_PERSEGUIR:
    case EB_INVESTIGAR:
        tem_memoria_ = false;
        if (estado_ != EB_COMBATER) inimigo_ = 0;
        break;
    case EB_ORDEM:
        n_evitar_ = 0;
        precisa_caminho_ = true;
        return;
    default: break;
    }
    tem_alvo_ = false;
    caminho_.n = 0;
    prox_decisao_ = static_cast<i32>(c.tick);
}

void Cerebro::combater(ContextoBots& c, Cmd& k) {
    i32 t = static_cast<i32>(c.tick);
    const Lutador* o = lutador(c, inimigo_);
    if (!o || !o->vivo) {
        inimigo_ = 0;
        tem_memoria_ = false;
        prox_decisao_ = t;
        return;
    }
    bool visivel = t - visto_tick_ < TR;
    f32 dist = dist3(eu_->p.pos, o->p.pos);
    int arma = melhor_arma(dist);
    const Inventario& inv = eu_->comb.inv;
    if (arma < 0) {
        // sem arma: picareta de perto, senão vai até ele
        if (dist > 3.0f) {
            alvo_ = o->p.pos;
            tem_alvo_ = true;
            andar(c, k, true);
            return;
        }
        if (!inv.picareta_na_mao()) k.espaco = Inventario::PICARETA;
        mirar(k, o->p.pos + Vec3(0, 1.15f, 0), dif_->erro_mira_graus);
        k.my = 0.6f;
        k.fogo = true;
        return;
    }
    if (!visivel) {  // perdeu de vista: vai até onde viu por último
        alvo_ = memoria_pos_;
        tem_alvo_ = tem_memoria_;
        andar(c, k, true);
        return;
    }
    if (inv.selecionado != arma) { k.espaco = arma; return; }
    // distância preferida da arma: aproxima ou afasta; passos de lado para não ficar parado
    f32 pref = distancia_preferida(inv.banco().id(inv.atual().item));
    k.my = dist > pref * 1.35f ? 0.7f : (dist < pref * 0.6f ? -0.6f : 0.0f);
    k.mx = sinf(t * dif_->ritmo_lateral + eu_->id) * 0.85f;
    disparar(c, k, false);
}

void Cerebro::disparar(ContextoBots& c, Cmd& k, bool andando) {
    i32 t = static_cast<i32>(c.tick);
    const Lutador* o = lutador(c, inimigo_);
    const Inventario& inv = eu_->comb.inv;
    const BancoItens& it = inv.banco();
    const Espaco& s = inv.atual();
    if (!o || !o->vivo || !it.eh_arma(s.item)) return;
    const ArmaDado& w = it.arma(s.item);
    f32 visada = (t - visada_ini_) / static_cast<f32>(TR);
    f32 assenta = dif_->mira_assenta_s;
    f32 vo = sqrtf(ponto(o->p.vel, o->p.vel));
    f32 erro = dif_->erro_mira_graus * (1.0f + 2.0f * clampf(1.0f - visada / assenta, 0, 1)) * (1.0f + clampf(vo / 6.0f, 0, 1)) *
               (andando ? 1.4f : 1.0f);
    f32 antes = k.yaw;
    mirar(k, o->p.pos + Vec3(0, 1.15f, 0), erro);
    yaw_disparo_ = k.yaw;
    if (andando) k.yaw = antes;
    if (s.pente == 0) {
        if (alterna_) k.recarga = true;
        return;
    }
    if (t < reagir_tick_) return;
    f32 dist = dist3(eu_->p.pos, o->p.pos);
    if (dist > 18.0f && w.dispersao_mirando >= 0) k.mira = true;
    if (w.automatica) {
        // rajadas: atira N passos, pausa M (mais longe = rajadas curtas)
        int n = dist < 20.0f ? static_cast<int>(dif_->rajada_ticks) : static_cast<int>(dif_->rajada_ticks) / 2;
        if (++rajada_ > n) rajada_ = -static_cast<int>(dif_->pausa_ticks);
        if (rajada_ > 0) k.fogo = true;
    } else if (alterna_) {
        k.fogo = true;
    }
}

void Cerebro::mirar(Cmd& k, Vec3 p, f32 erro_graus) {
    Vec3 pos = eu_->p.pos;
    Vec3 d = p - (pos + Vec3(0, OLHO, 0));
    f32 yaw = atan2f(-d.x, -d.z), pitch = atan2f(d.y, sqrtf(d.x * d.x + d.z * d.z));
    tiro::Raio r = tiro::mira(pos, eu_->p.agachado, yaw, pitch);
    d = p - r.origem;  // corrige o ombro
    f32 e = erro_graus * PI / 180.0f;
    k.yaw = atan2f(-d.x, -d.z) + faixa(rng_, -e, e);
    k.pitch = atan2f(d.y, sqrtf(d.x * d.x + d.z * d.z)) + faixa(rng_, -e, e) * 0.6f;
}

void Cerebro::curar(ContextoBots& c, Cmd& k) {
    int s = consumivel_util();
    if (s < 0) { prox_decisao_ = static_cast<i32>(c.tick); return; }
    if (eu_->comb.inv.selecionado != s) { k.espaco = s; return; }
    k.fogo = true;  // segura até terminar de usar
}

void Cerebro::na_cobertura(ContextoBots& c, Cmd& k) {
    k.mx = k.my = 0;
    const Inventario& inv = eu_->comb.inv;
    const Espaco& s = inv.atual();
    if (inv.banco().eh_arma(s.item) && s.pente < inv.banco().arma(s.item).pente && alterna_) k.recarga = true;
    else if (eu_->comb.vit.vida + eu_->comb.vit.escudo < dif_->cura_abaixo && consumivel_util() >= 0) curar(c, k);
    Vec3 d = inimigo_pos_ - eu_->p.pos;
    k.yaw = atan2f(-d.x, -d.z);
    if (static_cast<i32>(c.tick) > cobertura_ate_) prox_decisao_ = static_cast<i32>(c.tick);
}

void Cerebro::olhar_em_volta(ContextoBots& c, Cmd& k) {
    if (observar_ == 0) observar_ = TR * 3;
    observar_--;
    k.mx = k.my = 0;
    k.yaw = yaw_ + 0.09f * (eu_->id % 2 == 0 ? 1.0f : -1.0f);
    if (observar_ <= 1) {
        observar_ = 0;
        if (estado_ == EB_PERSEGUIR) { tem_memoria_ = false; inimigo_ = 0; }
        else tem_ouviu_ = false;
        prox_decisao_ = static_cast<i32>(c.tick);
    }
}

// ---------------- apoio ----------------

int Cerebro::melhor_arma(f32 dist) const {
    const Inventario& inv = eu_->comb.inv;
    const BancoItens& it = inv.banco();
    int melhor = -1;
    f32 nota = -1;
    for (int i = 0; i < Inventario::ESPACOS; i++) {
        const Espaco& s = inv.espacos[i];
        if (!it.eh_arma(s.item)) continue;
        const ArmaDado& w = it.arma(s.item);
        if (s.pente == 0 && inv.municao[w.municao] == 0) continue;
        f32 n = 1.0f + s.raridade * 0.1f;
        const char* id = w.id;
        if (!strcmp(id, "trovao")) n += dist < 12.0f ? 1.5f : -1.0f;
        else if (!strcmp(id, "zumbido")) n += dist < 25.0f ? 0.8f : -0.4f;
        else if (!strcmp(id, "rajada")) n += dist >= 10.0f && dist < 70.0f ? 0.8f : 0.2f;
        else if (!strcmp(id, "gaviao")) n += dist > 45.0f ? 1.4f : -0.8f;
        if (n > nota) { nota = n; melhor = i; }
    }
    return melhor;
}

int Cerebro::consumivel_util() const {
    const Inventario& inv = eu_->comb.inv;
    const BancoItens& it = inv.banco();
    const Vitais& v = eu_->comb.vit;
    for (int i = 0; i < Inventario::ESPACOS; i++) {
        const Espaco& s = inv.espacos[i];
        if (!it.eh_consumivel(s.item)) continue;
        const ConsumivelDado& d = it.consumivel(s.item);
        if (d.vida > 0 && v.vida < static_cast<int>(d.limite)) return i;
        if (d.escudo > 0 && v.escudo < static_cast<int>(d.limite)) return i;
    }
    return -1;
}

f32 Cerebro::valor_loot(const ItemChao& l) const {
    const Inventario& inv = eu_->comb.inv;
    const BancoItens& it = inv.banco();
    if (it.eh_arma(l.item)) {
        int melhor_igual = -1, armas = 0;
        for (const Espaco& s : inv.espacos)
            if (it.eh_arma(s.item)) {
                armas++;
                if (s.item == l.item && s.raridade > melhor_igual) melhor_igual = s.raridade;
            }
        if (armas == 0) return 3.0f;
        if (melhor_igual >= 0) return l.raridade <= melhor_igual ? 0.0f : 1.0f + 0.4f * (l.raridade - melhor_igual);
        return inv.livres() > 0 ? 1.2f : 0.0f;
    }
    if (it.eh_consumivel(l.item)) {
        int tem = 0;
        for (const Espaco& s : inv.espacos)
            if (it.eh_consumivel(s.item)) tem += s.qtd;
        return tem < 3 ? 0.9f : 0.2f;
    }
    return 0.3f;
}

int Cerebro::loot_perto(const ContextoBots& c) const {
    Vec3 pos = eu_->p.pos;
    int melhor = 0;
    f32 nota = 0;
    for (int i = 0; i < c.loot->n_itens(); i++) {
        const ItemChao& l = c.loot->item(i);
        if (!l.id || l.reserva() || l.salto < 1.0f || ignorado(l.id)) continue;
        if (eu_->comb.inv.cabe(l.item) <= 0) continue;
        f32 d = dist3(pos, l.pos);
        if (d > 70.0f) continue;
        // valor do item para ESTE inventário, descontada a distância
        f32 v = valor_loot(l);
        if (v <= 0) continue;
        f32 n = v / (1.0f + d / 18.0f);
        if (n > nota) { nota = n; melhor = l.id; }
    }
    return melhor;
}

int Cerebro::bau_perto(const ContextoBots& c) const {
    Vec3 pos = eu_->p.pos;
    int melhor = -1;
    f32 dist = 80.0f;
    for (int i = 0; i < c.loot->n_baus(); i++) {
        const BauMundo& b = c.loot->bau(i);
        if (b.aberto || ignorado(0x10000u + static_cast<u32>(i))) continue;
        f32 d = dist3(pos, b.pos) + fabsf(b.pos.y - pos.y) * 1.5f;  // andares: um pouco mais caro
        if (d < dist) { dist = d; melhor = i; }
    }
    return melhor;
}

Vec3 Cerebro::ponto_explorar(ContextoBots& c) {
    Vec3 pos = eu_->p.pos;
    const Relevo* r = c.mundo->relevo();
    f32 zx, zz, raio_zona;  // a zona que importa (sem partida: a ilha toda)
    zona(c, &zx, &zz, &raio_zona);
    // prefere cômodos de edifícios ainda não explorados, não muito longe
    if (c.nav && c.nav->n_comodos() > 0 && rng_.unitario() < dif_->explora_predios) {
        int melhor = -1;
        f32 nota = 1e9f;
        int nc = c.nav->n_comodos();
        for (int k = 0; k < 24; k++) {
            int i = rng_.ate(nc);
            if (i < 256 && (visitados_[i / 8] & (1 << (i % 8)))) continue;
            Vec3 cc = c.nav->comodo(i);
            f32 ex = cc.x - zx, ez = cc.z - zz;
            if (sqrtf(ex * ex + ez * ez) > raio_zona * 0.9f) continue;
            f32 d = dist3(pos, cc);
            if (d < 4.0f || d > 140.0f) continue;
            if (d < nota) { nota = d; melhor = i; }
        }
        if (melhor >= 0) {
            if (melhor < 256) visitados_[melhor / 8] |= static_cast<u8>(1 << (melhor % 8));
            return c.nav->comodo(melhor);
        }
    }
    for (int i = 0; i < 12; i++) {
        f32 ang = rng_.unitario() * 2 * PI;
        f32 rr = sqrtf(rng_.unitario()) * fminf(raio_zona, 160.0f) * 0.8f;
        f32 x = zx + cosf(ang) * rr, z = zz + sinf(ang) * rr;
        f32 h = r->altura(x, z);
        if (h > 1.0f) return Vec3(x, h, z);
    }
    return Vec3(zx, r->altura(zx, zz), zz);
}

// Ponto de cobertura perto: alcançável a 3–9 m, em que o inimigo NÃO enxerga.
Vec3 Cerebro::achar_cobertura(ContextoBots& c) {
    Vec3 pos = eu_->p.pos;
    const Relevo* r = c.mundo->relevo();
    Vec3 olho_ini = inimigo_pos_ + Vec3(0, OLHO, 0);
    Vec3 melhor;
    bool tem = false;
    f32 nota = 1e9f, base = rng_.unitario() * 2 * PI;
    for (int k = 0; k < 10; k++) {
        if (c.raios <= 1) break;
        f32 ang = base + k * 2 * PI / 10.0f, rr = 3.0f + (k % 3) * 3.0f;
        Vec3 q = pos + Vec3(cosf(ang), 0, sinf(ang)) * rr;
        f32 h = r->altura(q.x, q.z);
        q.y = fabsf(h - pos.y) > 2.5f ? pos.y : h;
        if (!visivel_raio(c, pos + Vec3(0, 0.9f, 0), q + Vec3(0, 0.9f, 0))) { c.raios--; continue; }  // caminho bloqueado
        if (visivel_raio(c, olho_ini, q + Vec3(0, 1.2f, 0))) continue;  // o inimigo veria
        f32 n = rr + dist3(q, inimigo_pos_) * -0.05f;
        if (n < nota) { nota = n; melhor = q; tem = true; }
    }
    if (!tem) {
        // sem cobertura por perto: se afasta de lado
        Vec3 l = vetorial(pos - inimigo_pos_, Vec3(0, 1, 0));
        f32 ll = sqrtf(ponto(l, l));
        l = ll > 1e-3f ? l * ((rng_.unitario() < 0.5f ? 1.0f : -1.0f) / ll) : Vec3(1, 0, 0);
        melhor = pos + l * 6.0f;
        melhor.y = r->altura(melhor.x, melhor.z);
    }
    return melhor;
}

// ---------------- ônibus, queda e Tormenta (Etapa 8) ----------------

void Cerebro::zona(const ContextoBots& c, f32* cx, f32* cz, f32* r) const {
    if (c.partida) { c.partida->zona_segura(cx, cz, r); return; }
    *cx = 0; *cz = 0; *r = 1e6f;
}

// 0 (seguro) .. 1 (urgente): fora da zona que importa, pior longe e se já está na Tormenta.
f32 Cerebro::urgencia_tormenta(const ContextoBots& c) const {
    if (!c.partida || !c.partida->tem_tormenta()) return 0;
    f32 zx, zz, zr;
    zona(c, &zx, &zz, &zr);
    Vec3 p = eu_->p.pos;
    f32 fora = sqrtf((p.x - zx) * (p.x - zx) + (p.z - zz) * (p.z - zz)) - zr * 0.85f;
    if (fora <= 0) return 0;
    bool dentro_agora = !c.partida->tormenta().fora(p);
    return clampf(0.35f + fora / 120.0f + (dentro_agora ? 0 : 0.4f), 0, 1);
}

Vec3 Cerebro::escolher_destino_salto(ContextoBots& c) {
    const MapaBrisa* m = c.mapa;
    f32 r = rng_.unitario();
    if (m && m->n_pois() > 0 && r < 0.65f) {
        const PoiMapa& poi = m->poi(rng_.ate(m->n_pois()));
        f32 ox = faixa(rng_, -1, 1) * poi.raio * 0.6f, oz = faixa(rng_, -1, 1) * poi.raio * 0.6f;
        return Vec3(poi.x + ox, poi.y, poi.z + oz);
    }
    if (c.loot && c.loot->n_baus() > 0) return c.loot->bau(rng_.ate(c.loot->n_baus())).pos;
    return Vec3();
}

// No ônibus: salta perto do destino, ou quando o ônibus começa a se afastar dele.
void Cerebro::no_onibus(ContextoBots& c, Cmd& k) {
    if (!c.partida) return;
    if (!tem_destino_) { destino_salto_ = escolher_destino_salto(c); tem_destino_ = true; }
    const Onibus& o = c.partida->onibus();
    if (!o.portas_abertas(c.tick)) return;
    Vec3 p = o.posicao(c.tick);
    f32 d = sqrtf((p.x - destino_salto_.x) * (p.x - destino_salto_.x) + (p.z - destino_salto_.z) * (p.z - destino_salto_.z));
    bool afastando = d > dist_onibus_ + 0.5f;
    dist_onibus_ = fminf(dist_onibus_, d);
    if (d < 55.0f * (1.0f + dif_->salto_cedo) || (afastando && rng_.unitario() < 0.08f)) k.pular = true;
}

// Caindo / planando: vira para o destino e vai para a frente até chegar em cima dele.
void Cerebro::caindo(ContextoBots& c, Cmd& k) {
    if (!tem_destino_) { destino_salto_ = escolher_destino_salto(c); tem_destino_ = true; }
    Vec3 d = destino_salto_ - eu_->p.pos;
    k.yaw = atan2f(-d.x, -d.z);
    k.pitch = 0;
    k.my = sqrtf(d.x * d.x + d.z * d.z) > 6.0f ? 1.0f : 0.0f;
    k.mx = 0;
    tem_alvo_ = false;  // ao pousar, decide do zero
    prox_decisao_ = static_cast<i32>(c.tick);
}
