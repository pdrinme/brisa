#include "terreno.h"

#include <math.h>
#include <string.h>

namespace {
// Escala das posições: 1/512 m (2 mm). 30 m = 15360 unidades exatas, então
// blocos vizinhos quantizam a borda comum igual (sem frestas de arredondamento).
const f32 EXTENSAO = 32767.0f / 512.0f;
const f32 MAR_TAM = 60.0f;  // ladrilho do mar
}  // namespace

void Terreno::vertice(int i, int j, int passo, f32 ox, f32 oz, i16* pos, u32* nrm, u32* cor) const {
    const Relevo& r = mapa_->relevo();
    f32 h = r.amostra(i, j);
    // Costura: na borda do bloco, as amostras ímpares seguem a reta entre as pares
    // vizinhas (a borda do LOD1).
    if (passo == 1) {
        if (i % BLOCO == 0 && (j & 1)) h = 0.5f * (r.amostra(i, j - 1) + r.amostra(i, j + 1));
        else if (j % BLOCO == 0 && (i & 1)) h = 0.5f * (r.amostra(i - 1, j) + r.amostra(i + 1, j));
    }
    f32 x = r.origem + i * r.passo - ox, z = r.origem + j * r.passo - oz;
    quantizar_posicao(Vec3(x, h, z), EXTENSAO / 32767.0f, pos);
    f32 gx = (r.amostra(i + 1, j) - r.amostra(i - 1, j)) / (2 * r.passo);
    f32 gz = (r.amostra(i, j + 1) - r.amostra(i, j - 1)) / (2 * r.passo);
    *nrm = empacotar_normal(normalizar(Vec3(-gx, 1.0f, -gz)));
    *cor = mapa_->cor(i, j);
}

// Gera os quadrados [i0, i0 + n) × [j0, j0 + n) (em amostras, de `passo` em `passo`).
u32 Terreno::gerar(int bi, int bj, int passo, int i0, int j0, int n, i16* pos, u32* nrm, u32* cor) const {
    const Relevo& r = mapa_->relevo();
    f32 ox = r.origem + bi * BLOCO * r.passo, oz = r.origem + bj * BLOCO * r.passo;
    const f32 fundo = mapa_->nivel_mar() - 2.2f;
    u32 k = 0;
    for (int j = j0; j < j0 + n; j += passo)
        for (int i = i0; i < i0 + n; i += passo) {
            f32 maior = fmaxf(fmaxf(r.amostra(i, j), r.amostra(i + passo, j)), fmaxf(r.amostra(i, j + passo), r.amostra(i + passo, j + passo)));
            if (maior < fundo) continue;  // quadrado todo no fundo do mar
            // Sentido anti-horário visto de cima: (i,j) (i+1,j+1) (i+1,j) e (i,j) (i,j+1) (i+1,j+1).
            const int q[6][2] = {{i, j}, {i + passo, j + passo}, {i + passo, j}, {i, j}, {i, j + passo}, {i + passo, j + passo}};
            for (int v = 0; v < 6; v++, k++) vertice(q[v][0], q[v][1], passo, ox, oz, pos + k * 4, nrm + k, cor + k);
        }
    return k;
}

namespace {
void esfera(Malha& m) {
    if (m.n == 0) return;
    Vec3 mn(1e30f, 1e30f, 1e30f), mx(-1e30f, -1e30f, -1e30f);
    for (u32 i = 0; i < m.n; i++) {
        Vec3 p = m.posicao(i);
        mn = Vec3(fminf(mn.x, p.x), fminf(mn.y, p.y), fminf(mn.z, p.z));
        mx = Vec3(fmaxf(mx.x, p.x), fmaxf(mx.y, p.y), fmaxf(mx.z, p.z));
    }
    m.centro = (mn + mx) * 0.5f;
    Vec3 d = mx - m.centro;
    m.raio = sqrtf(ponto(d, d));
}
}  // namespace

bool Terreno::iniciar(const MapaBrisa& mapa, Arena& arena) {
    mapa_ = &mapa;
    lado_ = (mapa.relevo().n - 1) / BLOCO;
    if (lado_ * lado_ > MAX_BLOCOS) return false;
    n_blocos_ = 0;
    bytes_ = 0;
    const u32 MAX_V1 = (BLOCO / 2) * (BLOCO / 2) * 6;
    // Temporários para medir cada bloco antes de alocar o tamanho exato.
    static i16 tpos[MAX_V1 * 4 + 16];
    static u32 tnrm[MAX_V1 + 4], tcor[MAX_V1 + 4];
    for (int bj = 0; bj < lado_; bj++)
        for (int bi = 0; bi < lado_; bi++) {
            int b = bj * lado_ + bi;
            u32 n = gerar(bi, bj, 2, bi * BLOCO, bj * BLOCO, BLOCO, tpos, tnrm, tcor);
            tem_[b] = n > 0;
            lod1_[b] = Malha();
            if (!n) continue;
            u32 nf = (n + 4 + 3) / 4 * 4;  // folga zerada (o VIF lê de 16 em 16 bytes)
            Malha& m = lod1_[b];
            m.pos = arena.alocar_n<i16>(nf * 4);
            m.normal = arena.alocar_n<u32>(nf);
            m.cor = arena.alocar_n<u32>(nf);
            if (!m.pos || !m.normal || !m.cor) return false;
            memset(m.pos, 0, nf * 8);
            memset(m.normal, 0, nf * 4);
            memset(m.cor, 0, nf * 4);
            memcpy(m.pos, tpos, n * 8);
            memcpy(m.normal, tnrm, n * 4);
            memcpy(m.cor, tcor, n * 4);
            m.n = n;
            m.escala = EXTENSAO / 32767.0f;
            esfera(m);
            centro_[b] = m.centro + Vec3(mapa.origem() + bi * BLOCO * mapa.relevo().passo, 0, mapa.origem() + bj * BLOCO * mapa.relevo().passo);
            raio_[b] = m.raio;
            bytes_ += nf * 16;
            n_blocos_++;
        }
    // Cache do LOD0: 4 partes de 5 × 5 quadrados (150 vértices) por bloco.
    const u32 V0 = 150, VF = (V0 + 4 + 3) / 4 * 4;
    for (Slot& s : cache_) {
        s.pos = arena.alocar_n<i16>(VF * 4 * 4);
        s.nrm = arena.alocar_n<u32>(VF * 4);
        s.cor = arena.alocar_n<u32>(VF * 4);
        if (!s.pos || !s.nrm || !s.cor) return false;
        memset(s.pos, 0, VF * 4 * 8);
        memset(s.nrm, 0, VF * 4 * 4);
        memset(s.cor, 0, VF * 4 * 4);
        s.bloco = -1;
        bytes_ += VF * 4 * 16;
    }
    // Mar: um ladrilho de 60 m (2 × 2 quadrados) desenhado onde há mar, até 360 m do centro.
    ConstrutorMalha mar(arena, 8, 40.0f);
    const Cor AZUL = rgb(0x3fb6d9);
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++) {
            f32 x0 = -30 + i * 30.0f, z0 = -30 + j * 30.0f;
            mar.quad(Vec3(x0, 0, z0 + 30), Vec3(x0 + 30, 0, z0 + 30), Vec3(x0 + 30, 0, z0), Vec3(x0, 0, z0), AZUL);
        }
    mar_ = mar.terminar();
    for (int j = 0; j < 12; j++)
        for (int i = 0; i < 12; i++) {
            f32 cx = -360 + (i + 0.5f) * MAR_TAM, cz = -360 + (j + 0.5f) * MAR_TAM;
            bool mar_aqui = false;
            for (int a = 0; a < 7 && !mar_aqui; a++)
                for (int b = 0; b < 7 && !mar_aqui; b++) {
                    f32 x = cx - 30 + a * 10, z = cz - 30 + b * 10;
                    if (fabsf(x) > 299 || fabsf(z) > 299 || mapa.relevo().altura(x, z) < mapa.nivel_mar() + 0.5f) mar_aqui = true;
                }
            tem_mar_[j * 12 + i] = mar_aqui;
        }
    return true;
}

const Malha* Terreno::lod0(int bi, int bj, u32 quadro) {
    int b = bj * lado_ + bi;
    Slot* livre = nullptr;
    for (Slot& s : cache_) {
        if (s.bloco == b) { s.usado = quadro; return s.partes; }
        if (!livre || s.usado < livre->usado) livre = &s;
    }
    if (!livre || (livre->bloco >= 0 && livre->usado == quadro) || montados_quadro_ >= MONTAR_POR_QUADRO) return nullptr;
    const u32 VF = (150 + 4 + 3) / 4 * 4;
    for (int p = 0; p < 4; p++) {
        i16* pos = livre->pos + p * VF * 4;
        u32* nrm = livre->nrm + p * VF;
        u32* cor = livre->cor + p * VF;
        memset(pos, 0, VF * 8);
        memset(nrm, 0, VF * 4);
        memset(cor, 0, VF * 4);
        u32 n = gerar(bi, bj, 1, bi * BLOCO + (p & 1) * 5, bj * BLOCO + (p >> 1) * 5, 5, pos, nrm, cor);
        Malha& m = livre->partes[p];
        m = Malha();
        m.pos = pos;
        m.normal = nrm;
        m.cor = cor;
        m.n = n;
        m.escala = EXTENSAO / 32767.0f;
        esfera(m);
    }
    livre->bloco = b;
    livre->usado = quadro;
    montados_quadro_++;
    return livre->partes;
}

void Terreno::montar(Cena3D& cena, const Frustum& fr, Vec3 cam, EstatisticasTerreno& est) {
    quadro_++;
    montados_quadro_ = 0;
    const Relevo& r = mapa_->relevo();
    const f32 tam = BLOCO * r.passo;
    // Primeiro os mais perto (o cache do LOD0 vai para eles).
    int ci = static_cast<int>((cam.x - r.origem) / tam), cj = static_cast<int>((cam.z - r.origem) / tam);
    int alcance = static_cast<int>(dist_max / tam) + 1;
    for (int anel = 0; anel <= alcance; anel++)
        for (int bj = cj - anel; bj <= cj + anel; bj++)
            for (int bi = ci - anel; bi <= ci + anel; bi++) {
                if (bi < 0 || bj < 0 || bi >= lado_ || bj >= lado_) continue;
                if (bi != ci - anel && bi != ci + anel && bj != cj - anel && bj != cj + anel) continue;  // só a borda do anel
                int b = bj * lado_ + bi;
                if (!tem_[b]) continue;
                Vec3 d = centro_[b] - cam;
                f32 dist = sqrtf(d.x * d.x + d.z * d.z);
                if (dist - raio_[b] > dist_max) continue;
                if (fr.esfera(centro_[b], raio_[b]) == 0) continue;
                Mat4 t = Mat4::translacao(Vec3(r.origem + bi * tam, 0, r.origem + bj * tam));
                const Malha* p = dist < dist_lod0 ? lod0(bi, bj, quadro_) : nullptr;
                if (p) {
                    for (int k = 0; k < 4; k++)
                        if (p[k].n && cena.adicionar(&p[k], t)) est.triangulos += p[k].triangulos();
                    est.blocos_lod0++;
                } else {
                    if (cena.adicionar(&lod1_[b], t)) est.triangulos += lod1_[b].triangulos();
                    est.blocos_lod1++;
                }
            }
    est.montados += montados_quadro_;
    for (int j = 0; j < 12; j++)
        for (int i = 0; i < 12; i++) {
            if (!tem_mar_[j * 12 + i]) continue;
            Vec3 c(-360 + (i + 0.5f) * MAR_TAM, mapa_->nivel_mar(), -360 + (j + 0.5f) * MAR_TAM);
            Vec3 d = c - cam;
            if (sqrtf(d.x * d.x + d.z * d.z) - 43 > dist_max) continue;
            if (fr.esfera(c, 43.0f) == 0) continue;
            if (cena.adicionar(&mar_, Mat4::translacao(c))) est.triangulos += mar_.triangulos();
        }
}
