// Textura do Brisa (BTEX): PSMT8 (CLUT de 256 já no arranjo CSM1 do GS) ou
// PSMT4 (CLUT de 16), cores RGBA com alfa na escala do GS (0..128).
#pragma once
#include "nucleo/tipos.h"

struct TexturaBrisa {
    static const u32 VERSAO = 1;
    static const u8 PSM_T8 = 0x13, PSM_T4 = 0x14;
    u16 largura = 0, altura = 0;
    u8 psm = 0;
    u16 n_cores = 0;
    const u8* pixels = nullptr;
    const u32* clut = nullptr;  // RGBA (r no byte baixo)
    u32 bytes_pixels = 0;
    bool carregar(const u8* dados, u32 tamanho, char* erro, int n_erro);
    // Índice do pixel (x, y) e a cor RGBA dele (para o PC; desfaz o CSM1).
    u8 indice(int x, int y) const;
    u32 rgba(int x, int y) const;
};
