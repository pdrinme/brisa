// Relevo desenhável da ilha, gerado na carga a partir do BMAP (alturas + cores
// do chão), sem malha pronta no pacote (BRISA_ARCHITECTURE §18):
// - blocos de 30 m (os setores do mapa); quadrados todos no fundo do mar não existem
//   (como no Ventania);
// - LOD1: quadrados de 6 m (50 triângulos por bloco), todos montados na carga;
// - LOD0: quadrados de 3 m (200 triângulos, em 4 partes de 15 m), montados sob
//   demanda num cache em volta da câmera;
// - as bordas do LOD0 usam só as amostras pares (as do LOD1): sem frestas entre LODs.
// Normais suaves por amostra e cores do terreno.gdshader assadas por vértice.
#pragma once
#include "mapa.h"
#include "nucleo/arena.h"
#include "render3d/cena3d.h"

struct EstatisticasTerreno {
    u32 blocos_lod0 = 0, blocos_lod1 = 0, montados = 0, triangulos = 0;
};

class Terreno {
public:
    static const int BLOCO = 10;            // amostras por bloco (30 m)
    static const int MAX_BLOCOS = 20 * 20;
    static const int CACHE_LOD0 = 28;
    static const int MONTAR_POR_QUADRO = 4;
    f32 dist_lod0 = 75.0f, dist_max = 250.0f;
    bool iniciar(const MapaBrisa& mapa, Arena& arena);
    // Põe os blocos visíveis na cena e o mar em volta.
    void montar(Cena3D& cena, const Frustum& fr, Vec3 camera, EstatisticasTerreno& est);
    int blocos() const { return n_blocos_; }
    u32 bytes() const { return bytes_; }
    const Malha& lod1(int bi, int bj) const { return lod1_[bj * lado_ + bi]; }
    // Monta (se preciso) e devolve as 4 partes do LOD0 do bloco; nullptr se o cache estiver ocupado.
    const Malha* lod0(int bi, int bj, u32 quadro);
private:
    struct Slot { int bloco = -1; u32 usado = 0; Malha partes[4]; i16* pos = nullptr; u32* nrm = nullptr; u32* cor = nullptr; };
    u32 gerar(int bi, int bj, int passo, int i0, int j0, int n, i16* pos, u32* nrm, u32* cor) const;
    void vertice(int i, int j, int passo, f32 ox, f32 oz, i16* pos, u32* nrm, u32* cor) const;
    const MapaBrisa* mapa_ = nullptr;
    int lado_ = 0, n_blocos_ = 0;
    Malha lod1_[MAX_BLOCOS];
    bool tem_[MAX_BLOCOS];
    Vec3 centro_[MAX_BLOCOS];
    f32 raio_[MAX_BLOCOS];
    Slot cache_[CACHE_LOD0];
    u32 quadro_ = 0, montados_quadro_ = 0;
    Malha mar_;
    bool tem_mar_[12 * 12];
    u32 bytes_ = 0;
};
