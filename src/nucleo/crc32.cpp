#include "crc32.h"

static u32 g_tabela[256];
static bool g_pronta = false;

u32 crc32(const void* dados, size_t n, u32 c) {
    if (!g_pronta) {
        for (u32 i = 0; i < 256; i++) {
            u32 v = i;
            for (int k = 0; k < 8; k++) v = (v & 1) ? 0xEDB88320u ^ (v >> 1) : v >> 1;
            g_tabela[i] = v;
        }
        g_pronta = true;
    }
    const u8* p = static_cast<const u8*>(dados);
    c ^= 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = g_tabela[(c ^ p[i]) & 255] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
