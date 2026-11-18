#include "tela_boot.h"

#include <math.h>
#include <stdio.h>

#include "nucleo/fonte.h"
#include "nucleo/formato.h"

// Cores do tema do Ventania (game/ui/shared/ventania_theme.gd).
namespace {
const Cor TEAL = rgb(0x22b8a8);
const Cor TEAL_NOITE = rgb(0x0f4f56);
const Cor INDIGO = rgb(0x2b2470);
const Cor AMARELO = rgb(0xffc93c);
const Cor CORAL = rgb(0xff6b5b);
const Cor BRANCO = rgb(0xf6f3ec);
const Cor TINTA = rgb(0x15161c);
const Cor NOITE_CARTAO = rgb(0x101c33, 230);
const Cor APAGADO = rgb(0xf6f3ec, 60);
const Cor CORES_QUADRADO[] = {CORAL, AMARELO, TEAL, BRANCO};
const int N_CORES = 4;
const f32 PASSO_S = 1.0f / 30.0f;

Cor com_alfa(Cor c, u8 a) { c.a = a; return c; }

void texto_centro(ListaDesenho2D& l, f32 cx, f32 y, f32 e, Cor c, const char* s) {
    fonte::desenhar(l, cx - fonte::largura(s, e) / 2, y, e, c, s);
}

void cartao(ListaDesenho2D& l, f32 x, f32 y, f32 w, f32 h) {
    l.retangulo(x + 6, y + 6, w, h, com_alfa(TINTA, 150));  // sombra dura deslocada
    l.retangulo(x, y, w, h, NOITE_CARTAO);
    l.moldura(x, y, w, h, 3, TINTA);
}

void caixa_botao(ListaDesenho2D& l, f32 x, f32 y, f32 w, f32 h, bool ativo, const char* rotulo) {
    l.retangulo(x, y, w, h, ativo ? AMARELO : com_alfa(BRANCO, 25));
    l.moldura(x, y, w, h, 2, ativo ? TINTA : APAGADO);
    if (rotulo) texto_centro(l, x + w / 2, y + h / 2 - 11, 2, ativo ? TINTA : BRANCO, rotulo);
}

// Quadrado girado (4 vértices em ordem de tira).
void quadrado_girado(ListaDesenho2D& l, f32 cx, f32 cy, f32 meio, f32 graus, Cor c) {
    f32 a = graus * 3.14159265f / 180.0f;
    f32 co = cosf(a) * meio, si = sinf(a) * meio;
    // cantos (-1,-1) (1,-1) (-1,1) (1,1) girados
    f32 xs[4] = {cx - co + si, cx + co + si, cx - co - si, cx + co - si};
    f32 ys[4] = {cy - si - co, cy + si - co, cy - si + co, cy + si + co};
    l.quad(xs, ys, c);
}

void triangulo(ListaDesenho2D& l, f32 cx, f32 cy, f32 r, Cor c) {
    f32 xs[4] = {cx, cx, cx - r, cx + r};
    f32 ys[4] = {cy - r, cy - r, cy + r * 0.8f, cy + r * 0.8f};
    l.quad(xs, ys, c);
}

void circulo(ListaDesenho2D& l, f32 cx, f32 cy, f32 r, Cor c) {
    // Octógono em 3 peças que não se sobrepõem (com alfa, sobreposição escurece).
    f32 k = r * 0.41f;
    l.retangulo(cx - r, cy - k, 2 * r, 2 * k, c);
    f32 tx[4] = {cx - k, cx + k, cx - r, cx + r};
    f32 ty[4] = {cy - r, cy - r, cy - k, cy - k};
    l.quad(tx, ty, c);
    f32 bx[4] = {cx - r, cx + r, cx - k, cx + k};
    f32 by[4] = {cy + k, cy + k, cy + r, cy + r};
    l.quad(bx, by, c);
}

void xis(ListaDesenho2D& l, f32 cx, f32 cy, f32 r, f32 e, Cor c) {
    f32 xs[4] = {cx - r, cx - r + e, cx + r - e, cx + r};
    f32 ys[4] = {cy - r, cy - r, cy + r, cy + r};
    l.quad(xs, ys, c);
    f32 xs2[4] = {cx + r - e, cx + r, cx - r, cx - r + e};
    f32 ys2[4] = {cy - r, cy - r, cy + r, cy + r};
    l.quad(xs2, ys2, c);
}
}  // namespace

const f32 TelaBoot::GIRO_GRAUS_POR_SEG = 90.0f;

void TelaBoot::passo(const Controle& c) {
    passos_++;
    if (c.pressionou(BT_START)) pausado_ = !pausado_;
    if (c.pressionou(BT_XIS)) cor_ = (cor_ + 1) % N_CORES;
    if (pausado_) return;
    vento_++;
    f32 giro = Controle::eixo(c.estado().rx);
    angulo_ += GIRO_GRAUS_POR_SEG * (1.0f + 2.0f * giro) * PASSO_S;
    if (angulo_ >= 360.0f) angulo_ -= 360.0f;
    if (angulo_ < 0.0f) angulo_ += 360.0f;
    if (c.segurando(BT_R1)) escala_ += 0.05f;
    if (c.segurando(BT_L1)) escala_ -= 0.05f;
    if (escala_ < 0.5f) escala_ = 0.5f;
    if (escala_ > 1.5f) escala_ = 1.5f;
}

void TelaBoot::montar(ListaDesenho2D& l, const Controle& controle, const EstatisticasQuadro& est,
                      const InfoPlataforma& info) const {
    const f32 W = 640, H = 448;
    l.limpar(TINTA);
    l.gradiente(0, 0, W, H, TEAL_NOITE, TEAL_NOITE, INDIGO, INDIGO);

    // Rajadas de vento: faixas claras inclinadas, nunca com 1 px de altura (cintila no entrelaçado).
    for (int i = 0; i < 7; i++) {
        f32 comp = 120.0f + static_cast<f32>((i * 37) % 80);
        f32 vel = 60.0f + i * 15.0f;
        f32 per = W + comp;
        f32 x = fmodf(vento_ * vel * PASSO_S + i * 97.0f, per) - comp;
        f32 y = 30.0f + i * 58.0f;
        f32 xs[4] = {x, x + comp, x, x + comp};
        f32 ys[4] = {y, y - 8, y + 4, y - 4};
        l.quad(xs, ys, com_alfa(BRANCO, 26));
    }

    // Logotipo em estilo adesivo: contorno grosso escuro e sombra dura.
    const char* logo = "BRISA";
    f32 e = 10;
    fonte::desenhar_adesivo(l, (W - fonte::largura(logo, e)) / 2, 4, e, BRANCO, TINTA, 5, 7, logo);
    const char* sub = "PLAYSTATION 2 EDITION";
    fonte::desenhar_adesivo(l, (W - fonte::largura(sub, 2)) / 2, 100, 2, AMARELO, TINTA, 2, 3, sub);

    // Cartão esquerdo: o quadrado girando.
    cartao(l, 40, 136, 180, 194);
    texto_centro(l, 130, 142, 2, AMARELO, "START PAUSA");
    f32 meio = 46.0f * escala_;
    quadrado_girado(l, 135, 238, meio + 5, angulo_, com_alfa(TINTA, 140));  // sombra
    quadrado_girado(l, 130, 233, meio + 4, angulo_, TINTA);                // contorno
    quadrado_girado(l, 130, 233, meio, angulo_, CORES_QUADRADO[cor_]);
    if (pausado_) fonte::desenhar_adesivo(l, 130 - fonte::largura("PAUSADO", 3) / 2, 296, 3, AMARELO, TINTA, 2, 2, "PAUSADO");

    // Cartão direito: o controle.
    const f32 X = 236, Y = 136;
    cartao(l, X, Y, 364, 194);
    const EstadoControle& s = controle.estado();
    const char* modo = s.modo == ModoControle::ANALOGICO ? "CONTROLE 1: ANALÓGICO"
                     : s.modo == ModoControle::DIGITAL   ? "CONTROLE 1: DIGITAL"
                                                         : "CONTROLE 1: DESCONECTADO";
    fonte::desenhar(l, X + 14, Y + 8, 2, s.modo == ModoControle::DESCONECTADO ? CORAL : BRANCO, modo);
    auto b = [&](u16 bit) { return controle.segurando(bit); };
    caixa_botao(l, X + 14, Y + 36, 40, 22, b(BT_L2), "L2");
    caixa_botao(l, X + 58, Y + 36, 40, 22, b(BT_L1), "L1");
    caixa_botao(l, X + 266, Y + 36, 40, 22, b(BT_R1), "R1");
    caixa_botao(l, X + 310, Y + 36, 40, 22, b(BT_R2), "R2");
    caixa_botao(l, X + 112, Y + 70, 44, 22, b(BT_SELECT), "SEL");
    caixa_botao(l, X + 162, Y + 70, 68, 22, b(BT_START), "START");
    // Direcional.
    f32 cx = X + 60, cy = Y + 118;
    caixa_botao(l, cx - 11, cy - 35, 22, 22, b(BT_CIMA), nullptr);
    caixa_botao(l, cx - 11, cy + 13, 22, 22, b(BT_BAIXO), nullptr);
    caixa_botao(l, cx - 35, cy - 11, 22, 22, b(BT_ESQUERDA), nullptr);
    caixa_botao(l, cx + 13, cy - 11, 22, 22, b(BT_DIREITA), nullptr);
    // Botões de ação com as formas do controle.
    f32 fx = X + 304, fy = Y + 118;
    triangulo(l, fx, fy - 26, 10, b(BT_TRIANGULO) ? AMARELO : APAGADO);
    circulo(l, fx + 26, fy, 10, b(BT_CIRCULO) ? AMARELO : APAGADO);
    xis(l, fx, fy + 26, 9, 5, b(BT_XIS) ? AMARELO : APAGADO);
    l.moldura(fx - 36, fy - 10, 20, 20, 4, b(BT_QUADRADO) ? AMARELO : APAGADO);
    // Analógicos.
    const u8 eixos[2][2] = {{s.lx, s.ly}, {s.rx, s.ry}};
    const u16 cliques[2] = {BT_L3, BT_R3};
    const char* rot[2] = {"L3", "R3"};
    for (int k = 0; k < 2; k++) {
        f32 bx = X + 112 + k * 64, by = Y + 104;
        l.retangulo(bx, by, 54, 54, com_alfa(BRANCO, 20));
        l.moldura(bx, by, 54, 54, 2, b(cliques[k]) ? AMARELO : APAGADO);
        f32 px = bx + 27 + (eixos[k][0] - 127.5f) / 127.5f * 21.0f;
        f32 py = by + 27 + (eixos[k][1] - 127.5f) / 127.5f * 21.0f;
        bool ativo = s.modo == ModoControle::ANALOGICO;
        l.retangulo(px - 5, py - 5, 10, 10, ativo ? CORAL : APAGADO);
        texto_centro(l, bx + 27, by + 58, 2, b(cliques[k]) ? AMARELO : BRANCO, rot[k]);
    }

    // Rodapé: medições (sempre visíveis na Etapa 1).
    char linha[96], n1[16], n2[16], n3[16], n4[16];
    formato::decimal(n1, sizeof n1, est.media_quadro_us(), 1000, 1);
    formato::decimal(n2, sizeof n2, est.media_trabalho_us(), 1000, 1);
    formato::decimal(n3, sizeof n3, est.pior_trabalho_us(), 1000, 1);
    formato::decimal(n4, sizeof n4, est.fps_x10(), 10, 1);
    snprintf(linha, sizeof linha, "QUADRO %s MS · CPU %s (PIOR %s) · %s FPS", n1, n2, n3, n4);
    fonte::desenhar(l, 40, 340, 2, BRANCO, linha);
    formato::decimal(n1, sizeof n1, info.memoria_livre / 1024, 1024, 1);
    snprintf(linha, sizeof linha, "MEMÓRIA LIVRE %s MB · PROGRAMA %u KB", n1, static_cast<unsigned>(info.tamanho_programa / 1024));
    fonte::desenhar(l, 40, 362, 2, BRANCO, linha);
    snprintf(linha, sizeof linha, "%s · %s", info.nome, info.video);
    fonte::desenhar(l, 40, 384, 2, BRANCO, linha);
    fonte::desenhar(l, 40, 406, 2, AMARELO, "X COR · L1/R1 TAMANHO · ANALÓG. DIR. GIRA");
}
