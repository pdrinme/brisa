#include "jogo/tela_boot.h"
#include "plataforma/host/rasterizador.h"
#include "teste.h"

namespace {
EstadoControle analogico(u16 botoes = 0, u8 rx = 128) {
    EstadoControle e;
    e.modo = ModoControle::ANALOGICO;
    e.botoes = botoes;
    e.rx = rx;
    return e;
}
void passos(TelaBoot& t, Controle& c, const EstadoControle& e, int n) {
    for (int i = 0; i < n; i++) {
        c.ler(e);
        c.iniciar_passo();
        t.passo(c);
    }
}
}  // namespace

TESTE(tela_boot_gira_90_graus_por_segundo) {
    TelaBoot t;
    Controle c;
    passos(t, c, analogico(), 30);
    VERIFICA(t.angulo_graus() > 89.0f && t.angulo_graus() < 91.0f);
    TelaBoot t2;
    passos(t2, c, analogico(0, 255), 30);  // analógico direito todo para a direita: 3×
    VERIFICA(t2.angulo_graus() > 269.0f && t2.angulo_graus() < 271.0f);
}

TESTE(tela_boot_start_pausa_e_xis_troca_cor) {
    TelaBoot t;
    Controle c;
    passos(t, c, analogico(BT_START), 1);
    VERIFICA(t.pausado());
    f32 a = t.angulo_graus();
    passos(t, c, analogico(BT_START), 10);  // segurando: não alterna de novo
    VERIFICA(t.pausado() && t.angulo_graus() == a);
    passos(t, c, analogico(), 1);
    passos(t, c, analogico(BT_START), 1);
    VERIFICA(!t.pausado());
    passos(t, c, analogico(BT_XIS), 1);
    passos(t, c, analogico(), 1);
    passos(t, c, analogico(BT_XIS), 1);
    VERIFICA(t.indice_cor() == 2);
}

TESTE(tela_boot_l1_r1_limitam_tamanho) {
    TelaBoot t;
    Controle c;
    passos(t, c, analogico(BT_R1), 100);
    VERIFICA(t.escala_quadrado() == 1.5f);
    passos(t, c, analogico(BT_L1), 100);
    VERIFICA(t.escala_quadrado() == 0.5f);
}

TESTE(tela_boot_cabe_na_lista_e_na_area_segura) {
    TelaBoot t;
    Controle c;
    EstatisticasQuadro est;
    for (int i = 0; i < 60; i++) est.registrar(16683, 2500);
    InfoPlataforma info;
    snprintf(info.nome, sizeof info.nome, "PS2");
    snprintf(info.video, sizeof info.video, "NTSC 640×448 ENTRELAÇADO");
    info.memoria_livre = 30u * 1024 * 1024;
    info.tamanho_programa = 999u * 1024;
    static ListaDesenho2D l;
    const u16 combinacoes[] = {0, 0xffff};
    for (u16 b : combinacoes) {
        c.ler(analogico(b));
        c.iniciar_passo();
        t.montar(l, c, est, info);
        VERIFICA(l.descartadas() == 0);
        VERIFICA(l.tamanho() < ListaDesenho2D::CAPACIDADE / 2);  // folga para as próximas telas
        // Todo retângulo (texto, cartões, botões) fica dentro da tela; só as
        // rajadas de vento (quads) entram e saem pelas bordas.
        for (int i = 0; i < l.tamanho(); i++) {
            if (l[i].tipo != TipoPrim::RETANGULO) continue;
            VERIFICA(l[i].x[0] >= 0 && l[i].x[1] <= 606 && l[i].y[0] >= 0 && l[i].y[1] <= 448);
        }
    }
    EstadoControle d;
    c.ler(d);
    t.montar(l, c, est, info);
    VERIFICA(l.descartadas() == 0);
}

TESTE(tela_boot_rasteriza_com_cores_do_tema) {
    TelaBoot t;
    Controle c;
    EstatisticasQuadro est;
    InfoPlataforma info;
    static ListaDesenho2D l;
    static Rasterizador r;
    c.ler(analogico());
    t.montar(l, c, est, info);
    r.desenhar(l);
    // Canto superior esquerdo: azul-petróleo noturno; canto inferior direito: índigo.
    u32 topo = r.pixel(2, 2), base = r.pixel(637, 445);
    VERIFICA(((topo >> 8) & 255) > ((topo >> 16) & 255));  // mais verde que vermelho (teal)
    VERIFICA((base & 255) > ((base >> 8) & 255));          // mais azul que verde (índigo)
    // O centro do quadrado (130, 233) está pintado de coral (cor 0).
    VERIFICA(r.pixel(130, 233) == 0xff6b5b);
}
