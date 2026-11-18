#include "textura.h"

#include <stdio.h>
#include <string.h>

bool TexturaBrisa::carregar(const u8* d, u32 t, char* erro, int ne) {
    if (!d || t < 64 || memcmp(d, "BTEX", 4) != 0) { snprintf(erro, ne, "textura sem assinatura BTEX"); return false; }
    u32 versao;
    memcpy(&versao, d + 4, 4);
    if (versao != VERSAO) { snprintf(erro, ne, "versão da textura %u", static_cast<unsigned>(versao)); return false; }
    memcpy(&largura, d + 8, 2);
    memcpy(&altura, d + 10, 2);
    psm = d[12];
    memcpy(&n_cores, d + 14, 2);
    u32 off_pix, off_clut;
    memcpy(&off_pix, d + 16, 4);
    memcpy(&off_clut, d + 20, 4);
    memcpy(&bytes_pixels, d + 24, 4);
    if ((psm != PSM_T8 && psm != PSM_T4) || off_clut + n_cores * 4u > t || off_pix + bytes_pixels > t) {
        snprintf(erro, ne, "textura inválida");
        return false;
    }
    pixels = d + off_pix;
    clut = reinterpret_cast<const u32*>(d + off_clut);
    return true;
}

u8 TexturaBrisa::indice(int x, int y) const {
    int i = y * largura + x;
    if (psm == PSM_T8) return pixels[i];
    u8 b = pixels[i >> 1];
    return (i & 1) ? (b >> 4) : (b & 15);
}

u32 TexturaBrisa::rgba(int x, int y) const {
    u32 i = indice(x, y);
    if (psm == PSM_T8) {
        // CSM1: dentro de cada bloco de 32, as entradas 8–15 e 16–23 estão trocadas.
        u32 b = i & 0x18;
        if (b == 0x08) i += 8;
        else if (b == 0x10) i -= 8;
    }
    return clut[i];
}
