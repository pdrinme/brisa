#include "jogador.h"

#include <math.h>
#include <stdio.h>

#include <string.h>

#include "render3d/instancia.h"

namespace {
const char* const TRAJES[Jogador::N_TRAJES] = {"recruta_base_01", "recruta_brisa_01", "capitao_mare_01", "entregador_01",
                                               "escafandro_01", "faz_tudo_01", "espantalho_01", "explorador_01",
                                               "botanico_01", "astronomo_01", "guardiao_01"};
}  // namespace

bool Jogador::iniciar(const Pacote& pak, BibliotecaModelos& bib, char* erro, int ne) {
    for (int t = 0; t < N_TRAJES; t++)
        if ((trajes_[t] = bib.carregar(TRAJES[t], erro, ne)) < 0) return false;
    u32 tam = 0;
    char e[96];
    const u8* d = pak.arquivo("animacoes/humanoide.bani", &tam);
    if (!d || !anims_.carregar(d, tam, e, sizeof e) || !anim_.iniciar(anims_)) {
        snprintf(erro, ne, "animações do personagem: %s", d ? e : "ausente");
        return false;
    }
    d = pak.arquivo("dados/itens.bdat", &tam);
    if (!d || !itens_.carregar(d, tam, e, sizeof e)) { snprintf(erro, ne, "itens: %s", d ? e : "dados/itens.bdat ausente"); return false; }
    if (itens_.n_itens() > MAX_ITENS) { snprintf(erro, ne, "itens demais (%d)", itens_.n_itens()); return false; }
    for (int i = 0; i < MAX_ITENS; i++) modelo_item_[i] = -1;
    for (int i = 1; i < itens_.n_itens(); i++)
        if ((modelo_item_[i] = bib.carregar(itens_.modelo(i), erro, ne)) < 0) return false;
    bib_ = &bib;
    comb_.iniciar(&itens_, 1);
    reposicionar(Vec3(), 0);
    return true;
}

void Jogador::reposicionar(Vec3 pes, f32 yaw, f32 pitch) {
    p_ = Personagem();
    p_.pos = pes;
    cam_ = Camera3P();
    cam_.yaw = fmodf(yaw + 720.0f, 360.0f);
    cam_.pitch = pitch;
    agachar_ = false;
}

Mat4 Jogador::matriz() const {
    // O modelo olha para +z; o jogo usa −z como frente (como no Ventania).
    return Mat4::translacao(p_.pos) * Mat4::rotacao_y(p_.yaw + 3.14159265f);
}

void Jogador::passo(const Controle& c, const MundoColisao& m, Loot& loot, AlvoCombate* alvos, int n, u32 tick, bool bloquear,
                    EventosCombate& ev) {
    const f32 dt = 1.0f / 30.0f;
    const EstadoControle& e = c.estado();
    if (c.pressionou(BT_R3)) agachar_ = !agachar_;  // R3 alterna (Ventania)
    cam_.mirando = c.segurando(BT_L2) && !bloquear;
    cam_.olhar(Controle::eixo(e.rx), Controle::eixo(e.ry), dt);
    ComandoMovimento cmd;
    cmd.x = Controle::eixo(e.lx);
    cmd.y = -Controle::eixo(e.ly);  // analógico para cima = frente
    cmd.yaw = cam_.yaw_rad();
    cmd.pular = c.pressionou(BT_XIS);
    cmd.correr = c.segurando(BT_L3);
    cmd.agachar = agachar_;
    passo_movimento(p_, cmd, m, dt);  // sem espaço para levantar, continua agachado
    anim_.atualizar(p_, c.pressionou(BT_ESQUERDA), dt);
    cam_.atualizar(p_.pos, p_.agachado, m, dt);
    // Combate (R1/L1 só trocam de item sozinhos: os dois juntos são os atalhos de depuração).
    EntradaCombate ec;
    ec.yaw = cam_.yaw_rad();
    ec.pitch = cam_.pitch * 3.14159265f / 180.0f;
    if (!bloquear) {
        ec.atirar = c.segurando(BT_R2);
        ec.mirar = c.segurando(BT_L2);
        ec.recarregar = c.pressionou(BT_QUADRADO);
        ec.interagir = c.segurando(BT_TRIANGULO);
        if (c.pressionou(BT_R1) && !c.segurando(BT_L1)) ec.ciclo = 1;
        if (c.pressionou(BT_L1) && !c.segurando(BT_R1)) ec.ciclo = -1;
        ec.picareta = c.pressionou(BT_DIREITA);
    }
    comb_.passo(ec, p_, m, loot, alvos, n, tick, ev);
    if (ev.atirou) {
        const char* id = itens_.id(comb_.inv.atual().item);
        port_.disparou(!strcmp(id, "trovao") || !strcmp(id, "gaviao"));
    }
    // Item na mão e ação para o visual.
    int item = comb_.inv.atual().item;
    port_.pedir_item(item, item > 0 && modelo_item_[item] >= 0 ? &(*bib_)[modelo_item_[item]] : nullptr, itens_.pose(item), itens_.id(item));
    Vec3 alcance;
    bool alcancar = false;
    int b = comb_.bau_interagindo();
    if (b >= 0) {
        const BauMundo& bau = loot.bau(b);
        Vec3 para_mim = p_.pos - bau.pos;
        para_mim.y = 0;
        f32 l = sqrtf(ponto(para_mim, para_mim));
        Vec3 alca = bau.pos + Vec3(0, 0.62f, 0) + (l > 0.01f ? para_mim * (0.35f / l) : Vec3());
        Mat4 inv = inversa_rigida(matriz());
        Vec4 q = transformar(inv, alca);
        alcance = Vec3(q.x, q.y, q.z);
        alcancar = true;
    }
    port_.atualizar(dt, ec.pitch, cam_.mirando, sqrtf(p_.vel.x * p_.vel.x + p_.vel.z * p_.vel.z), comb_.acao(), comb_.progresso(),
                    alcancar, alcance);
}

void Jogador::montar(Cena3D& cena, const BibliotecaModelos& bib, const MundoColisao& m, const Loot*, f32 aspecto) {
    cam_.aplicar(cena.camera, aspecto);
    static Pose pose;
    const ModeloBrisa& mod = bib[trajes_[traje_]];
    anim_.pose(mod, pose);
    Mat4 mm = matriz(), item;
    int ii = port_.item_na_mao();
    if (port_.aplicar(mod, pose, &item) && ii > 0 && modelo_item_[ii] >= 0) adicionar_modelo(cena, bib[modelo_item_[ii]], nullptr, mm * item);
    adicionar_modelo(cena, mod, &pose, mm);
    Vec3 c;
    if (port_.cano(&c)) {
        Vec4 w = transformar(mm, c);
        cano_ = Vec3(w.x, w.y, w.z);
    } else {
        tiro::Raio r = tiro::mira(p_.pos, p_.agachado, cam_.yaw_rad(), cam_.pitch * 3.14159265f / 180.0f);
        cano_ = r.origem;
    }
    f32 y_chao = 0;
    if (m.chao(p_.pos.x, p_.pos.z, 0.1f, p_.pos.y + 0.05f, &y_chao)) {
        f32 h = p_.pos.y - y_chao;
        cena.sombra(Vec3(p_.pos.x, y_chao, p_.pos.z), 0.55f * (1.0f - fminf(h / 12.0f, 0.6f)));
    }
}
