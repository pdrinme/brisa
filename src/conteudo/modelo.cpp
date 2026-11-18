#include "modelo.h"

#include <stdio.h>
#include <string.h>

#include "nucleo/crc32.h"

namespace {
struct Cabecalho {
    char magia[4];
    u32 versao, n_vertices, n_partes, n_nos, n_marcadores, n_colisoes, flags;
    f32 centro[3], raio;
    f32 mn[3], escala;  // v3: escala das posições int16
    f32 mx[3], _b;
    u32 off_partes, off_nos, off_marcadores, off_colisoes, off_pos, off_normal, off_cor, _c;
};
static_assert(sizeof(Cabecalho) == 112, "Cabecalho BMOD");

struct ParteBMOD {
    i32 no;
    u32 inicio, n, _;
    f32 centro[3], raio;
};
}  // namespace

bool ModeloBrisa::carregar(const u8* d, u32 t, char* erro, int ne) {
    if (!d || t < 128 || memcmp(d, "BMOD", 4) != 0) { snprintf(erro, ne, "modelo sem assinatura BMOD"); return false; }
    if (reinterpret_cast<uintptr_t>(d) & 15) { snprintf(erro, ne, "modelo fora do alinhamento de 16"); return false; }
    const Cabecalho* c = reinterpret_cast<const Cabecalho*>(d);
    if (c->versao != VERSAO) { snprintf(erro, ne, "versão do modelo %u (o jogo lê a %u)", static_cast<unsigned>(c->versao), static_cast<unsigned>(VERSAO)); return false; }
    if (c->n_partes > static_cast<u32>(MAX_PARTES) || c->n_nos > static_cast<u32>(MAX_NOS)) {
        snprintf(erro, ne, "modelo com %u partes / %u nós (máximo %d / %d)", static_cast<unsigned>(c->n_partes),
                 static_cast<unsigned>(c->n_nos), MAX_PARTES, MAX_NOS);
        return false;
    }
    u32 nv_pad = (c->n_vertices + 4 + 3) / 4 * 4;
    if (c->off_cor + nv_pad * 4 > t || c->off_pos + nv_pad * 8 > t) { snprintf(erro, ne, "modelo truncado"); return false; }
    const ParteBMOD* partes = reinterpret_cast<const ParteBMOD*>(d + c->off_partes);
    const i16* pos = reinterpret_cast<const i16*>(d + c->off_pos);
    const u32* nrm = reinterpret_cast<const u32*>(d + c->off_normal);
    const u32* cor = reinterpret_cast<const u32*>(d + c->off_cor);
    n_partes_ = static_cast<int>(c->n_partes);
    for (int i = 0; i < n_partes_; i++) {
        const ParteBMOD& p = partes[i];
        if (p.inicio + p.n > c->n_vertices || p.n % 3 || (p.inicio & 3) || (p.no >= static_cast<i32>(c->n_nos))) {
            snprintf(erro, ne, "parte %d inválida", i);
            return false;
        }
        // O conversor alinha cada parte a 4 vértices (o VIF lê de 16 em 16 bytes).
        Malha& m = malhas_[i];
        m.n = p.n;
        m.pos = const_cast<i16*>(pos + p.inicio * 4);
        m.escala = c->escala;
        m.normal = const_cast<u32*>(nrm + p.inicio);
        m.cor = const_cast<u32*>(cor + p.inicio);
        m.centro = Vec3(p.centro[0], p.centro[1], p.centro[2]);
        m.raio = p.raio;
        no_parte_[i] = p.no;
    }
    nos_ = reinterpret_cast<const NoBMOD*>(d + c->off_nos);
    n_nos_ = static_cast<int>(c->n_nos);
    marc_ = c->n_marcadores ? reinterpret_cast<const MarcadorBMOD*>(d + c->off_marcadores) : nullptr;
    n_marc_ = static_cast<int>(c->n_marcadores);
    col_ = c->n_colisoes ? reinterpret_cast<const ColisaoBMOD*>(d + c->off_colisoes) : nullptr;
    n_col_ = static_cast<int>(c->n_colisoes);
    n_vertices_ = c->n_vertices;
    esqueletico_ = (c->flags & 1) != 0;
    mn_ = Vec3(c->mn[0], c->mn[1], c->mn[2]);
    mx_ = Vec3(c->mx[0], c->mx[1], c->mx[2]);
    // Assinatura do esqueleto: "índice:nome" dos ossos e nós animados, unidos por '|'
    // (igual a formatos.assinatura_esqueleto no conversor).
    u32 a = 0;
    bool primeiro = true;
    for (int i = 0; i < n_nos_; i++) {
        if (!(nos_[i].flags & 3)) continue;
        char buf[40];
        int n = snprintf(buf, sizeof buf, "%s%d:%.23s", primeiro ? "" : "|", i, nos_[i].nome);
        a = crc32(buf, static_cast<size_t>(n), a);
        primeiro = false;
    }
    assinatura_ = a;
    return true;
}

int ModeloBrisa::achar_no(const char* nome) const {
    for (int i = 0; i < n_nos_; i++)
        if (strncmp(nos_[i].nome, nome, sizeof nos_[i].nome) == 0) return i;
    return -1;
}

void pose_repouso(const ModeloBrisa& m, Pose& p) {
    p.n = m.n_nos();
    for (int i = 0; i < p.n; i++) {
        const NoBMOD& n = m.no(i);
        Mat4 local = Mat4::trs(Vec3(n.t[0], n.t[1], n.t[2]), n.r, Vec3(n.s[0], n.s[1], n.s[2]));
        p.mundo[i] = n.pai < 0 ? local : p.mundo[n.pai] * local;
    }
}

Vec3 posicao_marcador(const ModeloBrisa& m, const Pose& p, int i) {
    const MarcadorBMOD& k = m.marcador(i);
    Vec3 l(k.pos[0], k.pos[1], k.pos[2]);
    if (k.no < 0) return l;
    Vec4 r = transformar(p.mundo[k.no], l);
    return Vec3(r.x, r.y, r.z);
}
