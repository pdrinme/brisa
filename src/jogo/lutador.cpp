#include "lutador.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "render3d/instancia.h"

namespace {
const char* const TRAJES[RecursosLutador::N_TRAJES] = {"recruta_base_01", "recruta_brisa_01", "capitao_mare_01", "entregador_01",
                                                      "escafandro_01", "faz_tudo_01", "espantalho_01", "explorador_01",
                                                      "botanico_01", "astronomo_01", "guardiao_01"};
}  // namespace

bool RecursosLutador::iniciar(const Pacote& pak, BibliotecaModelos& b, char* erro, int ne) {
    bib = &b;
    char nome[48];
    for (int t = 0; t < N_TRAJES; t++)
        for (int l = 0; l < 3; l++) {
            if (l == 0) snprintf(nome, sizeof nome, "%s", TRAJES[t]);
            else snprintf(nome, sizeof nome, "%s_lod%d", TRAJES[t], l);
            if ((trajes[t][l] = b.carregar(nome, erro, ne)) < 0) return false;
        }
    u32 tam = 0;
    char e[96];
    const u8* d = pak.arquivo("animacoes/humanoide.bani", &tam);
    if (!d || !anims.carregar(d, tam, e, sizeof e)) { snprintf(erro, ne, "animações do personagem: %s", d ? e : "ausente"); return false; }
    d = pak.arquivo("dados/itens.bdat", &tam);
    if (!d || !itens.carregar(d, tam, e, sizeof e)) { snprintf(erro, ne, "itens: %s", d ? e : "dados/itens.bdat ausente"); return false; }
    if (itens.n_itens() > MAX_ITENS) { snprintf(erro, ne, "itens demais (%d)", itens.n_itens()); return false; }
    if ((planador = b.carregar("planador_01", erro, ne)) < 0) return false;
    for (int i = 0; i < MAX_ITENS; i++) modelo_item[i] = -1;
    for (int i = 1; i < itens.n_itens(); i++)
        if ((modelo_item[i] = b.carregar(itens.modelo(i), erro, ne)) < 0) return false;
    return true;
}

int RecursosLutador::traje_por_id(const char* id) const {
    for (int t = 0; t < N_TRAJES; t++)
        if (!strcmp(TRAJES[t], id)) return t;
    return 0;
}

void Lutador::iniciar(const RecursosLutador* r, u16 i, int t, const char* n) {
    r_ = r;
    id = i;
    traje = t;
    snprintf(nome, sizeof nome, "%s", n ? n : "");
    anim.iniciar(r->anims);
    comb.iniciar(&r->itens, i);
    port = Portador();
    stats = EstatisticasLutador();
    vivo = true;
    reposicionar(Vec3(), 0);
}

void Lutador::reposicionar(Vec3 pes, f32 yaw) {
    p = Personagem();
    p.pos = pes;
    p.yaw = yaw;
}

Mat4 Lutador::matriz() const {
    // O modelo olha para +z; o jogo usa −z como frente (como no Ventania).
    return Mat4::translacao(p.pos) * Mat4::rotacao_y(p.yaw + 3.14159265f);
}

AlvoCombate Lutador::como_alvo() {
    AlvoCombate a;
    a.id = id;
    a.pes = p.pos;
    a.agachado = p.agachado;
    a.vit = &comb.vit;
    a.ultimo_acerto = &ultimo_acerto;
    return a;
}

void Lutador::passo(const ComandoLutador& c, const MundoColisao& m, Loot& loot, AlvoCombate* alvos, int n, u32 tick,
                    EventosCombate& ev, bool com_visual) {
    const f32 dt = 1.0f / 30.0f;
    quer_saltar = p.queda == NO_ONIBUS && c.mov.pular;
    passo_movimento(p, c.mov, m, dt);
    anim.atualizar(p, c.acenar, dt);
    // No ônibus e no ar não há combate nem interação (os tempos seguem correndo).
    comb.passo(p.queda == NO_CHAO_NORMAL ? c.comb : EntradaCombate(), p, m, loot, alvos, p.queda == NO_CHAO_NORMAL ? n : 0, tick, ev);
    for (int k = 0; k < ev.n_acertos; k++) stats.dano += ev.acertos[k].dano;
    if (ev.bau_aberto >= 0) stats.baus++;
    if (ev.usou) stats.curas++;
    const BancoItens& it = r_->itens;
    if (ev.atirou) {
        const char* idi = it.id(comb.inv.atual().item);
        port.disparou(!strcmp(idi, "trovao") || !strcmp(idi, "gaviao"));
    }
    if (!com_visual) return;
    int item = comb.inv.atual().item;
    port.pedir_item(item, item > 0 && r_->modelo_item[item] >= 0 ? &(*r_->bib)[r_->modelo_item[item]] : nullptr, it.pose(item), it.id(item));
    Vec3 alcance;
    bool alcancar = false;
    int b = comb.bau_interagindo();
    if (b >= 0) {
        const BauMundo& bau = loot.bau(b);
        Vec3 para_mim = p.pos - bau.pos;
        para_mim.y = 0;
        f32 l = sqrtf(ponto(para_mim, para_mim));
        Vec3 alca = bau.pos + Vec3(0, 0.62f, 0) + (l > 0.01f ? para_mim * (0.35f / l) : Vec3());
        Vec4 q = transformar(inversa_rigida(matriz()), alca);
        alcance = Vec3(q.x, q.y, q.z);
        alcancar = true;
    }
    port.atualizar(dt, c.comb.pitch, c.comb.mirar, sqrtf(p.vel.x * p.vel.x + p.vel.z * p.vel.z), comb.acao(), comb.progresso(),
                   alcancar, alcance);
}

void Lutador::montar(Cena3D& cena, const MundoColisao& m, int lod, bool sombra) {
    static Pose pose;
    lod = lod < 0 ? 0 : (lod > 2 ? 2 : lod);
    const ModeloBrisa& mod = (*r_->bib)[r_->trajes[traje][lod]];
    if (p.queda == NO_ONIBUS) return;  // dentro do ônibus: invisível
    anim.pose(mod, pose);
    Mat4 mm = matriz(), item;
    if (p.queda != NO_CHAO_NORMAL || fabsf(anim.deitar()) > 0.01f || fabsf(anim.rolar()) > 0.01f) {
        // No ar: de bruços / de cabeça para baixo / planando, girando em torno do centro do corpo.
        const Vec3 centro(0, 1.0f, 0);
        mm = Mat4::translacao(p.pos) * Mat4::rotacao_y(p.yaw) * Mat4::translacao(centro) * Mat4::rotacao_x(-anim.deitar()) *
             Mat4::rotacao_z(anim.rolar()) * Mat4::translacao(Vec3() - centro) * Mat4::rotacao_y(3.14159265f);
    }
    if (anim.peso_planador() > 0.05f && r_->planador >= 0) {
        // Asa delta sobre a cabeça: sai do ombro e cresce (tween TRANS_BACK de 0,35–0,4 s do Ventania).
        f32 k = fminf(anim.tempo_planador() / 0.38f, 1.0f);
        f32 e = 1.0f + 2.70158f * (k - 1) * (k - 1) * (k - 1) + 1.70158f * (k - 1) * (k - 1);  // ease out back
        Vec3 pos = Vec3(0, 1.45f, -0.25f) + (Vec3(0, 2.45f, 0) - Vec3(0, 1.45f, -0.25f)) * e;
        f32 esc = 0.08f + (1.0f - 0.08f) * e;
        adicionar_modelo(cena, (*r_->bib)[r_->planador], nullptr, mm * Mat4::translacao(pos) * Mat4::escala(esc));
    }
    int ii = p.queda == NO_CHAO_NORMAL ? port.item_na_mao() : 0;
    if (lod < 2 && port.aplicar(mod, pose, &item) && ii > 0 && r_->modelo_item[ii] >= 0)
        adicionar_modelo(cena, (*r_->bib)[r_->modelo_item[ii]], nullptr, mm * item);
    adicionar_modelo(cena, mod, &pose, mm);
    Vec3 c;
    if (lod < 2 && port.cano(&c)) {
        Vec4 w = transformar(mm, c);
        cano = Vec3(w.x, w.y, w.z);
    } else {
        cano = p.pos + Vec3(0, 1.35f, 0);
    }
    f32 y_chao = 0;
    if (sombra && m.chao(p.pos.x, p.pos.z, 0.1f, p.pos.y + 0.05f, &y_chao)) {
        f32 h = p.pos.y - y_chao;
        cena.sombra(Vec3(p.pos.x, y_chao, p.pos.z), 0.55f * (1.0f - fminf(h / 12.0f, 0.6f)));
    }
}
