#include "mapa.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {
struct CabecalhoMapa {
    char magia[4];
    u32 versao, n;
    f32 passo, origem, nivel_mar;
    u32 setores;
    f32 tam_setor;
    u32 n_modelos, n_inst, n_pois, n_spawns, n_pontos, n_costa;
    u32 off_relevo, off_cores, off_costa, off_modelos, off_setores, off_inst, off_pois, off_pontos, off_spawns;
    u32 n_loot, off_loot;  // v2
};
static_assert(sizeof(CabecalhoMapa) == 100, "CabecalhoMapa");
}  // namespace

bool MapaBrisa::carregar(const u8* d, u32 t, char* erro, int ne) {
    if (!d || t < 128 || memcmp(d, "BMAP", 4) != 0) { snprintf(erro, ne, "mapa sem assinatura BMAP"); return false; }
    if (reinterpret_cast<uintptr_t>(d) & 15) { snprintf(erro, ne, "mapa fora do alinhamento de 16"); return false; }
    const CabecalhoMapa* c = reinterpret_cast<const CabecalhoMapa*>(d);
    if (c->versao != VERSAO) { snprintf(erro, ne, "versão do mapa %u (o jogo lê a %u)", static_cast<unsigned>(c->versao), static_cast<unsigned>(VERSAO)); return false; }
    u32 nn = c->n * c->n;
    struct { u32 off, bytes; } blocos[] = {{c->off_relevo, nn * 2}, {c->off_cores, nn * 4}, {c->off_costa, c->n_costa * 4},
                                           {c->off_modelos, c->n_modelos * 48}, {c->off_setores, c->setores * c->setores * 16},
                                           {c->off_inst, c->n_inst * 32}, {c->off_pois, c->n_pois * 96},
                                           {c->off_pontos, c->n_pontos * 8}, {c->off_spawns, c->n_spawns * 16},
                                           {c->off_loot, c->n_loot * 16}};
    for (auto& b : blocos)
        if (b.off + b.bytes > t || (b.off & 3)) { snprintf(erro, ne, "mapa truncado"); return false; }
    relevo_.alturas_cm = reinterpret_cast<const i16*>(d + c->off_relevo);
    relevo_.n = static_cast<int>(c->n);
    relevo_.passo = c->passo;
    relevo_.origem = c->origem;
    nivel_mar_ = c->nivel_mar;
    s_ = static_cast<int>(c->setores);
    tam_setor_ = c->tam_setor;
    n_mod_ = static_cast<int>(c->n_modelos);
    n_inst_ = static_cast<int>(c->n_inst);
    n_pois_ = static_cast<int>(c->n_pois);
    n_spawns_ = static_cast<int>(c->n_spawns);
    n_costa_ = static_cast<int>(c->n_costa);
    cores_ = reinterpret_cast<const u32*>(d + c->off_cores);
    costa_ = reinterpret_cast<const f32*>(d + c->off_costa);
    mod_ = reinterpret_cast<const ModeloMapa*>(d + c->off_modelos);
    setores_ = reinterpret_cast<const SetorMapa*>(d + c->off_setores);
    inst_ = reinterpret_cast<const InstanciaMapa*>(d + c->off_inst);
    pois_ = reinterpret_cast<const PoiMapa*>(d + c->off_pois);
    pontos_ = reinterpret_cast<const f32*>(d + c->off_pontos);
    spawns_ = reinterpret_cast<const f32*>(d + c->off_spawns);
    loot_ = reinterpret_cast<const f32*>(d + c->off_loot);
    n_loot_ = static_cast<int>(c->n_loot);
    for (int i = 0; i < n_inst_; i++)
        if (inst_[i].modelo >= n_mod_) { snprintf(erro, ne, "instância %d com modelo inválido", i); return false; }
    for (int i = 0; i < n_mod_; i++)
        if (mod_[i].lod >= n_mod_) { snprintf(erro, ne, "modelo %d com LOD inválido", i); return false; }
    for (int i = 0; i < s_ * s_; i++)
        if (setores_[i].inicio + setores_[i].n > static_cast<u32>(n_inst_)) { snprintf(erro, ne, "setor %d inválido", i); return false; }
    for (int i = 0; i < n_pois_; i++)
        if (pois_[i].inicio + pois_[i].n_pontos > c->n_pontos) { snprintf(erro, ne, "POI %d inválido", i); return false; }
    return true;
}

u32 MapaBrisa::cor(int i, int j) const {
    int n = relevo_.n;
    i = i < 0 ? 0 : (i >= n ? n - 1 : i);
    j = j < 0 ? 0 : (j >= n ? n - 1 : j);
    return cores_[j * n + i];
}

f32 MapaBrisa::raio_costa(f32 a) const {
    if (n_costa_ == 0) return 0;
    f32 f = a / (2 * 3.14159265f) * n_costa_;
    f = fmodf(f, static_cast<f32>(n_costa_));
    if (f < 0) f += n_costa_;
    int k = static_cast<int>(f);
    f32 u = f - k;
    return costa_[k] + (costa_[(k + 1) % n_costa_] - costa_[k]) * u;
}

int MapaBrisa::regiao(f32 x, f32 z) const {
    for (int p = 0; p < n_pois_; p++) {
        const f32* q = pontos_ + pois_[p].inicio * 2;
        int n = static_cast<int>(pois_[p].n_pontos);
        bool dentro = false;
        for (int i = 0, j = n - 1; i < n; j = i++) {
            f32 xi = q[i * 2], zi = q[i * 2 + 1], xj = q[j * 2], zj = q[j * 2 + 1];
            if ((zi > z) != (zj > z) && x < (xj - xi) * (z - zi) / (zj - zi) + xi) dentro = !dentro;
        }
        if (dentro) return p;
    }
    return -1;
}
