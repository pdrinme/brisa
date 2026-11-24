// Plataforma PS2: IOP e módulos, vídeo (gsKit), DualShock 2 (libpad), relógio.
#include "plataforma/plataforma.h"

#include <dmaKit.h>
#include <gsKit.h>
#include <gsInline.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <libpad.h>
#include <loadfile.h>
#include <malloc.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>
#include <timer.h>
#include <unistd.h>

#include "irx_embutidos.h"
#include "render_ee.h"
#include "render_vu1.h"

extern "C" char _end[];  // fim do programa (linkfile do PS2SDK)
extern "C" const u8 brisa_pak_inicio[];  // conteudo_embutido.S
extern "C" const u8 brisa_pak_fim[];

namespace {

GSGLOBAL* g_gs = nullptr;
char g_buffer_pad[256] __attribute__((aligned(64)));
bool g_pad_aberto = false;
bool g_vu1 = false;
const int MAX_TEXTURAS = 64;
GSTEXTURE g_tex[MAX_TEXTURAS];
int g_n_tex = 0;

// Sombras blob pelo gsKit (STQ, textura com alfa, teste de Z ligado).
struct SaidaSombras : SaidaTriangulos {
    f32 perto = 0.5f;
    void triangulo(const VerticeTela&, const VerticeTela&, const VerticeTela&) override {}
    void triangulo_textura(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c, int tex) override {
        if (tex < 0 || tex >= g_n_tex) return;
        GSPRIMSTQPOINT v[3];
        const VerticeTela* vs[3] = {&a, &b, &c};
        for (int i = 0; i < 3; i++) {
            const VerticeTela& p = *vs[i];
            v[i].rgbaq = color_to_RGBAQ(0x80, 0x80, 0x80, 0x80, p.q);
            v[i].stq = vertex_to_STQ(p.s * p.q, p.t * p.q);
            v[i].xyz2 = vertex_to_XYZ2(g_gs, p.x, p.y, static_cast<int>(gs3d::z_de(p.q, perto)));
        }
        gsKit_prim_list_triangle_goraud_texture_stq_3d(g_gs, &g_tex[tex], 3, v);
    }
    static u64 rgbaq(Cor k) { return GS_SETREG_RGBAQ(k.r, k.g, k.b, (k.a + 1) >> 1, 0x00); }
    void triangulo_alfa(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c) override {
        int za = static_cast<int>(gs3d::z_de(a.q, perto)), zb = static_cast<int>(gs3d::z_de(b.q, perto)),
            zc = static_cast<int>(gs3d::z_de(c.q, perto));
        gsKit_prim_triangle_gouraud_3d(g_gs, a.x, a.y, za, b.x, b.y, zb, c.x, c.y, zc, rgbaq(a.cor), rgbaq(b.cor), rgbaq(c.cor));
    }
};
bool g_pad_modo_pedido = false;
int g_pad_ultimo_estado = -1;

void copiar(char* dst, size_t n, const char* src) {
    strncpy(dst, src, n - 1);
    dst[n - 1] = 0;
}

// Carrega SIO2MAN e PADMAN. Ordem padrão: os módulos da ROM do console
// (XSIO2MAN/XPADMAN e, nos BIOS antigos, SIO2MAN/PADMAN), que o libpad atual
// aceita nos dois protocolos; os IRX do PS2SDK embutidos no ELF ficam como
// última alternativa. Motivo (Etapa 1, medido): no emulador Play!, o PADMAN de
// código aberto embutido fica preso em PAD_STATE_EXECCMD, e os da ROM funcionam.
// -DBRISA_PAD_EMBUTIDO inverte a ordem para comparar no console.
bool carregar_embutidos(InfoPlataforma& info) {
    int res = 0;
    int r1 = SifExecModuleBuffer(sio2man_irx, size_sio2man_irx, 0, nullptr, &res);
    int r2 = r1 >= 0 ? SifExecModuleBuffer(padman_irx, size_padman_irx, 0, nullptr, &res) : -1;
    if (r1 >= 0 && r2 >= 0) copiar(info.modulos, sizeof info.modulos, "IRX EMBUTIDOS (SIO2MAN+PADMAN)");
    return r1 >= 0 && r2 >= 0;
}

bool carregar_rom(InfoPlataforma& info) {
    if (SifLoadModule("rom0:XSIO2MAN", 0, nullptr) >= 0 && SifLoadModule("rom0:XPADMAN", 0, nullptr) >= 0) {
        copiar(info.modulos, sizeof info.modulos, "ROM (XSIO2MAN+XPADMAN)");
        return true;
    }
    if (SifLoadModule("rom0:SIO2MAN", 0, nullptr) >= 0 && SifLoadModule("rom0:PADMAN", 0, nullptr) >= 0) {
        copiar(info.modulos, sizeof info.modulos, "ROM (SIO2MAN+PADMAN)");
        return true;
    }
    return false;
}

void carregar_modulos(InfoPlataforma& info) {
#ifdef BRISA_PAD_EMBUTIDO
    if (carregar_embutidos(info) || carregar_rom(info)) return;
#else
    if (carregar_rom(info) || carregar_embutidos(info)) return;
#endif
    copiar(info.modulos, sizeof info.modulos, "FALHA AO CARREGAR");
}

// Memória livre para o heap: do fim atual do heap (sbrk) até o fim do heap do
// kernel (EndOfHeap, abaixo da pilha), confirmada por uma busca binária com
// malloc limitada a esse intervalo (o maior bloco que o malloc entrega agora).
u32 medir_memoria_livre(char* detalhe, size_t n) {
    uintptr_t topo = reinterpret_cast<uintptr_t>(EndOfHeap());
    uintptr_t atual = reinterpret_cast<uintptr_t>(sbrk(0));
    u32 teorico = topo > atual ? static_cast<u32>(topo - atual) : 0;
    snprintf(detalhe, n, "fim do programa %p, sbrk %p, EndOfHeap %p, RAM %d MB", static_cast<void*>(_end),
             reinterpret_cast<void*>(atual), reinterpret_cast<void*>(topo), GetMemorySize() / (1024 * 1024));
    u32 lo = 0, hi = teorico;
    while (hi - lo > 16 * 1024) {
        u32 meio = lo + (hi - lo) / 2;
        void* p = malloc(meio);
        if (p) { free(p); lo = meio; } else { hi = meio; }
    }
    return lo;
}

u32 cor_gs(Cor c) {
    // O GS usa alfa 0..128 (128 = opaco).
    return static_cast<u32>(GS_SETREG_RGBAQ(c.r, c.g, c.b, (c.a + 1) >> 1, 0x00));
}

}  // namespace

namespace plataforma {

bool iniciar(InfoPlataforma& info, int argc, char** argv) {
    copiar(info.nome, sizeof info.nome, "PS2");
    copiar(info.caminho_boot, sizeof info.caminho_boot, argc > 0 && argv && argv[0] ? argv[0] : "?");

    // IOP limpo: descarta módulos deixados pelo carregador (OPL, uLaunchELF, ps2link).
    SifInitRpc(0);
#ifndef BRISA_SEM_RESET_IOP
    while (!SifIopReset("", 0)) {}
    while (!SifIopSync()) {}
    SifInitRpc(0);
#endif
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    carregar_modulos(info);

    padInit(0);
    g_pad_aberto = padPortOpen(0, 0, g_buffer_pad) != 0;
    printf("BRISA: padInit/padPortOpen porta 1: %s\n", g_pad_aberto ? "aberta" : "FALHOU");

    // Vídeo: modo do console (NTSC 640×448 ou PAL 640×512), entrelaçado, buffer duplo.
    g_gs = gsKit_init_global();
#ifdef BRISA_FORCAR_NTSC
    g_gs->Mode = GS_MODE_NTSC;
    g_gs->Height = 448;
#endif
    g_gs->PSM = GS_PSM_CT24;
    g_gs->ZBuffering = GS_SETTING_ON;   // Etapa 2: Z de 24 bits (BRISA_ARCHITECTURE §5.1)
    g_gs->PSMZ = GS_PSMZ_24;
    g_gs->DoubleBuffering = GS_SETTING_ON;
    g_gs->PrimAlphaEnable = GS_SETTING_ON;
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_VIF1);
    gsKit_init_screen(g_gs);
    gsKit_mode_switch(g_gs, GS_ONESHOT);
    gsKit_set_test(g_gs, GS_ZTEST_OFF);
    gsKit_set_primalpha(g_gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);

    g_vu1 = vu1::iniciar();
    printf("BRISA: VU1 %s\n", g_vu1 ? "pronto (microprograma toon)" : "FALHOU");
    info.largura = g_gs->Width;
    info.altura = g_gs->Height;
    bool pal = g_gs->Mode == GS_MODE_PAL;
    info.hz_video = pal ? 50 : 60;
    snprintf(info.video, sizeof info.video, "%s %d×%d %s", pal ? "PAL" : "NTSC", g_gs->Width, g_gs->Height,
             g_gs->Interlace == GS_INTERLACED ? "ENTRELAÇADO" : "PROGRESSIVO");
    info.tamanho_programa = static_cast<u32>(reinterpret_cast<uintptr_t>(_end) - 0x00100000u);
    info.memoria_livre = medir_memoria_livre(info.memoria_detalhe, sizeof info.memoria_detalhe);
    return true;
}

void ler_controle(EstadoControle& e) {
    e = EstadoControle();
    if (!g_pad_aberto) return;
    int estado = padGetState(0, 0);
    if (estado != g_pad_ultimo_estado) {
        printf("BRISA: controle 1 estado %d -> %d\n", g_pad_ultimo_estado, estado);
        g_pad_ultimo_estado = estado;
    }
    if (estado != PAD_STATE_STABLE && estado != PAD_STATE_FINDCTP1) {
        if (estado == PAD_STATE_DISCONN) g_pad_modo_pedido = false;
        return;
    }
    int tipo = padInfoMode(0, 0, PAD_MODECURID, 0);
    if (!g_pad_modo_pedido && estado == PAD_STATE_STABLE) {
        // Pede o modo analógico (DualShock) uma vez por conexão e trava.
        if (padInfoMode(0, 0, PAD_MODETABLE, -1) > 0) padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
        g_pad_modo_pedido = true;
    }
    padButtonStatus b;
    if (padRead(0, 0, &b) == 0) return;
    // Os bits do libpad (ativos em 0) seguem a mesma ordem do enum Botao.
    e.botoes = static_cast<u16>(0xffff ^ b.btns);
    bool analogico = tipo == PAD_TYPE_DUALSHOCK || tipo == PAD_TYPE_ANALOG;
    e.modo = analogico ? ModoControle::ANALOGICO : ModoControle::DIGITAL;
    if (analogico) {
        e.lx = b.ljoy_h; e.ly = b.ljoy_v;
        e.rx = b.rjoy_h; e.ry = b.rjoy_v;
    }
}

u64 tempo_us() {
    u64 c = GetTimerSystemTime();  // relógio do barramento (147,456 MHz)
    return c / (kBUSCLK / 1000000);
}

static void desenhar_2d(const ListaDesenho2D& l) {
    for (int i = 0; i < l.tamanho(); i++) {
        const Prim& p = l[i];
        switch (p.tipo) {
        case TipoPrim::RETANGULO:
            gsKit_prim_sprite(g_gs, p.x[0], p.y[0], p.x[1], p.y[1], 1, cor_gs(p.cor[0]));
            break;
        case TipoPrim::GRADIENTE:
            gsKit_prim_quad_gouraud(g_gs, p.x[0], p.y[0], p.x[1], p.y[0], p.x[0], p.y[1], p.x[1], p.y[1], 1,
                                    cor_gs(p.cor[0]), cor_gs(p.cor[1]), cor_gs(p.cor[2]), cor_gs(p.cor[3]));
            break;
        case TipoPrim::SPRITE_TEX:
            if (p.textura >= 0 && p.textura < g_n_tex) {
                const GSTEXTURE& t = g_tex[p.textura];
                f32 w = static_cast<f32>(t.Width), h = static_cast<f32>(t.Height);
                gsKit_prim_sprite_texture(g_gs, &t, p.x[0], p.y[0], p.x[2] * w, p.y[2] * h, p.x[1], p.y[1], p.x[3] * w,
                                          p.y[3] * h, 1,
                                          GS_SETREG_RGBAQ(p.cor[0].r, p.cor[0].g, p.cor[0].b, p.cor[0].a, 0x00));
            }
            break;
        case TipoPrim::QUAD:
            gsKit_prim_quad(g_gs, p.x[0], p.y[0], p.x[1], p.y[1], p.x[2], p.y[2], p.x[3], p.y[3], 1, cor_gs(p.cor[0]));
            break;
        }
    }
}

void desenhar_e_apresentar(const ListaDesenho2D& l) {
    gsKit_clear(g_gs, cor_gs(l.fundo()));
    gsKit_set_test(g_gs, GS_ZTEST_OFF);
    desenhar_2d(l);
    gsKit_queue_exec(g_gs);
    gsKit_sync_flip(g_gs);
}

bool caminho_disponivel(Caminho3D c) { return c == Caminho3D::EE || g_vu1; }

const u8* conteudo(u32* tamanho) {
    if (tamanho) *tamanho = static_cast<u32>(brisa_pak_fim - brisa_pak_inicio);
    return brisa_pak_inicio;
}

int registrar_textura(const TexturaBrisa& t) {
    if (g_n_tex >= MAX_TEXTURAS) return -1;
    GSTEXTURE& g = g_tex[g_n_tex];
    memset(&g, 0, sizeof g);
    g.Width = t.largura;
    g.Height = t.altura;
    g.PSM = t.psm == TexturaBrisa::PSM_T8 ? GS_PSM_T8 : GS_PSM_T4;
    g.ClutPSM = GS_PSM_CT32;
    g.Mem = reinterpret_cast<u32*>(const_cast<u8*>(t.pixels));
    g.Clut = const_cast<u32*>(t.clut);
    g.Filter = GS_FILTER_LINEAR;
    g.Vram = gsKit_vram_alloc(g_gs, gsKit_texture_size(g.Width, g.Height, g.PSM), GSKIT_ALLOC_USERBUFFER);
    g.VramClut = gsKit_vram_alloc(g_gs, gsKit_texture_size(g.PSM == GS_PSM_T8 ? 16 : 8, g.PSM == GS_PSM_T8 ? 16 : 2, GS_PSM_CT32),
                                  GSKIT_ALLOC_USERBUFFER);
    if (g.Vram == GSKIT_ALLOC_ERROR || g.VramClut == GSKIT_ALLOC_ERROR) return -1;
    FlushCache(0);
    gsKit_texture_upload(g_gs, &g);
    return g_n_tex++;
}

void desenhar_quadro(const Cena3D& cena, Caminho3D caminho, const ListaDesenho2D& hud, Estatisticas3D& est) {
    est = Estatisticas3D();
    // 1) limpa cor e Z (fila do gsKit) e espera terminar
    gsKit_clear(g_gs, cor_gs(cena.ceu));
    gsKit_queue_exec(g_gs);
    u64 t0 = tempo_us();
    // 2) estado do 3D (TEST Z, cor da neblina) e o caminho escolhido
    SaidaGif::enviar_estado(cena);
    SaidaGif gif;
    gif.iniciar(cena.camera.perto);
    if (caminho == Caminho3D::VU1 && g_vu1) {
        vu1::desenhar(cena, gif, est);
    } else {
        pipeline_cena(cena, ConfigTela(), gif, est);
    }
    gif.terminar();
    est.tempo_3d_us = static_cast<u32>(tempo_us() - t0);
    // 3) sombras blob (textura com alfa, Z ligado) e o HUD por cima, sem Z
    gsKit_set_test(g_gs, GS_ZTEST_ON);
    SaidaSombras sombras;
    sombras.perto = cena.camera.perto;
    projetar_sombras(cena, ConfigTela(), sombras);
    projetar_parede(cena, ConfigTela(), sombras);  // Etapa 8: parede da Tormenta, por último
    gsKit_set_test(g_gs, GS_ZTEST_OFF);
    desenhar_2d(hud);
    gsKit_queue_exec(g_gs);
    gsKit_sync_flip(g_gs);
}

void log(const char* texto) {
    printf("%s", texto);
    fflush(stdout);
}

void encerrar() {
    // Sem RPC ao PADMAN na saída: no emulador Play!, com o PADMAN embutido,
    // padPortClose() e padEnd() não retornam (Etapa 1, medido). Quem carrega o
    // próximo programa reinicia o IOP, o que fecha a porta de qualquer forma.
    g_pad_aberto = false;
}

}  // namespace plataforma
