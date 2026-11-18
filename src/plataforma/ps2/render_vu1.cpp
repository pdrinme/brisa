#include "render_vu1.h"

#include <dmaKit.h>
#include <kernel.h>
#include <math.h>
#include <packet2.h>
#include <packet2_utils.h>
#include <string.h>

#include "gs_pacotes.h"

extern "C" {
extern u32 brisa_vu1_toon_inicio __attribute__((section(".vudata")));
extern u32 brisa_vu1_toon_fim __attribute__((section(".vudata")));
}

namespace {
const int MAX_OBJ_VU = 2048;
const int QW_CABECALHO = 12;  // qw 1..12 do lote
const u32 QW_CADEIA = 40000;
alignas(64) f32 g_cabecalhos[2][MAX_OBJ_VU][QW_CABECALHO * 4];
alignas(64) u32 g_contagens[vu1::MAX_VERTS_LOTE / 3 + 1][4];  // qw 0 de cada tamanho de lote
packet2_t* g_cadeia[2] = {nullptr, nullptr};
int g_atual = 0;
bool g_pronto = false;

void enviar_e_esperar(packet2_t* p) {
    dmaKit_wait(DMA_CHANNEL_VIF1, 0);
    dmaKit_send_chain(DMA_CHANNEL_VIF1, p->base, packet2_get_qw_count(p));
    dmaKit_wait(DMA_CHANNEL_VIF1, 0);
}

void unpack_ref(packet2_t* p, const void* dados, u32 qw, enum UnpackMode modo, u32 endereco, u32 num, bool sem_sinal) {
    packet2_chain_ref(p, dados, qw, 0, 0, 0);
    packet2_vif_stcycl(p, 0, 0x0101, 0);
    packet2_vif_open_unpack(p, modo, endereco, 1, 0, sem_sinal ? 1 : 0, 0);
    packet2_vif_close_unpack_manual(p, num);
}
}  // namespace

namespace vu1 {

bool iniciar() {
    u32 tam = packet2_utils_get_packet_size_for_program(&brisa_vu1_toon_inicio, &brisa_vu1_toon_fim) + 1;
    packet2_t* p = packet2_create(tam, P2_TYPE_NORMAL, P2_MODE_CHAIN, 1);
    if (!p) return false;
    packet2_vif_add_micro_program(p, 0, &brisa_vu1_toon_inicio, &brisa_vu1_toon_fim);
    packet2_utils_vu_add_end_tag(p);
    enviar_e_esperar(p);
    packet2_free(p);
    // Buffer duplo: BASE 0, OFFSET 512 → lotes alternam entre qw 0 e qw 512.
    p = packet2_create(2, P2_TYPE_NORMAL, P2_MODE_CHAIN, 1);
    packet2_utils_vu_add_double_buffer(p, 0, 512);
    packet2_utils_vu_add_end_tag(p);
    enviar_e_esperar(p);
    packet2_free(p);
    for (int k = 0; k <= MAX_VERTS_LOTE / 3; k++) {
        g_contagens[k][0] = k * 3;
        g_contagens[k][1] = g_contagens[k][2] = g_contagens[k][3] = 0;
    }
    g_cadeia[0] = packet2_create(QW_CADEIA, P2_TYPE_NORMAL, P2_MODE_CHAIN, 1);
    g_cadeia[1] = packet2_create(QW_CADEIA, P2_TYPE_NORMAL, P2_MODE_CHAIN, 1);
    g_pronto = g_cadeia[0] && g_cadeia[1];
    return g_pronto;
}

void desenhar(const Cena3D& cena, SaidaTriangulos& ee, Estatisticas3D& est) {
    if (!g_pronto) return;
    packet2_t* p = g_cadeia[g_atual];
    f32 (*cab)[QW_CABECALHO * 4] = g_cabecalhos[g_atual];
    g_atual ^= 1;
    packet2_reset(p, 0);
    Frustum fr = Frustum::de(cena.camera.vp);
    CoeficientesToon k = CoeficientesToon::de(cena.luz);
    const Camera3D& cam = cena.camera;
    f32 kneb = 255.0f / (cena.neblina.fim - cena.neblina.inicio > 0.01f ? cena.neblina.fim - cena.neblina.inicio : 0.01f);
    int n_vu = 0;
    // Reserva para fechar a cadeia (FLUSH + END) e para o pior lote (6 tags).
    const u32 RESERVA = 16;
    for (int i = 0; i < cena.n; i++) {
        const Objeto3D& o = cena.objetos[i];
        const Malha& m = *o.malha;
        est.objetos++;
        f32 esc = sqrtf(o.modelo(0, 0) * o.modelo(0, 0) + o.modelo(1, 0) * o.modelo(1, 0) + o.modelo(2, 0) * o.modelo(2, 0));
        Vec4 c = transformar(o.modelo, m.centro);
        bool cruza_perto = false;
        if (fr.esfera(Vec3(c.x, c.y, c.z), m.raio * esc, &cruza_perto) == 0) continue;
        est.objetos_visiveis++;
        est.triangulos_enviados += m.triangulos();
        u32 lotes = (m.n + MAX_VERTS_LOTE - 1) / MAX_VERTS_LOTE;
        if (cruza_perto || n_vu >= MAX_OBJ_VU || packet2_get_qw_count(p) + lotes * 8 + RESERVA > QW_CADEIA) {
            est.objetos_cortados_perto += cruza_perto ? 1 : 0;
            est.triangulos_desenhados += pipeline_objeto(cena, o, ConfigTela(), ee);
            continue;
        }
        // Cabeçalho do objeto (qw 1..12 do lote).
        f32* h = cab[n_vu++];
        Mat4 mvp = cam.vp * o.modelo;
        memcpy(h, mvp.m, 16 * sizeof(f32));
        for (int k = 0; k < 12; k++) h[k] *= m.escala;  // posições int16: a escala entra nas colunas x, y, z
        Vec3 l = normalizar(transformar_direcao(inversa_rigida(o.modelo), normalizar(cena.luz.direcao))) * (1.0f / 127.0f);
        f32* q = h + 16;
        q[0] = l.x; q[1] = l.y; q[2] = l.z; q[3] = k.a;                                     // 5
        q[4] = cena.luz.luz.x - cena.luz.sombra.x; q[5] = cena.luz.luz.y - cena.luz.sombra.y;  // 6
        q[6] = cena.luz.luz.z - cena.luz.sombra.z; q[7] = 0;
        q[8] = cena.luz.sombra.x; q[9] = cena.luz.sombra.y; q[10] = cena.luz.sombra.z; q[11] = k.b;  // 7
        // 8: o ftoi4 dá Z·16 = Z<<4, que é a posição do Z no XYZF2 (bits 4..27)
        q[12] = 320.0f; q[13] = -224.0f; q[14] = cam.perto * gs3d::Z_MAX; q[15] = 0;
        q[16] = gs::CENTRO_X; q[17] = gs::CENTRO_Y; q[18] = 0; q[19] = 0;                       // 9
        q[20] = -kneb; q[21] = cena.neblina.fim * kneb; q[22] = 255.0f; q[23] = 0.25f;          // 10
        q[24] = 255.0f; q[25] = 0; q[26] = 0; q[27] = 128.0f;                                    // 11
        u64* tag = reinterpret_cast<u64*>(q + 28);                                              // 12
        tag[0] = gs::giftag_lo(0, true, true, gs::PRIM_TRI_GOURAUD_NEBLINA, 2);
        tag[1] = gs::REG_RGBAQ | (gs::REG_XYZF2 << 4);
        for (u32 ini = 0; ini < m.n; ini += MAX_VERTS_LOTE) {
            u32 n = m.n - ini < static_cast<u32>(MAX_VERTS_LOTE) ? m.n - ini : MAX_VERTS_LOTE;
            unpack_ref(p, g_contagens[n / 3], 1, P2_UNPACK_V4_32, 0, 1, false);
            unpack_ref(p, h, QW_CABECALHO, P2_UNPACK_V4_32, 1, QW_CABECALHO, false);
            // V4-16: 2 vértices por qword; com n ímpar, o vetor a mais cai no início
            // das normais, que vêm depois e o sobrescrevem.
            u32 qw16 = (n + 1) / 2;
            unpack_ref(p, m.pos + ini * 4, qw16, P2_UNPACK_V4_16, 13, qw16 * 2, false);
            // V4-8: o VIF consome qwords inteiros (4 vetores). Desempacota até o
            // múltiplo de 4: os 1 a 3 vetores a mais caem no início da área seguinte,
            // que é sobrescrita depois (cores, e a saída do VU1). Sem isso, os bytes
            // que sobram no qword seriam lidos como comandos do VIF.
            u32 qw8 = (n + 3) / 4;
            unpack_ref(p, m.normal + ini, qw8, P2_UNPACK_V4_8, 13 + n, qw8 * 4, false);
            unpack_ref(p, m.cor + ini, qw8, P2_UNPACK_V4_8, 13 + 2 * n, qw8 * 4, true);
            packet2_chain_open_cnt(p, 0, 0, 0);
            packet2_vif_mscal(p, 0, 0);
            packet2_vif_nop(p, 0);
            packet2_chain_close_tag(p);
            est.lotes_vu1++;
        }
    }
    // FLUSH: o DMA só termina depois que o VU1 acabou e o PATH1 esvaziou.
    packet2_chain_open_end(p, 0, 0);
    packet2_vif_flush(p, 0);
    packet2_vif_nop(p, 0);
    packet2_chain_close_tag(p);
    FlushCache(0);  // cabeçalhos e malhas escritos pelo EE chegam à RAM antes do DMA
    if (est.lotes_vu1 > 0) enviar_e_esperar(p);
}

}  // namespace vu1
