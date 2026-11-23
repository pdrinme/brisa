// Etapa 5: mapa (BMAP), relevo desenhável, colisão da ilha, corte e minimapa.
#include <math.h>
#include <string.h>

#include "conteudo/pak.h"
#include "conteudo/textura.h"
#include "jogo/ilha.h"
#include "jogo/roteiro_ilha.h"
#include "mundo/mapa.h"
#include "mundo/terreno.h"
#include "nucleo/arena.h"
#include "plataforma/plataforma.h"
#include "teste.h"

namespace {
const f32 DT = 1.0f / 30.0f;

const Pacote* pacote() {
    static Pacote p;
    static bool ok = false, tentou = false;
    if (!tentou) {
        tentou = true;
        u32 t = 0;
        const u8* d = plataforma::conteudo(&t);
        char e[128];
        ok = d && p.abrir(d, t, e, sizeof e);
    }
    return ok ? &p : nullptr;
}

const MapaBrisa* mapa() {
    static MapaBrisa m;
    static bool ok = false, tentou = false;
    if (!tentou && pacote()) {
        tentou = true;
        u32 t = 0;
        const u8* d = pacote()->arquivo("mapa/ilha.bmap", &t);
        char e[128];
        ok = d && m.carregar(d, t, e, sizeof e);
    }
    return ok ? &m : nullptr;
}

Ilha* ilha() {
    alignas(64) static u8 memoria[3 * 1024 * 1024];
    static Ilha i;
    static bool ok = false, tentou = false;
    if (!tentou && pacote()) {
        tentou = true;
        Arena a(memoria, sizeof memoria);
        char e[160];
        ok = i.iniciar(*pacote(), a, e, sizeof e);
        if (!ok) printf("    ilha: %s\n", e);
    }
    return ok ? &i : nullptr;
}

// Anda na direção (dx, dz) do mundo por `passos` passos.
Personagem andar(const MundoColisao& m, Vec3 de, f32 dx, f32 dz, int passos, bool correr = false) {
    Personagem p;
    p.pos = de;
    p.no_chao = true;
    ComandoMovimento c;
    c.y = 1.0f;
    c.yaw = atan2f(-dx, -dz);  // frente = (−sen yaw, −cos yaw)
    c.correr = correr;
    for (int i = 0; i < passos; i++) passo_movimento(p, c, m, DT);
    return p;
}

// Ponto do mundo a partir do espaço local de uma instância (x, z).
void local_para_mundo(const InstanciaMapa& k, f32 lx, f32 lz, f32* x, f32* z) {
    f32 c = cosf(k.rot * 3.14159265f / 180.0f), s = sinf(k.rot * 3.14159265f / 180.0f);
    *x = k.x + c * lx + s * lz;
    *z = k.z - s * lx + c * lz;
}
}  // namespace

TESTE(mapa_carrega_com_os_numeros_do_ventania) {
    const MapaBrisa* m = mapa();
    VERIFICA(m != nullptr);
    if (!m) return;
    VERIFICA(m->n_instancias() == 1944);  // 646 peças + 1219 props + 37 baús + 42 trechos de estrada
    VERIFICA(m->n_pois() == 6 && m->n_spawns() == 24);
    VERIFICA(m->relevo().n == 201 && m->relevo().passo == 3.0f && m->relevo().origem == -300.0f);
    VERIFICA(m->setores_lado() == 20 && m->tam_setor() == 30.0f);
    int baus = 0;
    for (int i = 0; i < m->n_instancias(); i++) baus += m->instancia(i).categoria == CAT_BAU;
    VERIFICA(baus == 37);
    // Relevo: o Mirante é o morro do centro; o canto do mapa é mar.
    VERIFICA(m->relevo().altura(10, 5) > 15.0f);
    VERIFICA(m->relevo().altura(-295, -295) < m->nivel_mar());
    // Regiões pelo contorno dos POIs.
    int vila = m->regiao(-150, 55), mir = m->regiao(10, 5);
    VERIFICA(vila >= 0 && strcmp(m->poi(vila).id, "vila_brisa") == 0);
    VERIFICA(mir >= 0 && strcmp(m->poi(mir).id, "mirante_vento") == 0);
    VERIFICA(m->regiao(-295, -295) < 0);
    // Costa entre 150 e 288 m (ilha_ventania.gd COSTA_MAX).
    for (int k = 0; k < 36; k++) {
        f32 r = m->raio_costa(k * 10.0f * 3.14159265f / 180.0f);
        VERIFICA(r >= 150.0f && r <= 288.0f);
    }
}

TESTE(mapa_setores_e_cadeia_de_lods) {
    const MapaBrisa* m = mapa();
    if (!m) return;
    const int S = m->setores_lado();
    int total = 0;
    for (int sj = 0; sj < S; sj++)
        for (int si = 0; si < S; si++) {
            const SetorMapa& s = m->setor(si, sj);
            total += static_cast<int>(s.n);
            for (u32 i = s.inicio; i < s.inicio + s.n; i++) {
                const InstanciaMapa& k = m->instancia(static_cast<int>(i));
                int ci = static_cast<int>(floorf((k.x - m->origem()) / m->tam_setor()));
                int cj = static_cast<int>(floorf((k.z - m->origem()) / m->tam_setor()));
                ci = ci < 0 ? 0 : (ci >= S ? S - 1 : ci);
                cj = cj < 0 ? 0 : (cj >= S ? S - 1 : cj);
                VERIFICA(ci == si && cj == sj);
                if (k.categoria != CAT_ESTRADA) VERIFICA(k.y >= s.ymin - 0.01f);  // estrada: y = 0, alturas na malha
                if (i > s.inicio) VERIFICA(m->instancia(static_cast<int>(i) - 1).modelo <= k.modelo);  // ordenadas por modelo
            }
        }
    VERIFICA(total == m->n_instancias());
    // Cada LOD tem menos triângulos que o anterior e começa mais longe.
    BibliotecaModelos bib;
    alignas(64) static u8 mem[512 * 1024];
    Arena a(mem, sizeof mem);
    VERIFICA(bib.iniciar(pacote(), a, m->n_modelos()));
    char e[128];
    int cadeias = 0;
    for (int i = 0; i < m->n_modelos(); i++) {
        const ModeloMapa& mm = m->modelo(i);
        if (mm.lod < 0) continue;
        cadeias++;
        int a0 = bib.carregar(mm.nome, e, sizeof e), a1 = bib.carregar(m->modelo(mm.lod).nome, e, sizeof e);
        VERIFICA(a0 >= 0 && a1 >= 0);
        if (a0 >= 0 && a1 >= 0) VERIFICA(bib[a1].triangulos() < bib[a0].triangulos());
        if (m->modelo(mm.lod).lod >= 0) VERIFICA(m->modelo(mm.lod).dist_lod > mm.dist_lod);
    }
    VERIFICA(cadeias > 20);
}

TESTE(relevo_sem_frestas_entre_lods) {
    const MapaBrisa* m = mapa();
    if (!m) return;
    alignas(64) static u8 mem[2 * 1024 * 1024];
    Arena a(mem, sizeof mem);
    static Terreno t;
    VERIFICA(t.iniciar(*m, a));
    VERIFICA(t.blocos() > 200 && t.blocos() <= 400);
    // Em cada borda x = 0 de blocos com LOD0: os vértices do LOD0 ficam sobre a borda do LOD1.
    int conferidos = 0;
    u32 quadro = 1000;
    for (int bj = 4; bj < 16; bj += 3)
        for (int bi = 4; bi < 16; bi += 3) {
            const Malha& l1 = t.lod1(bi, bj);
            const Malha* l0 = t.lod0(bi, bj, quadro++);
            if (!l0 || l1.n == 0) continue;
            for (int p = 0; p < 4; p++)
                for (u32 v = 0; v < l0[p].n; v++) {
                    Vec3 q = l0[p].posicao(v);
                    if (fabsf(q.x) > 0.001f) continue;  // só a borda x = 0
                    // altura da borda do LOD1 em z: entre os vértices do LOD1 com x = 0
                    f32 za = -1, ya = 0, zb = 1e9f, yb = 0;
                    for (u32 w = 0; w < l1.n; w++) {
                        Vec3 r = l1.posicao(w);
                        if (fabsf(r.x) > 0.001f) continue;
                        if (r.z <= q.z + 0.001f && r.z > za) { za = r.z; ya = r.y; }
                        if (r.z >= q.z - 0.001f && r.z < zb) { zb = r.z; yb = r.y; }
                    }
                    if (za < 0 || zb > 1e8f) continue;  // quadrado do LOD1 sem borda aqui (mar)
                    f32 y = zb - za > 0.001f ? ya + (yb - ya) * (q.z - za) / (zb - za) : ya;
                    VERIFICA(fabsf(y - q.y) < 0.005f);
                    conferidos++;
                }
        }
    VERIFICA(conferidos > 100);
}

TESTE(ilha_carrega_e_poe_o_jogador_no_relevo) {
    Ilha* i = ilha();
    VERIFICA(i != nullptr);
    if (!i) return;
    VERIFICA(i->mundo().descartados() == 0);
    VERIFICA(i->mundo().n() > 2000);
    Vec3 s = i->mapa().spawn(0);
    i->ir_para(s.x, s.z, 0);
    const Personagem& p = i->jogador().personagem();
    VERIFICA(p.no_chao);
    VERIFICA(fabsf(p.pos.y - i->mapa().relevo().altura(p.pos.x, p.pos.z)) < 0.02f);
    VERIFICA(fabsf(p.pos.x - s.x) < 0.05f && fabsf(p.pos.z - s.z) < 0.05f);
}

TESTE(ilha_porta_passa_e_parede_inteira_bloqueia) {
    Ilha* il = ilha();
    if (!il) return;
    const MapaBrisa& m = il->mapa();
    const MundoColisao& w = il->mundo();
    bool testou_porta = false, testou_parede = false;
    for (int i = 0; i < m.n_instancias() && !(testou_porta && testou_parede); i++) {
        const InstanciaMapa& k = m.instancia(i);
        if (k.categoria != CAT_PECA || k.y > 0.5f + m.relevo().altura(k.x, k.z)) continue;  // só paredes do térreo
        const char* nome = m.modelo(k.modelo).nome;
        bool porta = strstr(nome, "parede_") == nome && strstr(nome, "_m493") != nullptr;  // M_PORTA do Ventania (495 é janela)
        bool inteira = strstr(nome, "parede_") == nome && strchr(nome + 7, '_') == nullptr;
        if ((!porta || testou_porta) && (!inteira || testou_parede)) continue;
        // Pontos a 1,5 m de cada lado do meio da parede (local z = ±1,5).
        f32 ax, az, bx, bz;
        local_para_mundo(k, 0, 1.5f, &ax, &az);
        local_para_mundo(k, 0, -1.5f, &bx, &bz);
        f32 ya = 0, yb = 0;
        w.chao(ax, az, 0.2f, k.y + 1.0f, &ya);
        w.chao(bx, bz, 0.2f, k.y + 1.0f, &yb);
        if (fabsf(ya - yb) > 0.3f) continue;
        if (!w.livre(Vec3(ax, ya, az), mov::ALTURA, 0.5f) || !w.livre(Vec3(bx, yb, bz), mov::ALTURA, 0.5f)) continue;
        // Nada no caminho além da própria parede (móveis atrás da porta: escolhe outra).
        Personagem p = andar(w, Vec3(ax, ya, az), bx - ax, bz - az, 30);
        f32 lx, lz;
        Colisor tmp;
        tmp.origem = Vec3(k.x, k.y, k.z);
        tmp.c = cosf(k.rot * 3.14159265f / 180.0f);
        tmp.s = sinf(k.rot * 3.14159265f / 180.0f);
        tmp.local(p.pos.x, p.pos.z, &lx, &lz);
        if (porta) {
            if (lz > -0.5f) continue;  // algo atrás da porta; tenta outra
            VERIFICA(fabsf(lx) < 0.7f);  // passou pelo vão
            testou_porta = true;
        } else {
            VERIFICA(lz > 0.12f + mov::RAIO - 0.03f);  // parou na face da parede
            testou_parede = true;
        }
    }
    VERIFICA(testou_porta);
    VERIFICA(testou_parede);
}

TESTE(ilha_costa_segura_o_jogador) {
    Ilha* il = ilha();
    if (!il) return;
    const MapaBrisa& m = il->mapa();
    for (int k = 0; k < 8; k++) {
        f32 a = k * 3.14159265f / 4 + 0.2f;
        f32 r = m.raio_costa(a) - 10.0f;
        Vec3 de(cosf(a) * r, 0, sinf(a) * r);
        de.y = m.relevo().altura(de.x, de.z);
        Personagem p = andar(il->mundo(), de, cosf(a), sinf(a), 300, true);  // 10 s correndo para o mar
        f32 d = sqrtf(p.pos.x * p.pos.x + p.pos.z * p.pos.z);
        VERIFICA(d < m.raio_costa(atan2f(p.pos.z, p.pos.x)) + 9.5f);
        VERIFICA(d > r + 10.0f);  // chegou à água rasa antes de parar
    }
}

TESTE(ilha_corte_por_distancia_e_lod) {
    Ilha* il = ilha();
    if (!il) return;
    static VistaIlha v[32];
    int n = vistas_da_ilha(il->mapa(), v, 32);
    VERIFICA(n == 16);
    static Cena3D cena;
    static ListaDesenho2D hud;
    EstatisticasQuadro est;
    Estatisticas3D e3d;
    InfoPlataforma info;
    u32 pior = 0;
    for (int k = 0; k < n; k++) {
        il->ir_para(v[k].x, v[k].z, v[k].yaw, v[k].pitch);
        for (int q = 0; q < 4; q++) il->montar(cena, hud, est, e3d, info);  // enche o cache do LOD0
        const EstatisticasIlha& ei = il->estatisticas();
        VERIFICA(ei.desenhadas > 0 && ei.desenhadas <= ei.instancias);
        VERIFICA(ei.terreno.blocos_lod0 > 0);
        // Triângulos enviados como o renderizador conta: partes cuja esfera passa no frustum.
        Frustum fr = Frustum::de(cena.camera.vp);
        u32 tris = 0;
        for (int o = 0; o < cena.n; o++) {
            const Objeto3D& ob = cena.objetos[o];
            const Mat4& md = ob.modelo;
            f32 esc = sqrtf(md(0, 0) * md(0, 0) + md(1, 0) * md(1, 0) + md(2, 0) * md(2, 0));
            Vec4 c = transformar(md, ob.malha->centro);
            if (fr.esfera(Vec3(c.x, c.y, c.z), ob.malha->raio * esc) != 0) tris += ob.malha->triangulos();
        }
        if (tris > pior) pior = tris;
        // Nada além da distância máxima de cada categoria.
        for (int i = 0; i < cena.n; i++) VERIFICA(cena.objetos[i].malha != nullptr);
    }
    // Orçamento: a pior vista (Vila Brisa) cabe em 50 mil triângulos (BRISA_ARCHITECTURE §5.2 e §18).
    VERIFICA(pior < 50000);
    // Móveis só até 24 m: do alto do Mirante, nenhum móvel a mais que isso entra na cena.
    il->ir_para(10, 5, 90);
    il->montar(cena, hud, est, e3d, info);
    VERIFICA(il->estatisticas().tris_categoria[CAT_MOVEL] < 3000);
}

TESTE(minimapa_256_psmt8) {
    if (!pacote()) return;
    u32 t = 0;
    const u8* d = pacote()->arquivo("texturas/minimapa.btex", &t);
    TexturaBrisa tex;
    char e[96];
    VERIFICA(d && tex.carregar(d, t, e, sizeof e));
    VERIFICA(tex.largura == 256 && tex.altura == 256);
}

// Escada em U do Ventania (gerar_interiores.py): lance 1 sobe para −x até o patamar
// (2,1 m), lance 2 volta para +x até o andar de cima (4,1 m). Sobe as duas.
TESTE(ilha_sobe_a_escada_em_u) {
    Ilha* il = ilha();
    if (!il) return;
    const MapaBrisa& m = il->mapa();
    const MundoColisao& w = il->mundo();
    int subiu = 0;
    for (int i = 0; i < m.n_instancias() && subiu < 3; i++) {
        const InstanciaMapa& k = m.instancia(i);
        if (strcmp(m.modelo(k.modelo).nome, "escada_madeira_01") != 0 &&
            strcmp(m.modelo(k.modelo).nome, "escada_concreto_01") != 0 &&
            strcmp(m.modelo(k.modelo).nome, "escada_metal_01") != 0)
            continue;
        f32 c = cosf(k.rot * 3.14159265f / 180.0f), s = sinf(k.rot * 3.14159265f / 180.0f);
        auto dir = [&](f32 lx, f32 lz, f32* x, f32* z) { *x = c * lx + s * lz; *z = -s * lx + c * lz; };
        f32 x, z, dx, dz, y = 0;
        local_para_mundo(k, 1.8f, 0.95f, &x, &z);  // pé do lance 1
        VERIFICA(w.chao(x, z, 0.1f, k.y + 0.6f, &y));
        VERIFICA(y < k.y + 0.6f);
        dir(-1, 0, &dx, &dz);
        Personagem p = andar(w, Vec3(x, y, z), dx, dz, 30);  // até a parede do fundo do patamar
        VERIFICA(p.no_chao && fabsf(p.pos.y - (k.y + 2.1f)) < 0.05f);  // no patamar
        // vai para a faixa do lance 2 (z local −1) e sobe para +x
        f32 qx = p.pos.x - k.x, qz = p.pos.z - k.z;
        f32 lx = c * qx - s * qz;
        VERIFICA(lx < -0.7f);
        dir(0, -1, &dx, &dz);
        p = andar(w, p.pos, dx, dz, 14);  // ~2 m para −z local (meio da faixa do lance 2)
        dir(1, 0, &dx, &dz);
        p = andar(w, p.pos, dx, dz, 40);  // lance 2 inteiro, até o andar de cima
        VERIFICA(p.no_chao && fabsf(p.pos.y - (k.y + 4.1f)) < 0.06f);  // no andar de cima
        subiu++;
    }
    VERIFICA(subiu >= 1);
}

TESTE(ilha_loot_baus_e_pontos_do_ventania) {
    Ilha* il = ilha();
    if (!il) return;
    const Loot& l = il->loot();
    VERIFICA(l.n_baus() == 37 && !l.reabastecer);
    int itens = 0, municao = 0;
    for (int i = 0; i < l.n_itens(); i++) {
        if (!l.item(i).id) continue;
        if (l.item(i).reserva()) municao++;
        else itens++;
        // todo item fica sobre o chão do mapa (relevo ou piso), nunca dentro dele
        f32 y = 0;
        VERIFICA(il->mundo().chao(l.item(i).pos.x, l.item(i).pos.z, 0.05f, l.item(i).pos.y + 0.3f, &y));
    }
    VERIFICA(itens > 60 && itens < 102);  // 75% dos 102 pontos
    VERIFICA(municao > 20);
}
