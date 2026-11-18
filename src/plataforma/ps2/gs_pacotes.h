// Montagem de pacotes do GS (GIFtag e registradores), comum aos dois caminhos 3D.
#pragma once
#include "nucleo/tipos.h"

namespace gs {

const u64 FLG_PACKED = 0;
const u64 REG_RGBAQ = 0x1, REG_XYZF2 = 0x4, REG_AD = 0xE;
const u64 ADDR_TEST_1 = 0x47, ADDR_FOGCOL = 0x3D;
// PRIM: triângulo (3), Gouraud (bit 3), neblina (bit 5).
const u64 PRIM_TRI_GOURAUD_NEBLINA = 3 | (1 << 3) | (1 << 5);

inline u64 giftag_lo(u32 nloop, bool eop, bool pre, u64 prim, u64 nreg) {
    return (static_cast<u64>(nloop) & 0x7fff) | (static_cast<u64>(eop) << 15) | (static_cast<u64>(pre) << 46) |
           ((prim & 0x7ff) << 47) | (FLG_PACKED << 58) | ((nreg & 0xf) << 60);
}

// TEST_1: Z ligado, "maior ou igual" (perto = Z maior), sem teste de alfa.
inline u64 test_z_gequal() { return (1ull << 16) | (2ull << 17); }
inline u64 fogcol(u8 r, u8 g, u8 b) { return r | (static_cast<u64>(g) << 8) | (static_cast<u64>(b) << 16); }

// Coordenadas: pixel (0,0) da tela = (2048, 2048) no GS (XYOFFSET do gsKit).
const f32 CENTRO_X = 2048.0f + 320.0f, CENTRO_Y = 2048.0f + 224.0f;

}  // namespace gs
