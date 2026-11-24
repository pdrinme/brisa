#include "onibus.h"

#include <math.h>

#include "combate/itens.h"

void Onibus::sortear(u32 semente, u32 tick_inicio, f32 limite, Terra terra, const void* ctx) {
    Aleatorio r;
    r.semear(semente);
    for (int tentativa = 0; tentativa < 60; tentativa++) {
        f32 ang = r.unitario() * 6.2831853f;
        f32 dx = cosf(ang), dz = sinf(ang);
        f32 lx = -dz, lz = dx;
        f32 m = -80.0f + 160.0f * r.unitario();
        f32 ax = lx * m - dx * (limite + 40.0f), az = lz * m - dz * (limite + 40.0f);
        f32 bx = lx * m + dx * (limite + 40.0f), bz = lz * m + dz * (limite + 40.0f);
        inicio = Vec3(ax, ALTITUDE, az);
        fim = Vec3(bx, ALTITUDE, bz);
        tick0 = tick_inicio;
        f32 comp = sqrtf((bx - ax) * (bx - ax) + (bz - az) * (bz - az));
        int d = static_cast<int>(floorf(comp / VELOCIDADE * 30.0f + 0.5f));
        duracao = static_cast<u32>(d < 1 ? 1 : d);
        f32 primeiro = -1, ultimo = -1;
        const int n = 200;
        for (int k = 0; k <= n; k++) {
            f32 f = static_cast<f32>(k) / n;
            if (terra(ctx, ax + (bx - ax) * f, az + (bz - az) * f)) {
                if (primeiro < 0) primeiro = f;
                ultimo = f;
            }
        }
        if (primeiro < 0 || (ultimo - primeiro) * comp < META_TERRA) continue;
        abre = tick_inicio + static_cast<u32>(floorf((primeiro + 0.01f) * duracao + 0.5f));
        fecha = tick_inicio + static_cast<u32>(floorf((ultimo - 0.03f) * duracao + 0.5f));
        return;
    }
    abre = tick0;
    fecha = tick0 + duracao;
}

Vec3 Onibus::posicao(u32 tick) const {
    f32 k = (static_cast<f32>(tick) - static_cast<f32>(tick0)) / static_cast<f32>(duracao);
    k = k < 0 ? 0 : (k > 1 ? 1 : k);
    return inicio + (fim - inicio) * k;
}

Vec3 Onibus::direcao() const {
    Vec3 d = fim - inicio;
    f32 l = sqrtf(ponto(d, d));
    return l > 1e-6f ? d * (1.0f / l) : Vec3(1, 0, 0);
}

f32 Onibus::yaw() const {
    Vec3 d = direcao();
    return atan2f(-d.x, -d.z);
}
