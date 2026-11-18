#include "estatisticas.h"

void EstatisticasQuadro::registrar(u32 quadro_us, u32 trabalho_us) {
    u32 i = n_ % JANELA;
    quadro_[i] = quadro_us;
    trabalho_[i] = trabalho_us;
    n_++;
    if (trabalho_us > pior_trabalho_total_) pior_trabalho_total_ = trabalho_us;
}

static u32 media(const u32* v, u32 n) {
    if (n == 0) return 0;
    u64 s = 0;
    for (u32 i = 0; i < n; i++) s += v[i];
    return static_cast<u32>(s / n);
}

static u32 maximo(const u32* v, u32 n) {
    u32 m = 0;
    for (u32 i = 0; i < n; i++) if (v[i] > m) m = v[i];
    return m;
}

u32 EstatisticasQuadro::media_quadro_us() const { return media(quadro_, amostras()); }
u32 EstatisticasQuadro::media_trabalho_us() const { return media(trabalho_, amostras()); }
u32 EstatisticasQuadro::pior_quadro_us() const { return maximo(quadro_, amostras()); }
u32 EstatisticasQuadro::pior_trabalho_us() const { return maximo(trabalho_, amostras()); }

u32 EstatisticasQuadro::fps_x10() const {
    u32 m = media_quadro_us();
    return m ? static_cast<u32>((10000000ull + m / 2) / m) : 0;
}
