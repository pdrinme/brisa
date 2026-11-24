#include "navegacao.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {
struct CabecalhoNav {
    char magia[4];
    u32 versao, n;
    f32 t, limite;
    u32 n_andares, n_lig, n_pontos, n_comodos;
    u32 off_alt, off_bloq, off_arestas, off_portas, off_andares, off_lig, off_pontos, off_comodos;
};
static_assert(sizeof(CabecalhoNav) == 68, "CabecalhoNav");
enum { L_N, L_L, L_S, L_O };

// Estado do A* (um por vez: os bots planejam em sequência). Carimbo por busca,
// para não limpar 22 mil entradas a cada caminho.
const int MAX_NOS = Navegacao::N * Navegacao::N + 1024;
f32 g_custo[MAX_NOS];
i32 g_veio[MAX_NOS];
i16 g_via[MAX_NOS];      // ligação usada para chegar no nó (−1 = nenhuma)
u16 g_marca[MAX_NOS];
u16 g_busca = 0;
struct Aberto { f32 f; i32 no; };
const int MAX_ABERTOS = 16384;
Aberto g_heap[MAX_ABERTOS];
int g_n_heap = 0;

void empurrar(f32 f, int no) {
    if (g_n_heap >= MAX_ABERTOS) return;
    int i = g_n_heap++;
    g_heap[i] = {f, no};
    while (i > 0) {
        int p = (i - 1) / 2;
        if (g_heap[p].f <= g_heap[i].f) break;
        Aberto t = g_heap[p]; g_heap[p] = g_heap[i]; g_heap[i] = t;
        i = p;
    }
}

Aberto tirar() {
    Aberto topo = g_heap[0];
    g_heap[0] = g_heap[--g_n_heap];
    int i = 0;
    for (;;) {
        int e = 2 * i + 1, d = e + 1, m = i;
        if (e < g_n_heap && g_heap[e].f < g_heap[m].f) m = e;
        if (d < g_n_heap && g_heap[d].f < g_heap[m].f) m = d;
        if (m == i) break;
        Aberto t = g_heap[m]; g_heap[m] = g_heap[i]; g_heap[i] = t;
        i = m;
    }
    return topo;
}
}  // namespace

bool Navegacao::carregar(const u8* d, u32 t, char* erro, int ne) {
    if (!d || t < 96 || memcmp(d, "BNAV", 4) != 0) { snprintf(erro, ne, "navegação sem assinatura BNAV"); return false; }
    const CabecalhoNav* c = reinterpret_cast<const CabecalhoNav*>(d);
    if (c->versao != VERSAO || c->n != static_cast<u32>(N)) { snprintf(erro, ne, "navegação: versão %u ou grade %u inesperadas", static_cast<unsigned>(c->versao), static_cast<unsigned>(c->n)); return false; }
    u32 nn = N * N;
    struct { u32 off, bytes; } b[] = {{c->off_alt, nn * 2}, {c->off_bloq, nn}, {c->off_arestas, nn}, {c->off_portas, nn},
                                      {c->off_andares, c->n_andares * 36}, {c->off_lig, c->n_lig * 16},
                                      {c->off_pontos, c->n_pontos * 12}, {c->off_comodos, c->n_comodos * 16}};
    for (auto& k : b)
        if (k.off + k.bytes > t || (k.off & 3)) { snprintf(erro, ne, "navegação truncada"); return false; }
    if (N * N + c->n_andares > static_cast<u32>(MAX_NOS)) { snprintf(erro, ne, "navegação com andares demais"); return false; }
    t_ = c->t;
    limite_ = c->limite;
    alt_ = reinterpret_cast<const i16*>(d + c->off_alt);
    bloq_ = d + c->off_bloq;
    arestas_ = d + c->off_arestas;
    portas_ = d + c->off_portas;
    andares_ = reinterpret_cast<const Andar*>(d + c->off_andares);
    lig_ = reinterpret_cast<const Ligacao*>(d + c->off_lig);
    pontos_ = reinterpret_cast<const f32*>(d + c->off_pontos);
    comodos_ = reinterpret_cast<const f32*>(d + c->off_comodos);
    n_andares_ = static_cast<int>(c->n_andares);
    n_lig_ = static_cast<int>(c->n_lig);
    n_pontos_ = static_cast<int>(c->n_pontos);
    n_comodos_ = static_cast<int>(c->n_comodos);
    for (int i = 0; i < n_lig_; i++)
        if (lig_[i].ponto + lig_[i].n_pontos > static_cast<u32>(n_pontos_) || lig_[i].a >= n_nos() || lig_[i].b >= n_nos()) {
            snprintf(erro, ne, "navegação: ligação %d inválida", i);
            return false;
        }
    return true;
}

Vec3 Navegacao::posicao(int no) const {
    if (no < N * N) {
        int i = no % N, j = no / N;
        return Vec3(-limite_ + (i + 0.5f) * t_, alt_[no] * 0.01f, -limite_ + (j + 0.5f) * t_);
    }
    const Andar& a = andares_[no - N * N];
    return Vec3(a.x, a.y, a.z);
}

bool Navegacao::livre(int ci, int cj, int pi, int pj) const {
    if (pi < 0 || pj < 0 || pi >= N || pj >= N || bloq_[pj * N + pi]) return false;
    int dx = pi - ci, dz = pj - cj, k = cj * N + ci;
    if (dx == 0 || dz == 0) {
        int lado = dx == 1 ? L_L : (dx == -1 ? L_O : (dz == 1 ? L_S : L_N));
        return (arestas_[k] & (1 << lado)) == 0;
    }
    // diagonal: as duas ortogonais livres e sem porta (não corta o batente)
    if (portas_[k] || portas_[pj * N + pi]) return false;
    return livre(ci, cj, ci + dx, cj) && livre(ci + dx, cj, pi, pj) && livre(ci, cj, ci, cj + dz) && livre(ci, cj + dz, pi, pj);
}

bool Navegacao::porta_entre(int a, int b) const {
    int ai = a % N, aj = a / N, bi = b % N, bj = b / N, dx = bi - ai, dz = bj - aj;
    if ((dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz) != 1) return false;
    int lado = dx == 1 ? L_L : (dx == -1 ? L_O : (dz == 1 ? L_S : L_N));
    return (portas_[a] & (1 << lado)) != 0;
}

int Navegacao::ligacao(int a, int b) const {
    for (int i = 0; i < n_lig_; i++)
        if (lig_[i].a == a && lig_[i].b == b) return i;
    return -1;
}

int Navegacao::livre_perto(int ci, int cj) const {
    for (int r = 1; r < 6; r++)
        for (int dj = -r; dj <= r; dj++)
            for (int di = -r; di <= r; di++) {
                int i = ci + di, j = cj + dj;
                if (i >= 0 && j >= 0 && i < N && j < N && !bloq_[j * N + i]) return j * N + i;
            }
    return cj * N + ci;
}

int Navegacao::no_de(Vec3 p) const {
    int ci = static_cast<int>(floorf((p.x + limite_) / t_)), cj = static_cast<int>(floorf((p.z + limite_) / t_));
    ci = ci < 0 ? 0 : (ci >= N ? N - 1 : ci);
    cj = cj < 0 ? 0 : (cj >= N ? N - 1 : cj);
    int k = cj * N + ci;
    if (p.y - alt_[k] * 0.01f <= 2.5f - 0.3f || n_andares_ == 0) return bloq_[k] ? livre_perto(ci, cj) : k;
    // andar: o piso mais perto abaixo do ponto, nesta célula ou numa vizinha
    int melhor = -1;
    f32 dist = 1e9f;
    int nivel = static_cast<int>(floorf((p.y + 0.5f) / t_));
    for (int a = 0; a < n_andares_; a++) {
        const Andar& f = andares_[a];
        int di = f.i - ci, dj = f.j - cj;
        if (di < -1 || di > 1 || dj < -1 || dj > 1 || (f.nivel != nivel && f.nivel != nivel - 1)) continue;
        Vec3 d = Vec3(f.x, f.y, f.z) - p;
        f32 k2 = sqrtf(ponto(d, d)) + (di == 0 && dj == 0 ? 0 : 1.5f);
        if (k2 < dist) { dist = k2; melhor = N * N + a; }
    }
    if (melhor >= 0) return melhor;
    return bloq_[k] ? livre_perto(ci, cj) : k;
}

bool Navegacao::caminho(Vec3 de, Vec3 para, const u32* evitar, int n_evitar, Caminho& s) {
    caminhos++;
    s.n = s.n_nos = 0;
    int a = no_de(de), b = no_de(para);
    // No meio de uma escada (patamar entre o chão e o andar): no_de cai no nó de baixo,
    // 2 m abaixo, e a rota mandaria pular do patamar. Segue o resto da escada até a
    // ponta mais promissora e começa o A* dali.
    int pre_lig = -1, pre_q = 0;
    if (fabsf(de.y - posicao(a).y) > 1.0f) {
        f32 melhor = 1e9f;
        for (int l = 0; l < n_lig_; l++)
            for (int q = 0; q < lig_[l].n_pontos; q++) {
                const f32* pp = pontos_ + (lig_[l].ponto + q) * 3;
                f32 dx = pp[0] - de.x, dz = pp[2] - de.z;
                if (fabsf(pp[1] - de.y) > 0.8f || dx * dx + dz * dz > 1.6f * 1.6f) continue;
                Vec3 fim = posicao(lig_[l].b);
                f32 resto = sqrtf(dx * dx + dz * dz);
                for (int r = q + 1; r < lig_[l].n_pontos; r++) {
                    const f32* p0 = pontos_ + (lig_[l].ponto + r - 1) * 3;
                    const f32* p1 = pontos_ + (lig_[l].ponto + r) * 3;
                    Vec3 d(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]);
                    resto += sqrtf(ponto(d, d));
                }
                Vec3 dd = para - fim;
                f32 custo = resto + sqrtf(ponto(dd, dd));
                if (custo < melhor) { melhor = custo; pre_lig = l; pre_q = q; }
            }
        if (pre_lig >= 0) a = lig_[pre_lig].b;
    }
    if (++g_busca == 0) { memset(g_marca, 0, sizeof g_marca); g_busca = 1; }
    g_n_heap = 0;
    g_marca[a] = g_busca;
    g_custo[a] = 0;
    g_veio[a] = a;
    g_via[a] = -1;
    empurrar(0, a);
    Vec3 pb = posicao(b);
    int n = 0;
    bool achou = false;
    auto relaxar = [&](int atual, int id, f32 custo, int via) {
        if (n_evitar) {
            u32 k = chave_aresta(atual, id);
            for (int e = 0; e < n_evitar; e++)
                if (evitar[e] == k) return;
        }
        f32 c = g_custo[atual] + custo;
        if (g_marca[id] == g_busca && c >= g_custo[id]) return;
        g_marca[id] = g_busca;
        g_custo[id] = c;
        g_veio[id] = atual;
        g_via[id] = static_cast<i16>(via);
        Vec3 pi = posicao(id);
        f32 dx = fabsf(pi.x - pb.x) / t_, dz = fabsf(pi.z - pb.z) / t_;
        f32 h = ((dx > dz ? dx : dz) + 0.414f * (dx < dz ? dx : dz) + fabsf(pi.y - pb.y) / t_) * 1.3f;
        empurrar(c + h, id);
    };
    while (g_n_heap > 0 && n < MAX_EXPANSOES) {
        Aberto t = tirar();
        int atual = t.no;
        n++;
        if (atual == b) { achou = true; break; }
        if (atual < N * N) {
            int ci = atual % N, cj = atual / N;
            static const int D[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
            for (auto& d : D)
                if (livre(ci, cj, ci + d[0], cj + d[1])) relaxar(atual, (cj + d[1]) * N + ci + d[0], d[0] && d[1] ? 1.414f : 1.0f, -1);
        } else {
            const Andar& f = andares_[atual - N * N];
            for (int v : f.viz)
                if (v >= 0) relaxar(atual, v, 1.0f, -1);
        }
        for (int l = 0; l < n_lig_; l++)
            if (lig_[l].a == atual) relaxar(atual, lig_[l].b, lig_[l].custo, l);
    }
    expansoes += static_cast<u32>(n);
    if (!achou && !(g_marca[b] == g_busca && a == b)) {
        if (a != b) return false;
    }
    // Reconstrói a rota (nós de b até a).
    static int nos[4096];
    int nn = 0;
    for (int c = b; c != a && nn < 4095; c = g_veio[c]) nos[nn++] = c;
    nos[nn++] = a;
    for (int i = 0; i < nn / 2; i++) { int x = nos[i]; nos[i] = nos[nn - 1 - i]; nos[nn - 1 - i] = x; }
    for (int i = 0; i < nn && s.n_nos < MAX_PONTOS; i++) s.nos[s.n_nos++] = nos[i];
    auto por = [&](Vec3 p, u8 preciso) {
        if (s.n < MAX_PONTOS) { s.pontos[s.n] = p; s.precisos[s.n] = preciso; s.n++; }
    };
    if (pre_lig >= 0)
        for (int q = pre_q; q < lig_[pre_lig].n_pontos; q++) {
            const f32* pp = pontos_ + (lig_[pre_lig].ponto + q) * 3;
            por(Vec3(pp[0], pp[1], pp[2]), 1);
        }
    for (int k = 1; k < nn; k++) {
        int ant = nos[k - 1], id = nos[k];
        int via = g_via[id];
        if (via >= 0 && lig_[via].a == ant && lig_[via].b == id) {
            for (int q = 0; q < lig_[via].n_pontos; q++) {
                const f32* pp = pontos_ + (lig_[via].ponto + q) * 3;
                por(Vec3(pp[0], pp[1], pp[2]), 1);
            }
        } else if (ant < N * N && id < N * N && porta_entre(ant, id)) {
            por((posicao(ant) + posicao(id)) * 0.5f, 1);
        }
        // corta pontos colineares do chão (caminho mais limpo)
        if (k < nn - 1 && id < N * N && nos[k + 1] < N * N && ant < N * N && g_via[nos[k + 1]] < 0 && id - ant == nos[k + 1] - id)
            continue;
        por(posicao(id), 0);
    }
    por(para, 0);
    return true;
}
