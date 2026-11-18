#include <math.h>
#include <string.h>

#include "conteudo/animacao.h"
#include "conteudo/modelo.h"
#include "conteudo/pak.h"
#include "conteudo/textura.h"
#include "nucleo/crc32.h"
#include "plataforma/plataforma.h"
#include "render3d/instancia.h"
#include "teste.h"

namespace {
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
bool modelo(const char* nome, ModeloBrisa& m) {
    u32 t = 0;
    const u8* d = pacote()->arquivo(nome, &t);
    char e[128];
    return d && m.carregar(d, t, e, sizeof e);
}
}  // namespace

TESTE(crc32_valor_conhecido) {
    VERIFICA(crc32("123456789", 9) == 0xCBF43926u);
    VERIFICA(crc32("6789", 4, crc32("12345", 5)) == 0xCBF43926u);  // incremental, como o zlib
}

TESTE(pacote_abre_e_todos_os_crcs_conferem) {
    const Pacote* p = pacote();
    VERIFICA(p != nullptr);
    if (!p) return;
    char e[128] = "";
    VERIFICA(p->verificar(e, sizeof e) == 0);
    VERIFICA(p->n() >= 100);
    VERIFICA(p->arquivo("modelos/recruta_base_01.bmod") != nullptr);
    VERIFICA(p->arquivo("modelos/nao_existe.bmod") == nullptr);
    for (u32 i = 1; i < p->n(); i++) VERIFICA(strcmp(p->nome(i - 1), p->nome(i)) < 0);  // ordenado (busca binária)
}

TESTE(pacote_recusa_corrompido) {
    const Pacote* p = pacote();
    if (!p) return;
    u32 t = 0;
    const u8* d = plataforma::conteudo(&t);
    static u8 copia[64 * 64] __attribute__((aligned(64)));
    memcpy(copia, d, sizeof copia);
    copia[64 + 3] ^= 0x55;  // mexe no índice
    Pacote q;
    char e[128];
    VERIFICA(!q.abrir(copia, t, e, sizeof e));
    VERIFICA(strstr(e, "CRC") != nullptr || strstr(e, "truncado") != nullptr);
}

TESTE(modelo_personagem_partes_alinhadas_para_o_vif) {
    if (!pacote()) return;
    ModeloBrisa m;
    VERIFICA(modelo("modelos/recruta_base_01.bmod", m));
    VERIFICA(m.esqueletico());
    VERIFICA(m.triangulos() == 2790);
    VERIFICA(m.n_partes() == 17 && m.n_nos() == 22);
    for (int i = 0; i < m.n_partes(); i++) {
        const Malha& k = m.malha(i);
        VERIFICA((reinterpret_cast<uintptr_t>(k.pos) & 15) == 0);
        VERIFICA((reinterpret_cast<uintptr_t>(k.normal) & 15) == 0);
        VERIFICA((reinterpret_cast<uintptr_t>(k.cor) & 15) == 0);
        VERIFICA(k.n % 3 == 0 && k.raio > 0);
        VERIFICA(m.no_da_parte(i) >= 0);
    }
    ModeloBrisa lod;
    VERIFICA(modelo("modelos/recruta_base_01_lod2.bmod", lod));
    VERIFICA(lod.triangulos() <= 320 && lod.assinatura() == m.assinatura());
}

TESTE(animacoes_servem_em_todos_os_trajes_e_mexem) {
    if (!pacote()) return;
    u32 t = 0;
    const u8* d = pacote()->arquivo("animacoes/humanoide.bani", &t);
    BancoAnimacoes b;
    char e[128];
    VERIFICA(d && b.carregar(d, t, e, sizeof e));
    VERIFICA(b.n_clipes() == 16);
    int idle = b.achar("idle"), acenar = b.achar("acenar");
    VERIFICA(idle >= 0 && b.loop(idle) && acenar >= 0 && !b.loop(acenar));
    const char* trajes[] = {"recruta_base_01", "capitao_mare_01", "guardiao_01", "espantalho_01"};
    for (const char* id : trajes) {
        char nome[64];
        snprintf(nome, sizeof nome, "modelos/%s.bmod", id);
        ModeloBrisa m;
        VERIFICA(modelo(nome, m) && m.assinatura() == b.assinatura());
    }
    ModeloBrisa m;
    modelo("modelos/recruta_base_01.bmod", m);
    int mao = m.achar_no("RightHand");
    VERIFICA(mao >= 0);
    Pose p0, p1;
    b.amostrar(acenar, 0.0f, m, p0);
    b.amostrar(acenar, 0.75f, m, p1);
    // Mão direita sobe no aceno.
    VERIFICA(p1.mundo[mao].m[13] > p0.mundo[mao].m[13] + 0.2f);
    // Laço: t e t + período dão a mesma pose.
    Pose a, c;
    b.amostrar(idle, 0.5f, m, a);
    b.amostrar(idle, 0.5f + b.duracao(idle), m, c);
    VERIFICA(fabsf(a.mundo[mao].m[12] - c.mundo[mao].m[12]) < 1e-3f);
    // A cena recebe uma parte por osso usado.
    static Cena3D cena;
    cena.limpar();
    VERIFICA(adicionar_modelo(cena, m, &p1, Mat4::identidade()) == m.n_partes());
}

TESTE(bau_abre_a_tampa) {
    if (!pacote()) return;
    ModeloBrisa m;
    VERIFICA(modelo("modelos/bau_ventania_01.bmod", m));
    u32 t = 0;
    const u8* d = pacote()->arquivo("animacoes/bau_ventania_01.bani", &t);
    BancoAnimacoes b;
    char e[128];
    VERIFICA(d && b.carregar(d, t, e, sizeof e) && b.assinatura() == m.assinatura());
    int marca = -1;
    for (int i = 0; i < m.n_marcadores(); i++)
        if (strcmp(m.marcador(i).nome, "alca_tampa") == 0) marca = i;
    VERIFICA(marca >= 0);
    Pose fechado, aberto;
    b.amostrar(b.achar("abrir"), 0, m, fechado);
    b.amostrar(b.achar("abrir"), 10.0f, m, aberto);
    // O centro da tampa (parte presa ao nó animado) sobe e vai para trás ao abrir.
    int parte = -1;
    for (int i = 0; i < m.n_partes(); i++)
        if (m.no_da_parte(i) >= 0) parte = i;
    VERIFICA(parte >= 0);
    if (parte < 0) return;
    int no = m.no_da_parte(parte);
    Mat4 inv;
    for (int k = 0; k < 16; k++) inv.m[k] = m.no(no).inv_bind[k];
    Vec4 c0 = transformar(fechado.mundo[no] * inv, m.malha(parte).centro);
    Vec4 c1 = transformar(aberto.mundo[no] * inv, m.malha(parte).centro);
    VERIFICA(c1.y > c0.y + 0.15f && c1.z < c0.z - 0.1f);
    VERIFICA(fabsf(c0.y - m.malha(parte).centro.y) < 1e-3f);  // fechado = repouso
    VERIFICA(m.n_colisoes() == 1);
}

TESTE(textura_icone_e_sombra) {
    if (!pacote()) return;
    u32 t = 0;
    TexturaBrisa tx;
    char e[128];
    const u8* d = pacote()->arquivo("texturas/icone_rajada.btex", &t);
    VERIFICA(d && tx.carregar(d, t, e, sizeof e));
    VERIFICA(tx.largura == 64 && tx.psm == TexturaBrisa::PSM_T8 && (reinterpret_cast<uintptr_t>(tx.pixels) & 15) == 0);
    VERIFICA((tx.rgba(0, 0) >> 24) == 0);  // canto transparente
    d = pacote()->arquivo("texturas/sombra_blob.btex", &t);
    VERIFICA(d && tx.carregar(d, t, e, sizeof e) && tx.psm == TexturaBrisa::PSM_T4);
    VERIFICA((tx.rgba(16, 16) >> 24) > (tx.rgba(1, 16) >> 24));
    VERIFICA((tx.rgba(16, 16) >> 24) <= 128);  // alfa na escala do GS
}
