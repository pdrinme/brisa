#include "render_ee.h"

#include <dmaKit.h>
#include <kernel.h>

#include "gs_pacotes.h"

namespace {
const u32 QW_BUFFER = 12288;                       // 192 KB por buffer
const u32 MAX_VERTS_LOTE = 3 * 2000;               // NLOOP ≤ 32767
alignas(64) u64 g_buf[2][QW_BUFFER * 2];
bool g_enviando = false;

void esperar_gif() {
    if (g_enviando) {
        dmaKit_wait(DMA_CHANNEL_GIF, 0);
        g_enviando = false;
    }
}
}  // namespace

void SaidaGif::enviar_estado(const Cena3D& cena) {
    alignas(64) static u64 p[6];
    p[0] = gs::giftag_lo(2, true, false, 0, 1);
    p[1] = gs::REG_AD;
    p[2] = gs::test_z_gequal();
    p[3] = gs::ADDR_TEST_1;
    p[4] = gs::fogcol(cena.neblina.cor.r, cena.neblina.cor.g, cena.neblina.cor.b);
    p[5] = gs::ADDR_FOGCOL;
    dmaKit_wait(DMA_CHANNEL_GIF, 0);
    dmaKit_send(DMA_CHANNEL_GIF, p, 3);
    dmaKit_wait(DMA_CHANNEL_GIF, 0);
}

void SaidaGif::iniciar(f32 perto) {
    perto_ = perto;
    usado_ = 0;
    verts_ = 0;
    abrir_lote();
}

void SaidaGif::abrir_lote() {
    tag_ = usado_;
    u64* q = g_buf[buf_] + usado_ * 2;
    q[0] = 0;
    q[1] = gs::REG_RGBAQ | (gs::REG_XYZF2 << 4);
    usado_++;
    verts_ = 0;
}

void SaidaGif::fechar_e_enviar() {
    u64* q = g_buf[buf_] + tag_ * 2;
    if (verts_ == 0) {
        usado_ = tag_;  // lote vazio: descarta o GIFtag
    } else {
        q[0] = gs::giftag_lo(verts_, true, true, gs::PRIM_TRI_GOURAUD_NEBLINA, 2);
    }
    if (usado_ > 0) {
        esperar_gif();
        dmaKit_send(DMA_CHANNEL_GIF, g_buf[buf_], usado_);  // faz SyncDCache do buffer
        g_enviando = true;
        buf_ ^= 1;
    }
    usado_ = 0;
    verts_ = 0;
}

void SaidaGif::vertice(const VerticeTela& v) {
    u64* q = g_buf[buf_] + usado_ * 2;
    q[0] = v.cor.r | (static_cast<u64>(v.cor.g) << 32);
    q[1] = v.cor.b | (static_cast<u64>(0x80) << 32);
    u32 x = static_cast<u32>((2048.0f + v.x) * 16.0f);
    u32 y = static_cast<u32>((2048.0f + v.y) * 16.0f);
    u32 z = gs3d::z_de(v.q, perto_);
    u32 f = static_cast<u32>(v.neblina);
    q[2] = x | (static_cast<u64>(y) << 32);
    q[3] = (static_cast<u64>(z & 0xffffff) << 4) | (static_cast<u64>(f & 0xff) << 36);
    usado_ += 2;
    verts_++;
}

void SaidaGif::triangulo(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c) {
    if (usado_ + 6 >= QW_BUFFER || verts_ + 3 > MAX_VERTS_LOTE) {
        if (usado_ + 6 >= QW_BUFFER) {
            fechar_e_enviar();
        } else {
            // fecha o lote atual dentro do mesmo buffer e abre outro
            g_buf[buf_][tag_ * 2] = gs::giftag_lo(verts_, true, true, gs::PRIM_TRI_GOURAUD_NEBLINA, 2);
        }
        abrir_lote();
    }
    vertice(a);
    vertice(b);
    vertice(c);
}

void SaidaGif::terminar() {
    fechar_e_enviar();
    esperar_gif();
}
