#include "hud_combate.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "conteudo/textura.h"
#include "nucleo/fonte.h"
#include "plataforma/plataforma.h"
#include "render3d/instancia.h"

namespace {
const Cor BRANCO = rgb(0xf6f3ec), TINTA = rgb(0x15161c), AMARELO = rgb(0xffc93c);
const Cor VERDE = rgb(0x5fd35a), AZUL = rgb(0x3fa2ff), FUNDO = rgb(0x0f4f57, 190), CINZA = rgb(0x9a9a9a);
const f32 PI = 3.14159265f;

Cor cor_hex(u32 h, u8 a = 255) { return rgb(h, a); }

void card(ListaDesenho2D& l, f32 x, f32 y, f32 w, f32 h) {
    l.retangulo(x + 3, y + 3, w, h, rgb(0x15161c, 140));
    l.retangulo(x, y, w, h, FUNDO);
    l.moldura(x, y, w, h, 2, TINTA);
}

// Ponto do mundo na tela (640 × 448); false se atrás da câmera.
bool tela(const Camera3D& cam, Vec3 p, f32* x, f32* y) {
    Vec4 c = transformar(cam.vp, p);
    if (c.w < cam.perto) return false;
    *x = 320.0f + 320.0f * c.x / c.w;
    *y = 224.0f - 224.0f * c.y / c.w;
    return true;
}

void linha(ListaDesenho2D& l, f32 x0, f32 y0, f32 x1, f32 y1, f32 e, Cor c) {
    f32 dx = x1 - x0, dy = y1 - y0, n = sqrtf(dx * dx + dy * dy);
    if (n < 0.5f) return;
    f32 px = -dy / n * e * 0.5f, py = dx / n * e * 0.5f;
    f32 xs[4] = {x0 + px, x0 - px, x1 + px, x1 - px}, ys[4] = {y0 + py, y0 - py, y1 + py, y1 - py};
    l.quad(xs, ys, c);
}

// Símbolos dos botões do DualShock (a fonte não tem esses desenhos).
enum Botao { B_TRIANGULO, B_CIRCULO, B_XIS, B_QUADRADO };
void botao(ListaDesenho2D& l, f32 x, f32 y, f32 t, Botao b) {
    l.retangulo(x, y, t, t, rgb(0x15161c, 220));
    f32 c = x + t * 0.5f, m = y + t * 0.5f, r = t * 0.32f;
    switch (b) {
    case B_TRIANGULO: {
        Cor v = rgb(0x5fd3a4);
        linha(l, c, m - r, c - r, m + r * 0.7f, 2, v);
        linha(l, c - r, m + r * 0.7f, c + r, m + r * 0.7f, 2, v);
        linha(l, c + r, m + r * 0.7f, c, m - r, 2, v);
        break;
    }
    case B_CIRCULO: {
        Cor v = rgb(0xff6b6b);
        for (int k = 0; k < 8; k++) {
            f32 a0 = k * PI / 4, a1 = (k + 1) * PI / 4;
            linha(l, c + cosf(a0) * r, m + sinf(a0) * r, c + cosf(a1) * r, m + sinf(a1) * r, 2, v);
        }
        break;
    }
    case B_XIS: {
        Cor v = rgb(0x6fa8ff);
        linha(l, c - r, m - r, c + r, m + r, 2, v);
        linha(l, c - r, m + r, c + r, m - r, 2, v);
        break;
    }
    case B_QUADRADO:
        l.moldura(c - r, m - r, 2 * r, 2 * r, 2, rgb(0xff8fd0));
        break;
    }
}
}  // namespace

void desenhar_botao(ListaDesenho2D& l, f32 x, f32 y, f32 t, int b) { botao(l, x, y, t, static_cast<decltype(B_XIS)>(b)); }
void desenhar_linha(ListaDesenho2D& l, f32 x0, f32 y0, f32 x1, f32 y1, f32 e, Cor c) { linha(l, x0, y0, x1, y1, e, c); }

bool HudCombate::iniciar(const Pacote& pak, BibliotecaModelos& bib, Arena& arena, const BancoItens& itens, char* erro, int ne) {
    itens_ = &itens;
    // Ícones do inventário do Ventania (convertidos na Etapa 3), uma vez só na VRAM.
    static int icones_reg[16];
    static int icone_mun_reg = -2;
    if (icone_mun_reg == -2) {
        for (int i = 0; i < 16; i++) icones_reg[i] = -1;
        u32 t = 0;
        char nome[48], e[96];
        TexturaBrisa tex;
        for (int i = 1; i < itens.n_itens() && i < 16; i++) {
            snprintf(nome, sizeof nome, "texturas/icone_%s.btex", itens.id(i));
            const u8* d = pak.arquivo(nome, &t);
            if (d && tex.carregar(d, t, e, sizeof e)) icones_reg[i] = plataforma::registrar_textura(tex);
        }
        const u8* d = pak.arquivo("texturas/icone_municao.btex", &t);
        icone_mun_reg = d && tex.carregar(d, t, e, sizeof e) ? plataforma::registrar_textura(tex) : -1;
    }
    for (int i = 0; i < 16; i++) icone_[i] = icones_reg[i];
    icone_municao_ = icone_mun_reg;
    // Anéis do loot no chão (cilindro baixo na cor da raridade; munição em cinza).
    for (int r = 0; r < 7; r++) {
        ConstrutorMalha c(arena, 16 * 4, 2.0f);
        Cor cor = r < itens.n_raridades() && r < 6 ? cor_hex(itens.raridade(r).cor) : CINZA;
        c.cilindro(Vec3(0, 0, 0), 0.42f, 0.03f, 16, cor);
        aneis_[r] = c.terminar();
        if (c.estourou()) { snprintf(erro, ne, "anel do loot"); return false; }
    }
    if ((municao_modelo_ = bib.carregar("municao_01", erro, ne)) < 0) return false;
    if ((bau_ = bib.carregar("bau_ventania_01", erro, ne)) < 0) return false;
    bau_modelo_ = &bib[bau_];
    u32 t = 0;
    char e[96];
    const u8* d = pak.arquivo("animacoes/bau_ventania_01.bani", &t);
    if (!d || !anim_bau_.carregar(d, t, e, sizeof e) || (clipe_abrir_ = anim_bau_.achar("abrir")) < 0) {
        snprintf(erro, ne, "animação do baú: %s", d ? e : "ausente");
        return false;
    }
    return true;
}

const Pose* HudCombate::pose_bau(f32 tampa) const {
    if (tampa <= 0.001f) return nullptr;  // fechado: pose de repouso
    anim_bau_.amostrar(clipe_abrir_, tampa * anim_bau_.duracao(clipe_abrir_), *bau_modelo_, pose_bau_);
    return &pose_bau_;
}

void HudCombate::montar_loot(Cena3D& cena, const Loot& loot, const BibliotecaModelos& bib, const Jogador& j, f32 t) const {
    Vec3 cam = cena.camera.posicao;
    for (int i = 0; i < loot.n_itens(); i++) {
        const ItemChao& l = loot.item(i);
        if (!l.id) continue;
        Vec3 d = l.pos - cam;
        if (ponto(d, d) > alcance_loot * alcance_loot) continue;
        int mod = l.municao ? municao_modelo_ : j.modelo_do_item(l.item);
        if (mod < 0) continue;
        f32 fase = l.id * 1.7f;
        Vec3 p = l.pos;
        if (l.salto < 1.0f) p = l.de + (l.pos - l.de) * l.salto + Vec3(0, sinf(l.salto * PI) * 0.9f, 0);
        else cena.adicionar(&aneis_[l.reserva() ? 6 : (l.raridade < 6 ? l.raridade : 5)], Mat4::translacao(l.pos + Vec3(0, 0.005f, 0)));
        Mat4 m = Mat4::translacao(p + Vec3(0, 0.35f + sinf(t * 2.0f + fase) * 0.05f, 0)) * Mat4::rotacao_y(t * 1.2f + fase);
        adicionar_modelo(cena, bib[mod], nullptr, m);
    }
}

void HudCombate::registrar(const EventosCombate& ev, const Jogador& j) {
    if (ev.atirou)
        for (int k = 0; k < ev.n_fins; k++) {
            if (n_trac_ >= 24) break;
            trac_[n_trac_++] = {j.cano(), ev.fins[k], 4, ev.fim_acertou[k]};
        }
    for (int k = 0; k < ev.n_acertos; k++) {
        if (n_num_ < 16) num_[n_num_++] = {ev.acertos[k].pos, ev.acertos[k].dano, 24, ev.acertos[k].cabeca, ev.acertos[k].escudo};
        marcador_ = 6;
        marcador_cabeca_ = ev.acertos[k].cabeca;
    }
    if (ev.erro != Inventario::OK) mensagem(Inventario::mensagem(ev.erro));
}

void HudCombate::passo() {
    int n = 0;
    for (int i = 0; i < n_trac_; i++)
        if (--trac_[i].vida > 0) trac_[n++] = trac_[i];
    n_trac_ = n;
    n = 0;
    for (int i = 0; i < n_num_; i++)
        if (--num_[i].vida > 0) {
            num_[i].pos.y += 0.03f;
            num_[n++] = num_[i];
        }
    n_num_ = n;
    if (marcador_ > 0) marcador_--;
    if (msg_vida_ > 0) msg_vida_--;
}

void HudCombate::mensagem(const char* t) {
    snprintf(msg_, sizeof msg_, "%s", t);
    msg_vida_ = 75;  // 2,5 s
}

bool HudCombate::controlar_inventario(const Controle& c, Jogador& j, Loot& loot, const MundoColisao& m) {
    if (c.pressionou(BT_SELECT) && !(c.segurando(BT_L1) && c.segurando(BT_R1))) {
        inv_aberto_ = !inv_aberto_;
        marcado_ = -1;
        cursor_ = j.combate().inv.selecionado < 0 ? 0 : j.combate().inv.selecionado;
        return true;
    }
    if (!inv_aberto_) return false;
    if (c.pressionou(BT_CIRCULO)) { inv_aberto_ = false; return true; }
    if (c.pressionou(BT_ESQUERDA)) cursor_ = (cursor_ + Inventario::ESPACOS - 1) % Inventario::ESPACOS;
    if (c.pressionou(BT_DIREITA)) cursor_ = (cursor_ + 1) % Inventario::ESPACOS;
    Inventario& inv = j.combate_mut().inv;
    if (c.pressionou(BT_XIS)) {
        if (marcado_ < 0) marcado_ = cursor_;
        else {
            Inventario::Erro r = marcado_ == cursor_ ? Inventario::OK : inv.trocar(marcado_, cursor_);
            if (r != Inventario::OK) mensagem(Inventario::mensagem(r));
            marcado_ = -1;
        }
    }
    if (c.pressionou(BT_QUADRADO)) {
        Espaco s;
        Inventario::Erro r = inv.tirar_de(cursor_, 9999, &s);
        if (r == Inventario::OK) {
            const Personagem& p = j.personagem();
            f32 y = j.camera().yaw_rad();
            Vec3 alvo = p.pos + Vec3(-sinf(y), 0, -cosf(y)) * 1.6f;
            if (m.raio(p.pos + Vec3(0, 1, 0), alvo + Vec3(0, 1, 0)).t < 1.0f) alvo = p.pos;
            f32 ch = alvo.y;
            if (m.chao(alvo.x, alvo.z, 0.1f, alvo.y + 1.2f, &ch)) alvo.y = ch + 0.02f;
            loot.criar_item(s.item, s.raridade, s.qtd, itens_->eh_arma(s.item) ? s.pente : -1, alvo, j.combate().id(), 0);
        } else {
            mensagem(Inventario::mensagem(r));
        }
    }
    if (c.pressionou(BT_TRIANGULO)) {  // dividir a pilha ao meio
        const Espaco& s = inv.espacos[cursor_];
        Inventario::Erro r = s.qtd >= 2 ? inv.dividir(cursor_, s.qtd / 2) : Inventario::INVALIDO;
        if (r != Inventario::OK) mensagem(Inventario::mensagem(r));
    }
    return true;
}

void HudCombate::hotbar(ListaDesenho2D& l, const Jogador& j) const {
    const Inventario& inv = j.combate().inv;
    const f32 T = 44, G = 4, x0 = 320 - (6 * T + 5 * G) * 0.5f, y0 = 380;
    for (int k = -1; k < Inventario::ESPACOS; k++) {
        f32 x = x0 + (k + 1) * (T + G);
        bool sel = inv.selecionado == k;
        const Espaco& s = k < 0 ? inv.atual() : inv.espacos[k];
        int item = k < 0 ? itens_->picareta() : s.item;
        Cor fundo = rgb(0x101c33, 200);
        if (k >= 0 && !s.vazio() && itens_->eh_arma(s.item)) fundo = cor_hex(itens_->raridade(s.raridade).cor, 150);
        l.retangulo(x, y0 - (sel ? 6 : 0), T, T, fundo);
        l.moldura(x, y0 - (sel ? 6 : 0), T, T, sel ? 3 : 2, sel ? AMARELO : TINTA);
        if (item > 0 && icone_[item] >= 0) l.sprite(x + 4, y0 + 4 - (sel ? 6 : 0), T - 8, T - 8, icone_[item]);
        if (k >= 0 && !s.vazio() && itens_->eh_consumivel(s.item)) {
            char q[8];
            snprintf(q, sizeof q, "%u", static_cast<unsigned>(s.qtd));
            fonte::desenhar(l, x + T - 4 - fonte::largura(q, 1), y0 + T - 12 - (sel ? 6 : 0), 1, BRANCO, q);
        }
        char n[4];
        snprintf(n, sizeof n, "%d", k + 2 > 1 ? k + 1 : 0);
        if (k >= 0) fonte::desenhar(l, x + 3, y0 + 2 - (sel ? 6 : 0), 1, rgb(0xf6f3ec, 170), n);
    }
    // nome e munição do item na mão
    const Espaco& a = inv.atual();
    char nome[64], mun[32] = "";
    itens_->nome_completo(a.item, itens_->eh_arma(a.item) ? a.raridade : -1, nome, sizeof nome);
    Cor cn = itens_->eh_arma(a.item) ? cor_hex(itens_->raridade(a.raridade).cor) : BRANCO;
    fonte::desenhar_adesivo(l, 320 - fonte::largura(nome, 2) * 0.5f, 342, 2, cn, TINTA, 1, 1, nome);
    if (itens_->eh_arma(a.item)) {
        const ArmaDado& w = itens_->arma(a.item);
        snprintf(mun, sizeof mun, "%u / %u", static_cast<unsigned>(a.pente), static_cast<unsigned>(inv.municao[w.municao]));
        fonte::desenhar_adesivo(l, 320 - fonte::largura(mun, 2) * 0.5f, 362, 2, BRANCO, TINTA, 1, 1, mun);
    }
}

void HudCombate::aviso(ListaDesenho2D& l, const Jogador& j, const Loot& loot) const {
    const Combatente& c = j.combate();
    int acao = c.acao();
    if (acao == ACAO_RECARGA || acao == ACAO_USO || acao == ACAO_BAU) {
        const char* t = acao == ACAO_RECARGA ? "RECARREGANDO" : (acao == ACAO_USO ? "USANDO" : "ABRINDO O BAÚ");
        f32 w = 200, x = 320 - w * 0.5f, y = 292;
        l.retangulo(x, y, w, 10, rgb(0x15161c, 200));
        l.retangulo(x + 2, y + 2, (w - 4) * c.progresso(), 6, AMARELO);
        fonte::desenhar_adesivo(l, 320 - fonte::largura(t, 1) * 0.5f, y - 14, 1, BRANCO, TINTA, 1, 0, t);
        return;
    }
    const Personagem& p = j.personagem();
    char t[96];
    if (loot.bau_perto(p.pos) >= 0) {
        snprintf(t, sizeof t, "ABRIR O BAÚ (SEGURE)");
    } else {
        int i = loot.mais_perto(p.pos);
        if (i < 0) return;
        const ItemChao& it = loot.item(i);
        char n[64];
        itens_->nome_completo(it.item, itens_->eh_arma(it.item) ? it.raridade : -1, n, sizeof n);
        if (it.qtd > 1) snprintf(t, sizeof t, "PEGAR %s X%u", n, static_cast<unsigned>(it.qtd));
        else snprintf(t, sizeof t, "PEGAR %s", n);
        if (c.inv.cabe(it.item) == 0) {
            size_t k = strlen(t);
            if (c.inv.picareta_na_mao()) snprintf(t + k, sizeof t - k, " (CHEIO)");
            else snprintf(t + k, sizeof t - k, " (TROCA)");
        }
    }
    f32 w = fonte::largura(t, 1) + 30, x = 320 - w * 0.5f, y = 262;
    l.retangulo(x, y, w, 18, rgb(0x15161c, 190));
    botao(l, x + 2, y + 2, 14, B_TRIANGULO);
    fonte::desenhar(l, x + 22, y + 4, 1, BRANCO, t);
}

void HudCombate::tela_inventario(ListaDesenho2D& l, const Jogador& j) const {
    const Inventario& inv = j.combate().inv;
    f32 x0 = 110, y0 = 110, w = 420, h = 220;
    card(l, x0, y0, w, h);
    fonte::desenhar_adesivo(l, x0 + 14, y0 + 10, 2, AMARELO, TINTA, 1, 0, "INVENTÁRIO");
    const f32 T = 64, G = 12, sx = 320 - (5 * T + 4 * G) * 0.5f, sy = y0 + 40;
    for (int k = 0; k < Inventario::ESPACOS; k++) {
        f32 x = sx + k * (T + G);
        const Espaco& s = inv.espacos[k];
        Cor fundo = !s.vazio() && itens_->eh_arma(s.item) ? cor_hex(itens_->raridade(s.raridade).cor, 160) : rgb(0x101c33, 210);
        l.retangulo(x, sy, T, T, fundo);
        l.moldura(x, sy, T, T, cursor_ == k ? 3 : 2, cursor_ == k ? AMARELO : (marcado_ == k ? BRANCO : TINTA));
        if (!s.vazio() && icone_[s.item] >= 0) l.sprite(x + 6, sy + 6, T - 12, T - 12, icone_[s.item]);
        if (!s.vazio() && itens_->eh_consumivel(s.item)) {
            char q[8];
            snprintf(q, sizeof q, "%u", static_cast<unsigned>(s.qtd));
            fonte::desenhar(l, x + T - 6 - fonte::largura(q, 1), sy + T - 12, 1, BRANCO, q);
        }
    }
    const Espaco& c = inv.espacos[cursor_];
    char n[64] = "VAZIO";
    if (!c.vazio()) itens_->nome_completo(c.item, itens_->eh_arma(c.item) ? c.raridade : -1, n, sizeof n);
    fonte::desenhar(l, 320 - fonte::largura(n, 1) * 0.5f, sy + T + 8, 1, BRANCO, n);
    // reservas: munição (duas por linha) e materiais
    char linha[128];
    if (icone_municao_ >= 0) l.sprite(x0 + 14, sy + T + 26, 16, 16, icone_municao_);
    for (int t0 = 0; t0 < itens_->n_municoes(); t0 += 2) {
        int k = 0;
        for (int t = t0; t < t0 + 2 && t < itens_->n_municoes(); t++)
            k += snprintf(linha + k, sizeof linha - k, "%s%s %u", t > t0 ? " · " : "", itens_->municao(t).nome, static_cast<unsigned>(inv.municao[t]));
        fonte::desenhar(l, x0 + 36, sy + T + 26 + (t0 / 2) * 13, 1, BRANCO, linha);
    }
    int k = 0;
    for (int t = 0; t < itens_->n_materiais(); t++)
        k += snprintf(linha + k, sizeof linha - k, "%s%s %u", t ? " · " : "", itens_->material(t).nome, static_cast<unsigned>(inv.materiais[t]));
    fonte::desenhar(l, x0 + 36, sy + T + 54, 1, rgb(0xf6f3ec, 190), linha);
    // ajuda
    f32 y = y0 + h - 24, x = x0 + 14;
    fonte::desenhar(l, x, y + 3, 1, BRANCO, "ESQ./DIR. ESCOLHER");
    x += 120;
    botao(l, x, y, 14, B_XIS);
    fonte::desenhar(l, x + 18, y + 3, 1, BRANCO, marcado_ >= 0 ? "SOLTAR" : "MOVER");
    x += 70;
    botao(l, x, y, 14, B_QUADRADO);
    fonte::desenhar(l, x + 18, y + 3, 1, BRANCO, "LARGAR");
    x += 70;
    botao(l, x, y, 14, B_TRIANGULO);
    fonte::desenhar(l, x + 18, y + 3, 1, BRANCO, "DIVIDIR");
    x += 76;
    botao(l, x, y, 14, B_CIRCULO);
    fonte::desenhar(l, x + 18, y + 3, 1, BRANCO, "FECHAR");
}

void HudCombate::desenhar(ListaDesenho2D& l, const Jogador& j, const Loot& loot, const Camera3D& cam) const {
    // traçantes e números de dano (pontos do mundo projetados na tela)
    for (int i = 0; i < n_trac_; i++) {
        f32 x0, y0, x1, y1;
        if (!tela(cam, trac_[i].a, &x0, &y0) || !tela(cam, trac_[i].b, &x1, &y1)) continue;
        linha(l, x0, y0, x1, y1, 2, rgb(0xffe08a, static_cast<u8>(60 * trac_[i].vida)));
    }
    for (int i = 0; i < n_num_; i++) {
        f32 x, y;
        if (!tela(cam, num_[i].pos, &x, &y)) continue;
        char t[8];
        snprintf(t, sizeof t, "%d", num_[i].dano);
        Cor c = num_[i].cabeca ? AMARELO : (num_[i].escudo ? AZUL : BRANCO);
        fonte::desenhar_adesivo(l, x - fonte::largura(t, 2) * 0.5f, y - 10, 2, c, TINTA, 1, 1, t);
    }
    // mira (abre com a dispersão) e marcador de acerto
    if (!inv_aberto_) {
        f32 ab = 6 + (1 - j.portador().ads()) * 4 + j.portador().recuo() * 4;
        Cor cm = rgb(0xffffff, 220);
        l.retangulo(320 - ab - 6, 223, 6, 2, cm);
        l.retangulo(320 + ab, 223, 6, 2, cm);
        l.retangulo(319, 224 - ab - 6, 2, 6, cm);
        l.retangulo(319, 224 + ab, 2, 6, cm);
        if (marcador_ > 0) {
            Cor h = marcador_cabeca_ ? AMARELO : BRANCO;
            linha(l, 312, 216, 316, 220, 2, h);
            linha(l, 328, 216, 324, 220, 2, h);
            linha(l, 312, 232, 316, 228, 2, h);
            linha(l, 328, 232, 324, 228, 2, h);
        }
    }
    // vida e escudo (canto inferior esquerdo)
    const Vitais& v = j.combate().vit;
    char t[16];
    f32 x = 20, y = 392, w = 112;
    l.retangulo(x, y, w, 10, rgb(0x15161c, 200));
    l.retangulo(x + 1, y + 1, (w - 2) * v.escudo / 100.0f, 8, AZUL);
    l.retangulo(x, y + 14, w, 16, rgb(0x15161c, 200));
    l.retangulo(x + 1, y + 15, (w - 2) * v.vida / 100.0f, 14, VERDE);
    snprintf(t, sizeof t, "%d", v.vida);
    fonte::desenhar_adesivo(l, x + w + 6, y + 16, 2, BRANCO, TINTA, 1, 1, t);
    snprintf(t, sizeof t, "%d", v.escudo);
    fonte::desenhar(l, x + w + 6, y, 1, AZUL, t);
    hotbar(l, j);
    if (inv_aberto_) tela_inventario(l, j);
    else aviso(l, j, loot);
    if (msg_vida_ > 0) {
        f32 mw = fonte::largura(msg_, 1) + 16;
        l.retangulo(320 - mw * 0.5f, 312, mw, 16, rgb(0x15161c, 200));
        fonte::desenhar(l, 320 - mw * 0.5f + 8, 315, 1, AMARELO, msg_);
    }
}
