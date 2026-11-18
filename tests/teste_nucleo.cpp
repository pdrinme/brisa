#include <string.h>

#include "nucleo/arena.h"
#include "nucleo/controle.h"
#include "nucleo/desenho2d.h"
#include "nucleo/estatisticas.h"
#include "nucleo/fonte.h"
#include "nucleo/formato.h"
#include "nucleo/passo_fixo.h"
#include "teste.h"

TESTE(arena_alinha_e_recusa_sem_estourar) {
    alignas(16) static unsigned char mem[256];
    Arena a(mem, sizeof mem);
    void* p1 = a.alocar(10, 16);
    void* p2 = a.alocar(1, 64);
    VERIFICA(p1 == mem);
    VERIFICA(p2 != nullptr && (reinterpret_cast<uintptr_t>(p2) & 63) == 0);
    VERIFICA(a.alocar(1000) == nullptr);   // não cabe: nullptr, sem mudar o uso
    size_t usado = a.usado();
    VERIFICA(a.alocar(3, 3) == nullptr);   // alinhamento inválido
    VERIFICA(a.usado() == usado);
    size_t m = a.marca();
    VERIFICA(a.alocar(16) != nullptr);
    a.voltar(m);
    VERIFICA(a.usado() == m);
    VERIFICA(a.pico() >= m + 16);
    a.limpar();
    VERIFICA(a.usado() == 0 && a.livre() == sizeof mem);
}

TESTE(passo_fixo_30hz_com_vsync_60hz) {
    PassoFixo p(30, 2);
    u32 soma = 0;
    for (int i = 0; i < 600; i++) soma += p.avancar(16683);  // 10 s de NTSC
    VERIFICA(soma >= 299 && soma <= 301);
}

TESTE(passo_fixo_limita_a_dois_passos_e_descarta) {
    PassoFixo p(30, 2);
    VERIFICA(p.avancar(1000000) == 2);  // 1 s parado (ex.: carregamento)
    VERIFICA(p.descartado_us() > 900000);
    VERIFICA(p.avancar(0) == 0);
    VERIFICA(p.avancar(p.passo_us()) == 1);
}

TESTE(controle_nao_perde_nem_duplica_toques) {
    Controle c;
    EstadoControle e;
    e.modo = ModoControle::ANALOGICO;
    e.botoes = BT_XIS;
    c.ler(e);              // quadro sem passo: o toque fica pendente
    EstadoControle solto = e;
    solto.botoes = 0;
    c.ler(solto);          // soltou antes do passo
    c.iniciar_passo();
    VERIFICA(c.pressionou(BT_XIS));
    c.iniciar_passo();     // segundo passo do mesmo quadro: não repete
    VERIFICA(!c.pressionou(BT_XIS));
    c.ler(e);
    c.ler(e);              // segurando não gera toque novo
    c.iniciar_passo();
    VERIFICA(c.pressionou(BT_XIS));
    c.ler(e);
    c.iniciar_passo();
    VERIFICA(!c.pressionou(BT_XIS));
    VERIFICA(c.segurando(BT_XIS));
}

TESTE(controle_eixo_zona_morta) {
    VERIFICA(Controle::eixo(128) == 0.0f);
    VERIFICA(Controle::eixo(140) == 0.0f);
    VERIFICA(Controle::eixo(255) == 1.0f);
    VERIFICA(Controle::eixo(0) == -1.0f);
    f32 m = Controle::eixo(200);
    VERIFICA(m > 0.3f && m < 0.8f);
}

TESTE(bits_do_controle_iguais_ao_libpad) {
    // PAD_* de libpad.h (PS2SDK): a plataforma PS2 copia os bits direto.
    VERIFICA(BT_SELECT == 0x0001 && BT_L3 == 0x0002 && BT_R3 == 0x0004 && BT_START == 0x0008);
    VERIFICA(BT_CIMA == 0x0010 && BT_DIREITA == 0x0020 && BT_BAIXO == 0x0040 && BT_ESQUERDA == 0x0080);
    VERIFICA(BT_L2 == 0x0100 && BT_R2 == 0x0200 && BT_L1 == 0x0400 && BT_R1 == 0x0800);
    VERIFICA(BT_TRIANGULO == 0x1000 && BT_CIRCULO == 0x2000 && BT_XIS == 0x4000 && BT_QUADRADO == 0x8000);
}

TESTE(estatisticas_media_pior_fps) {
    EstatisticasQuadro e;
    for (int i = 0; i < 100; i++) e.registrar(16683, i == 50 ? 9000 : 1000);
    VERIFICA(e.media_quadro_us() == 16683);
    VERIFICA(e.fps_x10() == 599);
    VERIFICA(e.pior_trabalho_total_us() == 9000);
    VERIFICA(e.amostras() == EstatisticasQuadro::JANELA);
    VERIFICA(e.pior_trabalho_us() == 9000);  // o quadro 50 está na janela (40..99)
}

TESTE(formato_decimal_com_virgula) {
    char b[16];
    formato::decimal(b, sizeof b, 16683, 1000, 1);
    VERIFICA(strcmp(b, "16,7") == 0);
    formato::decimal(b, sizeof b, 599, 10, 1);
    VERIFICA(strcmp(b, "59,9") == 0);
    formato::decimal(b, sizeof b, 0, 1000, 2);
    VERIFICA(strcmp(b, "0,00") == 0);
    formato::decimal(b, sizeof b, 29000, 1024, 0);
    VERIFICA(strcmp(b, "28") == 0);
    char c[8] = "AB";
    formato::juntar(c, sizeof c, "CDEFGHIJ");
    VERIFICA(strcmp(c, "ABCDEFG") == 0);
}

TESTE(fonte_tem_todos_os_caracteres_do_portugues) {
    u8 l[fonte::ALTURA];
    const char* todos = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !?.,:-+/()%='<>_ÁÀÂÃÉÊÍÓÔÕÚÇáàâãéêíóôõúç·×";
    const char* p = todos;
    u32 c;
    int n = 0;
    while ((c = fonte::proximo_codigo(&p)) != 0) {
        VERIFICA(fonte::glifo(c, l));
        n++;
    }
    VERIFICA(n == 79);  // 26 letras, 10 dígitos, espaço, 16 sinais, 24 acentuadas, · e ×
    VERIFICA(!fonte::glifo(0x4E2D, l));  // sem glifo: desenha '?'
    // Acento em cima e cedilha embaixo.
    fonte::glifo(0xC3, l);
    VERIFICA(l[0] != 0 && l[9] == 0);
    fonte::glifo(0xC7, l);
    VERIFICA(l[9] != 0);
    fonte::glifo('a', l);
    u8 m[fonte::ALTURA];
    fonte::glifo('A', m);
    VERIFICA(memcmp(l, m, sizeof l) == 0);
}

TESTE(fonte_largura_e_junta_pixels) {
    VERIFICA(fonte::largura("", 2) == 0);
    VERIFICA(fonte::largura("AB", 1) == 11);
    VERIFICA(fonte::largura("ÇÃO", 2) == (3 * 6 - 1) * 2);
    static ListaDesenho2D l;
    l.limpar(rgb(0));
    fonte::desenhar(l, 0, 0, 1, rgb(0xffffff), "T");
    // T = 1 traço de 5 px + 6 linhas de 1 px: 7 retângulos, não 11 pixels.
    VERIFICA(l.tamanho() == 7);
    // Adesivo = sombra + contorno (retângulos dilatados) + letra: 3 cópias, não 18.
    l.limpar(rgb(0));
    fonte::desenhar_adesivo(l, 10, 10, 2, rgb(0xffffff), rgb(0), 2, 3, "T");
    VERIFICA(l.tamanho() == 21);
    VERIFICA(l[7].x[0] == 10 - 2 && l[7].y[0] == 10 + 2 * 2 - 2);  // contorno do traço do T
}

TESTE(lista_descarta_sem_estourar) {
    static ListaDesenho2D l;
    l.limpar(rgb(0));
    for (int i = 0; i < ListaDesenho2D::CAPACIDADE + 10; i++) l.retangulo(0, 0, 1, 1, rgb(0xffffff));
    VERIFICA(l.tamanho() == ListaDesenho2D::CAPACIDADE);
    VERIFICA(l.descartadas() == 10);
    l.limpar(rgb(0));
    l.retangulo(0, 0, 0, 5, rgb(0xffffff));  // vazio: ignorado
    l.retangulo(0, 0, 5, 5, rgb(0xffffff, 0));  // transparente: ignorado
    VERIFICA(l.tamanho() == 0);
}
