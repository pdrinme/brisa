// O Ônibus dos Ventos (onibus.gd do Ventania): rota reta sobre a ilha, sorteada a
// cada partida. Entra e sai sobre o mar e passa a até 80 m do centro; as portas
// abrem quando ele está sobre terra e fecham um pouco antes de deixar a ilha
// (quem não saltou é ejetado ali). Altitude fixa, acima de tudo no mapa.
#pragma once
#include "nucleo/mat.h"

class Onibus {
public:
    static constexpr f32 VELOCIDADE = 20.0f;   // m/s
    static constexpr f32 ALTITUDE = 120.0f;
    static constexpr f32 META_TERRA = 240.0f;  // trecho mínimo sobre terra (m)
    typedef bool (*Terra)(const void* ctx, f32 x, f32 z);
    void sortear(u32 semente, u32 tick_inicio, f32 limite, Terra terra, const void* ctx);
    Vec3 posicao(u32 tick) const;
    Vec3 direcao() const;
    f32 yaw() const;                           // rad, do jeito da câmera (0 = −z)
    bool portas_abertas(u32 tick) const { return tick >= abre && tick < fecha; }
    bool terminou(u32 tick) const { return tick >= fecha; }
    Vec3 inicio, fim;
    u32 tick0 = 0, duracao = 1, abre = 0, fecha = 0;
};
