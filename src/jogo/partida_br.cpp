#include "partida_br.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "grupo_bots.h"

namespace {
// Terra para o ônibus (is_land do Ventania: relevo acima de 0,4 m).
bool terra(const void* ctx, f32 x, f32 z) {
    const MapaBrisa* m = static_cast<const MapaBrisa*>(ctx);
    return m->relevo().altura(x, z) > 0.4f;
}
// Centro de zona aceitável (_zona_valida): em terra e longe o bastante do mar.
bool zona_valida(const void* ctx, f32 x, f32 z, f32 r) {
    const MapaBrisa* m = static_cast<const MapaBrisa*>(ctx);
    if (m->relevo().altura(x, z) <= 1.0f) return false;
    f32 dentro = m->raio_costa(atan2f(z, x)) - sqrtf(x * x + z * z);  // inland()
    f32 minimo = r * 0.35f < 60.0f ? r * 0.35f : 60.0f;
    return dentro >= minimo + 8.0f;
}
const f32 LIMITE_MAPA = 296.0f;  // limit() do Ventania: COSTA_MAX + 8
}  // namespace

bool PartidaBR::iniciar(const Pacote& pak, const MapaBrisa* mapa, char* erro, int ne) {
    mapa_ = mapa;
    u32 t = 0;
    char e[96];
    const u8* d = pak.arquivo("dados/partida.bdat", &t);
    if (!d || !cfg_.carregar(d, t, e, sizeof e)) { snprintf(erro, ne, "partida: %s", d ? e : "dados/partida.bdat ausente"); return false; }
    estado_ = PART_FORA;
    return true;
}

void PartidaBR::comecar(u32 semente, Lutador** l, int n, u32 tick, Loot& loot, GrupoBots* grupo) {
    semente_ = semente;
    grupo_ = grupo;
    n_ = n > MAX ? MAX : n;
    n_vivos_ = n_;
    vencedor_ = 0;
    saltaram = ejetados = mortes_tormenta = 0;
    tick_jogo = 0;
    loot_ = &loot;
    for (int i = 0; i < n_; i++) {
        lut_[i] = l[i];
        Participante& p = part_[i];
        p = Participante();
        p.id = l[i]->id;
        snprintf(p.nome, sizeof p.nome, "%s", l[i]->nome);
        p.inicio = tick;
        p.assento = i;
        p.bot = l[i]->id != 1;
        l[i]->vivo = true;
        l[i]->p.queda = NO_CHAO_NORMAL;
    }
    // O ônibus sai ao fim da contagem; a rota já é sorteada (aparece no mapa).
    onibus_.sortear(semente, tick + static_cast<u32>(ticks(cfg_.contagem_s)), LIMITE_MAPA, terra, mapa_);
    mudar(PART_CONTAGEM);
}

Lutador* PartidaBR::lutador(u16 id) const {
    for (int i = 0; i < n_; i++)
        if (lut_[i]->id == id) return lut_[i];
    return nullptr;
}

const Participante* PartidaBR::participante(u16 id) const {
    for (int i = 0; i < n_; i++)
        if (part_[i].id == id) return &part_[i];
    return nullptr;
}

bool PartidaBR::vivo(u16 id) const {
    const Participante* p = participante(id);
    return p && p->colocacao == 0 && estado_ != PART_FORA;
}

bool PartidaBR::dano_ativo() const { return estado_ == PART_JOGO && t_ >= ticks(cfg_.protecao_inicio_s); }

const char* PartidaBR::nome_vencedor() const {
    const Participante* p = participante(vencedor_);
    return p ? p->nome : "";
}

// Posição de um jogador dentro do ônibus (assentos ao longo do corredor).
Vec3 PartidaBR::assento(int k, u32 tick) const {
    return onibus_.posicao(tick) + onibus_.direcao() * ((k % 6) - 2.5f) * 1.2f + Vec3(0, 0.6f, 0);
}

void PartidaBR::saltar(Lutador& l, u32 tick) {
    l.p.queda = QUEDA_LIVRE;
    l.p.pos = onibus_.posicao(tick) + Vec3(0, -3.0f, 0);
    l.p.vel = onibus_.direcao() * 6.0f + Vec3(0, -2.0f, 0);
    l.p.no_chao = false;
}

int PartidaBR::ticks_restantes(u32 tick) const {
    int r = 0;
    switch (estado_) {
    case PART_CONTAGEM: r = ticks(cfg_.contagem_s) - t_; break;
    case PART_ONIBUS: r = static_cast<int>(onibus_.fecha) - static_cast<int>(tick); break;
    case PART_JOGO: r = tormenta_.ticks_restantes(); break;
    case PART_FIM: r = ticks(cfg_.fim_s) - t_; break;
    default: break;
    }
    return r < 0 ? 0 : r;
}

void PartidaBR::passo(u32 tick) {
    if (estado_ == PART_FORA) return;
    t_++;
    switch (estado_) {
    case PART_CONTAGEM:
        if (t_ >= ticks(cfg_.contagem_s)) {
            // Loot novo e todos no ônibus só com a picareta, vida cheia, sem escudo
            // (_iniciar: loot.reset, Inventory.new, vit.reset).
            if (loot_) loot_->reiniciar(semente_ * 2654435761u + 17u);
            for (int i = 0; i < n_; i++) {
                lut_[i]->comb.iniciar(&lut_[i]->rec()->itens, lut_[i]->id);
                lut_[i]->stats = EstatisticasLutador();
                part_[i].inicio = tick;
                lut_[i]->p.queda = NO_ONIBUS;
                lut_[i]->p.vel = Vec3();
                lut_[i]->p.pos = assento(part_[i].assento, tick);
            }
            mudar(PART_ONIBUS);
        }
        break;
    case PART_ONIBUS: {
        int ainda = 0;
        for (int i = 0; i < n_; i++) {
            Lutador& l = *lut_[i];
            if (!l.vivo || l.p.queda != NO_ONIBUS) continue;
            if (onibus_.terminou(tick)) {
                saltar(l, tick);  // portas fechando: ejetado, ainda sobre a ilha
                ejetados++;
            } else if (l.quer_saltar && onibus_.portas_abertas(tick)) {
                saltar(l, tick);
                saltaram++;
            } else {
                l.p.pos = assento(part_[i].assento, tick);
                l.p.vel = onibus_.direcao() * Onibus::VELOCIDADE;
                l.p.yaw = onibus_.yaw();
                ainda++;
            }
            l.quer_saltar = false;
        }
        if (ainda == 0 && onibus_.terminou(tick)) {
            // O ônibus saiu da ilha: a Tormenta começa agora (fase 1: zona marcada + espera).
            tormenta_.iniciar(cfg_, onibus_.tick0 * 31u + semente_, zona_valida, mapa_);
            tick_jogo = tick;
            mudar(PART_JOGO);
        }
        break;
    }
    case PART_JOGO:
        tormenta_.passo();
        if (t_ % 30 == 0) dano_tormenta(tick);
        checar_vitoria(tick);
        break;
    default:
        break;
    }
}

// A cada segundo: quem está fora da zona perde vida (ignora o escudo). Se o dano
// eliminaria todos os que restam, o de mais vida sobrevive.
void PartidaBR::dano_tormenta(u32 tick) {
    int dano = tormenta_.dano_por_segundo();
    Lutador* fora[MAX];
    int n = 0;
    for (int i = 0; i < n_; i++)
        if (lut_[i]->vivo && part_[i].colocacao == 0 && tormenta_.fora(lut_[i]->p.pos)) fora[n++] = lut_[i];
    for (int a = 1; a < n; a++)  // menos vida primeiro (ordem estável)
        for (int b = a; b > 0 && fora[b]->comb.vit.vida < fora[b - 1]->comb.vit.vida; b--) {
            Lutador* x = fora[b]; fora[b] = fora[b - 1]; fora[b - 1] = x;
        }
    for (int k = 0; k < n; k++) {
        if (n_ >= 2 && n_vivos_ <= 1) break;  // o último que resta é o vencedor
        if (fora[k]->comb.vit.dano_tormenta(dano)) {
            mortes_tormenta++;
            if (grupo_) grupo_->eliminar_pela_tormenta(*fora[k], tick);
            else eliminado(fora[k]->id, fora[k]->id, tick);
        }
    }
}

void PartidaBR::eliminado(u16 vitima, u16 quem, u32 tick) {
    if (estado_ != PART_JOGO && estado_ != PART_ONIBUS) return;
    for (int i = 0; i < n_; i++) {
        Participante& p = part_[i];
        if (p.id != vitima || p.colocacao != 0) continue;
        p.colocacao = n_vivos_;
        p.fim = tick;
        n_vivos_--;
    }
    if (quem != vitima)
        for (int i = 0; i < n_; i++)
            if (part_[i].id == quem) part_[i].eliminacoes++;
}

void PartidaBR::checar_vitoria(u32 tick) {
    if (estado_ != PART_JOGO) return;
    bool acabou = (n_ >= 2 && n_vivos_ <= 1) || (n_ < 2 && (n_vivos_ == 0 || tormenta_.terminou));
    if (!acabou) return;
    vencedor_ = 0;
    for (int i = 0; i < n_; i++)
        if (part_[i].colocacao == 0) {
            part_[i].colocacao = 1;
            part_[i].fim = tick;
            vencedor_ = part_[i].id;
        }
    mudar(PART_FIM);
}

void PartidaBR::zona_segura(f32* cx, f32* cz, f32* r) const {
    if (estado_ < PART_JOGO) { *cx = 0; *cz = 0; *r = 1e6f; return; }
    const Tormenta& t = tormenta_;
    if (t.fechando || t.ticks_restantes() < 30 * 25 || t.terminou) {
        *cx = t.para_x; *cz = t.para_z; *r = t.r_para;
    } else {
        *cx = t.cx(); *cz = t.cz(); *r = t.raio();
    }
}

ResultadoPartida PartidaBR::resultado(u16 id) const {
    ResultadoPartida r;
    r.total = n_;
    snprintf(r.vencedor, sizeof r.vencedor, "%s", nome_vencedor());
    const Participante* p = participante(id);
    const Lutador* l = lutador(id);
    if (!p || !l) return r;
    r.colocacao = p->colocacao;
    r.eliminacoes = p->eliminacoes;
    u32 fim = p->fim ? p->fim : p->inicio;
    r.tempo_s = static_cast<int>((fim - p->inicio) / 30);
    r.dano = l->stats.dano;
    r.baus = l->stats.baus;
    r.curas = l->stats.curas;
    r.venceu = vencedor_ == id;
    return r;
}
