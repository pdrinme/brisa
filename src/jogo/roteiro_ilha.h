// Roteiro do autoteste da ilha (Etapa 5), o mesmo no PC e no PS2: vistas fixas
// para medir o desenho (cada POI chegando de fora e olhando de dentro para o
// centro da ilha, e o alto do Mirante nas 4 direções) e uma caminhada no relevo.
#pragma once
#include <math.h>
#include <stdio.h>

#include "ilha.h"

struct VistaIlha {
    char nome[40];
    f32 x, z, yaw, pitch;
};

// Monta as vistas a partir dos POIs do mapa. Devolve quantas.
inline int vistas_da_ilha(const MapaBrisa& m, VistaIlha* v, int max) {
    int n = 0;
    auto yaw_para = [](f32 dx, f32 dz) { return atan2f(-dx, -dz) * 57.29578f; };  // frente = (−sen, −cos)
    for (int p = 0; p < m.n_pois() && n + 2 <= max; p++) {
        const PoiMapa& q = m.poi(p);
        f32 d = sqrtf(q.x * q.x + q.z * q.z);
        f32 ux = d > 1 ? -q.x / d : 1, uz = d > 1 ? -q.z / d : 0;  // do POI para o centro da ilha
        VistaIlha& a = v[n++];
        snprintf(a.nome, sizeof a.nome, "%s_chegando", q.id);
        a.x = q.x + ux * q.raio * 1.15f;
        a.z = q.z + uz * q.raio * 1.15f;
        a.yaw = yaw_para(q.x - a.x, q.z - a.z);
        a.pitch = -8;
        VistaIlha& b = v[n++];
        snprintf(b.nome, sizeof b.nome, "%s_para_o_centro", q.id);
        b.x = q.x;
        b.z = q.z;
        b.yaw = yaw_para(ux, uz);
        b.pitch = -8;
    }
    // Alto do Mirante (o ponto mais alto do centro): vê a ilha quase toda.
    for (int k = 0; k < 4 && n < max; k++) {
        VistaIlha& a = v[n++];
        snprintf(a.nome, sizeof a.nome, "mirante_%d", k * 90);
        a.x = 10;
        a.z = 5;
        a.yaw = k * 90.0f;
        a.pitch = -12;
    }
    return n;
}

const u32 PASSOS_POR_VISTA = 16;   // o cache do LOD0 enche (4 blocos por quadro) antes de medir
const u32 PASSOS_CAMINHADA = 150;  // correndo a partir do início, sobre o relevo
