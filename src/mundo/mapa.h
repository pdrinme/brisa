// Mapa do Brisa (BMAP, docs/FORMATOS.md): relevo, cores do chão, costa, tabela
// de modelos, setores de 30 m com as instâncias ordenadas por modelo, POIs com
// contorno e pontos de início. Lido do pacote sem cópia.
#pragma once
#include "colisao.h"

struct ModeloMapa {
    char nome[32];   // "modelos/<nome>.bmod" no pacote
    i32 lod;         // índice do LOD1 nesta tabela, ou −1
    f32 dist_lod;    // a partir daqui usa o LOD1 (m)
    f32 dist_max;    // além daqui não desenha (m)
    u32 categoria;   // CategoriaMapa
};
static_assert(sizeof(ModeloMapa) == 48, "ModeloMapa");

enum CategoriaMapa : u16 { CAT_PECA, CAT_ARVORE, CAT_PEQUENO, CAT_MOVEL, CAT_GRANDE, CAT_BAU, CAT_ESTRADA, N_CATEGORIAS };

struct SetorMapa {
    u32 inicio, n;   // instâncias [inicio, inicio + n)
    f32 ymin, ymax;  // alturas (relevo e instâncias)
};

struct InstanciaMapa {
    u16 modelo, categoria;
    f32 x, y, z;
    f32 rot;         // graus em Y
    f32 escala;
    f32 raio;        // raio em volta da origem (já com a escala)
    u32 _;
};
static_assert(sizeof(InstanciaMapa) == 32, "InstanciaMapa");

struct PoiMapa {
    char id[24];
    char nome[40];
    f32 x, y, z, raio;
    u32 cor;              // 0xRRGGBB
    u32 inicio, n_pontos; // contorno (x, z) em pontos()
    u32 _;
};
static_assert(sizeof(PoiMapa) == 96, "PoiMapa");

class MapaBrisa {
public:
    static const u32 VERSAO = 1;
    bool carregar(const u8* dados, u32 tamanho, char* erro, int n_erro);
    const Relevo& relevo() const { return relevo_; }
    f32 nivel_mar() const { return nivel_mar_; }
    u32 cor(int i, int j) const;        // cor do chão na amostra (r | g<<8 | b<<16 | 255<<24)
    int setores_lado() const { return s_; }
    f32 tam_setor() const { return tam_setor_; }
    f32 origem() const { return relevo_.origem; }
    const SetorMapa& setor(int i, int j) const { return setores_[j * s_ + i]; }
    int n_modelos() const { return n_mod_; }
    const ModeloMapa& modelo(int i) const { return mod_[i]; }
    int n_instancias() const { return n_inst_; }
    const InstanciaMapa& instancia(int i) const { return inst_[i]; }
    int n_pois() const { return n_pois_; }
    const PoiMapa& poi(int i) const { return pois_[i]; }
    int n_spawns() const { return n_spawns_; }
    Vec3 spawn(int i) const { return Vec3(spawns_[i * 4], spawns_[i * 4 + 1], spawns_[i * 4 + 2]); }
    f32 raio_costa(f32 angulo) const;   // atan2(z, x), como coast_radius do Ventania
    int regiao(f32 x, f32 z) const;     // POI cujo contorno contém o ponto, ou −1
private:
    Relevo relevo_;
    f32 nivel_mar_ = 0, tam_setor_ = 30;
    int s_ = 0, n_mod_ = 0, n_inst_ = 0, n_pois_ = 0, n_spawns_ = 0, n_costa_ = 0;
    const u32* cores_ = nullptr;
    const f32* costa_ = nullptr;
    const ModeloMapa* mod_ = nullptr;
    const SetorMapa* setores_ = nullptr;
    const InstanciaMapa* inst_ = nullptr;
    const PoiMapa* pois_ = nullptr;
    const f32* pontos_ = nullptr;
    const f32* spawns_ = nullptr;
};
