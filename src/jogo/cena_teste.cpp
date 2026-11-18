#include "cena_teste.h"

#include <math.h>
#include <stdio.h>

#include "nucleo/fonte.h"
#include "nucleo/formato.h"
#include "render3d/malha.h"

namespace {
// Paleta do Ventania (tools/assets/kit/paleta.json).
const Cor MADEIRA_CLARA = rgb(0xE3A963), MADEIRA_MEDIA = rgb(0xB8773D), MADEIRA_ESCURA = rgb(0x7A4A26);
const Cor PEDRA_CLARA = rgb(0xA7A196), PEDRA_MEDIA = rgb(0x7F7A73), PEDRA_ESCURA = rgb(0x55524F);
const Cor METAL_CLARO = rgb(0xAEC0CC), METAL_ESCURO = rgb(0x5A6B7A);
const Cor FOLHA_CLARA = rgb(0x86DB5C), FOLHA_MEDIA = rgb(0x4DB043), FOLHA_ESCURA = rgb(0x2E7D3C);
const Cor TRONCO = rgb(0x8A5A3B), GRAMA = rgb(0x6CC24A);
const Cor TEAL = rgb(0x22B8A8), TEAL_ESCURO = rgb(0x167C84), AMARELO_SOL = rgb(0xFFC93C), CORAL = rgb(0xFF6B5B);
const Cor GRAMA_2 = rgb(0x62B444);
// Tema da interface.
const Cor BRANCO = rgb(0xf6f3ec), TINTA = rgb(0x15161c), AMARELO = rgb(0xffc93c);
const Cor NOITE_CARTAO = rgb(0x101c33, 215);

const f32 AREA = 150.0f;  // lado da área com objetos (m)

f32 relevo(f32 x, f32 z) { return 1.2f * sinf(x * 0.045f) * cosf(z * 0.05f) + 0.6f * sinf((x + z) * 0.09f); }

u32 aleatorio(u32& s) {
    s = s * 1664525u + 1013904223u;
    return s >> 8;
}

void casa(ConstrutorMalha& m) {
    // 4 × 4 m, paredes de tábuas verticais alternadas, como as peças de madeira do kit.
    const f32 L = 4.0f, H = 3.2f, E = 0.24f;
    for (int lado = 0; lado < 4; lado++)
        for (int t = 0; t < 5; t++) {
            f32 a = t * (L / 5), b = (t + 1) * (L / 5) - 0.02f;
            Cor c = (t + lado) % 2 ? MADEIRA_CLARA : MADEIRA_MEDIA;
            if (lado == 0) m.caixa(Vec3(a, 0, L - E), Vec3(b, H, L), (t == 2) ? MADEIRA_ESCURA : c);  // porta
            if (lado == 1) m.caixa(Vec3(a, 0, 0), Vec3(b, H, E), c);
            if (lado == 2) m.caixa(Vec3(0, 0, a), Vec3(E, H, b), c);
            if (lado == 3) m.caixa(Vec3(L - E, 0, a), Vec3(L, H, b), c);
        }
    m.piramide(Vec3(-0.2f, H, -0.2f), Vec3(L + 0.2f, H, L + 0.2f), 1.6f, MADEIRA_ESCURA);
}

void torre(ConstrutorMalha& m) {
    for (int i = 0; i < 6; i++) {
        f32 y = i * 1.4f;
        f32 r = (i % 2) ? 1.9f : 2.0f;
        m.caixa(Vec3(-r, y, -r), Vec3(r, y + 1.36f, r), (i % 2) ? PEDRA_CLARA : PEDRA_MEDIA);
    }
    for (int k = 0; k < 4; k++) {  // ameias
        f32 x = (k % 2) ? 1.2f : -2.0f, z = (k / 2) ? 1.2f : -2.0f;
        m.caixa(Vec3(x, 8.4f, z), Vec3(x + 0.8f, 9.2f, z + 0.8f), PEDRA_ESCURA);
    }
}

void galpao(ConstrutorMalha& m) {
    for (int t = 0; t < 8; t++) {
        f32 a = t * 1.0f;
        Cor c = t % 2 ? METAL_CLARO : METAL_ESCURO;
        m.caixa(Vec3(a, 0, 0), Vec3(a + 0.98f, 4.0f, 6.0f), c);
    }
    m.caixa(Vec3(-0.2f, 4.0f, -0.2f), Vec3(8.2f, 4.3f, 6.2f), METAL_ESCURO);
}

void pinheiro(ConstrutorMalha& m) {
    m.cilindro(Vec3(0, 0, 0), 0.28f, 1.4f, 6, TRONCO);
    m.cone(Vec3(0, 1.2f, 0), 1.6f, 2.4f, 8, FOLHA_MEDIA);
    m.cone(Vec3(0, 2.6f, 0), 1.15f, 2.1f, 8, FOLHA_ESCURA);
}

void arvore(ConstrutorMalha& m) {
    m.cilindro(Vec3(0, 0, 0), 0.32f, 2.2f, 6, TRONCO);
    m.esfera(Vec3(0, 3.4f, 0), 1.9f, 1, FOLHA_CLARA, 0.85f);
}

void bau(ConstrutorMalha& m) {
    // 1,1 × 0,7 × 0,68 m (guia de estilo do Ventania): azul-petróleo com amarelo.
    m.caixa(Vec3(-0.55f, 0, -0.35f), Vec3(0.55f, 0.42f, 0.35f), TEAL);
    m.caixa(Vec3(-0.57f, 0.42f, -0.37f), Vec3(0.57f, 0.68f, 0.37f), TEAL_ESCURO);
    m.caixa(Vec3(-0.6f, 0.36f, -0.4f), Vec3(0.6f, 0.46f, 0.4f), AMARELO_SOL);
    m.caixa(Vec3(-0.1f, 0.25f, 0.36f), Vec3(0.1f, 0.5f, 0.42f), AMARELO_SOL);  // fecho
}

void pedra(ConstrutorMalha& m) { m.esfera(Vec3(0, 0.3f, 0), 0.9f, 0, PEDRA_MEDIA, 0.7f); }

void gerar_carga(ConstrutorMalha& m) { m.esfera(Vec3(0, 2.5f, 0), 2.5f, 3, CORAL); }

f32 pos_x(u32 i, u32 semente) {
    u32 s = semente ^ (i * 2654435761u);
    return (static_cast<f32>(aleatorio(s) % 10000) / 10000.0f - 0.5f) * AREA;
}

void card(ListaDesenho2D& l, f32 x, f32 y, f32 w, f32 h) {
    l.retangulo(x + 4, y + 4, w, h, rgb(0x15161c, 140));
    l.retangulo(x, y, w, h, NOITE_CARTAO);
    l.moldura(x, y, w, h, 2, TINTA);
}

const char* nome_caminho(Caminho3D c) { return c == Caminho3D::VU1 ? "VU1" : "EE"; }
}  // namespace

bool CenaTeste::iniciar(Arena& a) {
    struct Gerador { void (*f)(ConstrutorMalha&); u32 max_tris; };
    ConstrutorMalha chao(a, 32 * 32 * 2);
    chao.grade(-AREA * 0.6f, -AREA * 0.6f, AREA * 1.2f, 32, GRAMA, GRAMA_2, relevo);
    malhas_[CHAO] = chao.terminar();
    const Gerador g[] = {{casa, 300}, {torre, 200}, {galpao, 120}, {pinheiro, 80}, {arvore, 120}, {bau, 60}, {pedra, 30}, {gerar_carga, 1400}};
    for (int i = 0; i < 8; i++) {
        ConstrutorMalha m(a, g[i].max_tris);
        if (!m.ok()) return false;
        g[i].f(m);
        if (m.estourou()) return false;
        malhas_[CASA + i] = m.terminar();
    }
    return malhas_[CHAO].n > 0;
}

u32 CenaTeste::instancias() const { return carga_ ? (nivel_ < 2 ? 4u << nivel_ : 16u * (nivel_ - 1)) : 24u << nivel_; }

void CenaTeste::definir_nivel(int nivel, bool c) {
    carga_ = c;
    int max = c ? NIVEIS_CARGA : NIVEIS;
    nivel_ = nivel < 0 ? 0 : (nivel >= max ? max - 1 : nivel);
}

void CenaTeste::passo(const Controle& c) {
    passos_++;
    if (c.pressionou(BT_TRIANGULO) && !bench_ativo_) iniciar_bench();
    if (bench_ativo_) {
        angulo_ += 0.01f;  // trajeto fixo da câmera durante o benchmark
        return;
    }
    if (c.pressionou(BT_START)) pausado_ = !pausado_;
    if (c.pressionou(BT_SELECT)) {
        Caminho3D outro = caminho_ == Caminho3D::VU1 ? Caminho3D::EE : Caminho3D::VU1;
        if (plataforma::caminho_disponivel(outro)) caminho_ = outro;
    }
    if (c.pressionou(BT_R1)) definir_nivel(nivel_ + 1, carga_);
    if (c.pressionou(BT_L1)) definir_nivel(nivel_ - 1, carga_);
    if (c.pressionou(BT_CIRCULO)) definir_nivel(carga_ ? 3 : 1, !carga_);
    f32 lx = Controle::eixo(c.estado().lx), ly = Controle::eixo(c.estado().ly);
    f32 ry = Controle::eixo(c.estado().ry);
    angulo_ += lx * 0.05f + (pausado_ ? 0.0f : 0.004f);
    distancia_ += ly * 1.5f;
    altura_ -= ry * 0.8f;
    distancia_ = distancia_ < 8 ? 8 : (distancia_ > 160 ? 160 : distancia_);
    altura_ = altura_ < 2 ? 2 : (altura_ > 90 ? 90 : altura_);
}

void CenaTeste::montar_instancias(Cena3D& cena) const {
    cena.adicionar(&malhas_[CHAO], Mat4::identidade());
    u32 n = instancias();
    for (u32 i = 0; i < n; i++) {
        f32 x = pos_x(i, 0x5eed1u), z = pos_x(i, 0xb215au);
        u32 s = i * 747796405u + 2891336453u;
        int tipo;
        if (carga_) {
            tipo = CARGA;
        } else {
            u32 r = aleatorio(s) % 100;
            tipo = r < 12 ? CASA : r < 16 ? TORRE : r < 20 ? GALPAO : r < 52 ? PINHEIRO : r < 74 ? ARVORE : r < 84 ? BAU : PEDRA;
        }
        f32 rot = static_cast<f32>(aleatorio(s) % 628) / 100.0f;
        Mat4 mdl = Mat4::translacao(Vec3(x, relevo(x, z) - 0.05f, z)) * Mat4::rotacao_y(rot);
        cena.adicionar(&malhas_[tipo], mdl);
    }
}

void CenaTeste::montar(Cena3D& cena, ListaDesenho2D& hud, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                       const InfoPlataforma& info) {
    cena.limpar();
    Vec3 olho(cosf(angulo_) * distancia_, altura_, sinf(angulo_) * distancia_);
    // A TV é 4:3 mesmo com o framebuffer em 640×448.
    cena.camera.configurar(olho, Vec3(0, 2.0f, 0), 55.0f * 3.14159265f / 180.0f, 4.0f / 3.0f, 0.5f, 400.0f);
    montar_instancias(cena);
    hud.limpar(cena.ceu);
    montar_hud(hud, est, e3d, info);
}

void CenaTeste::montar_hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                           const InfoPlataforma& info) const {
    char linha[96], a[16], b[16], c[16], d[16];
    card(l, 24, 20, 592, 98);
    snprintf(linha, sizeof linha, "MOTOR 3D · CAMINHO %s · %s %d", nome_caminho(caminho_), carga_ ? "CARGA" : "CENA",
             nivel_ + 1);
    fonte::desenhar(l, 36, 26, 2, AMARELO, linha);
    snprintf(linha, sizeof linha, "OBJETOS %u/%u · TRIÂNGULOS %u", static_cast<unsigned>(e3d.objetos_visiveis),
             static_cast<unsigned>(e3d.objetos), static_cast<unsigned>(e3d.triangulos_enviados));
    fonte::desenhar(l, 36, 48, 2, BRANCO, linha);
    formato::decimal(a, sizeof a, est.media_quadro_us(), 1000, 1);
    formato::decimal(b, sizeof b, est.media_trabalho_us(), 1000, 1);
    formato::decimal(c, sizeof c, e3d.tempo_3d_us, 1000, 1);
    formato::decimal(d, sizeof d, est.fps_x10(), 10, 1);
    snprintf(linha, sizeof linha, "QUADRO %s · CPU %s · 3D %s MS · %s FPS", a, b, c, d);
    fonte::desenhar(l, 36, 70, 2, BRANCO, linha);
    snprintf(linha, sizeof linha, "%s · %s", info.nome, info.video);
    fonte::desenhar(l, 36, 92, 2, rgb(0xf6f3ec, 170), linha);

    if (bench_ativo_ || n_res_ > 0) {
        int linhas = n_res_ < 11 ? n_res_ : 11;
        f32 h = 30.0f + 20.0f * (linhas > 0 ? linhas : 1);
        card(l, 24, 128, 592, h);
        fonte::desenhar(l, 36, 134, 2, AMARELO, bench_ativo_ ? "BENCHMARK EM ANDAMENTO..." : "BENCHMARK (MS: QUADRO/CPU/3D)");
        int inicio = n_res_ > 11 ? n_res_ - 11 : 0;
        for (int i = inicio; i < n_res_; i++) {
            const ResultadoBench& r = res_[i];
            formato::decimal(a, sizeof a, r.quadro_us, 1000, 1);
            formato::decimal(b, sizeof b, r.cpu_us, 1000, 1);
            formato::decimal(c, sizeof c, r.t3d_us, 1000, 1);
            snprintf(linha, sizeof linha, "%-3s %s%d %6u TRI  %s / %s / %s", nome_caminho(r.caminho), r.carga ? "C" : "N",
                     r.nivel + 1, static_cast<unsigned>(r.triangulos), a, b, c);
            fonte::desenhar(l, 36, 156 + 20.0f * (i - inicio), 2, BRANCO, linha);
        }
    }
    card(l, 16, 392, 608, 50);
    fonte::desenhar(l, 24, 398, 2, AMARELO, "SEL CAMINHO · L1/R1 NÍVEL · O CARGA · START PAUSA");
    fonte::desenhar(l, 24, 420, 2, AMARELO, "TRIÂNGULO BENCHMARK · ANALÓGICOS: CÂMERA");
}

// ---------------- Benchmark automático ----------------
// Para cada caminho disponível, percorre os níveis da cena e da carga: 30
// quadros de aquecimento e 60 de medição por configuração, com a câmera num
// trajeto fixo. Os resultados vão para a tela e para o log (BRISA_BENCH ...).
namespace {
#ifndef BRISA_BENCH_AQUECER
#define BRISA_BENCH_AQUECER 30
#define BRISA_BENCH_MEDIR 60
#endif
// O autoteste do emulador usa menos quadros (o Play! é lento no caminho do EE e
// os tempos dele não valem); no console vale o padrão.
const u32 AQUECER = BRISA_BENCH_AQUECER, MEDIR = BRISA_BENCH_MEDIR;
struct CfgBench { bool carga; int nivel; };
CfgBench cfg_de(int i) {
    int por_caminho = CenaTeste::NIVEIS + CenaTeste::NIVEIS_CARGA;
    int k = i % por_caminho;
    return k < CenaTeste::NIVEIS ? CfgBench{false, k} : CfgBench{true, k - CenaTeste::NIVEIS};
}
}  // namespace

void CenaTeste::iniciar_bench() {
    bench_ativo_ = true;
    bench_cfg_ = 0;
    n_res_ = 0;
    angulo_ = 0.6f;
    distancia_ = 70.0f;
    altura_ = 26.0f;
    aplicar_config_bench();
}

void CenaTeste::aplicar_config_bench() {
    int por_caminho = NIVEIS + NIVEIS_CARGA;
    Caminho3D c = bench_cfg_ < por_caminho ? Caminho3D::EE : Caminho3D::VU1;
    caminho_ = c;
    CfgBench k = cfg_de(bench_cfg_);
    definir_nivel(k.nivel, k.carga);
    bench_quadro_ = 0;
    soma_q_ = soma_c_ = soma_3d_ = 0;
    amostras_ = 0;
}

void CenaTeste::medir(u32 quadro_us, u32 cpu_us, const Estatisticas3D& e3d) {
    ultimo_e3d_ = e3d;
    if (!bench_ativo_) return;
    bench_quadro_++;
    if (bench_quadro_ > AQUECER) {
        soma_q_ += quadro_us;
        soma_c_ += cpu_us;
        soma_3d_ += e3d.tempo_3d_us;
        amostras_++;
    }
    if (bench_quadro_ < AQUECER + MEDIR) return;
    if (n_res_ < MAX_RES) {
        ResultadoBench& r = res_[n_res_++];
        r.caminho = caminho_;
        r.carga = carga_;
        r.nivel = nivel_;
        r.objetos = e3d.objetos_visiveis;
        r.triangulos = e3d.triangulos_enviados;
        r.quadro_us = static_cast<u32>(soma_q_ / amostras_);
        r.cpu_us = static_cast<u32>(soma_c_ / amostras_);
        r.t3d_us = static_cast<u32>(soma_3d_ / amostras_);
        char msg[160];
        snprintf(msg, sizeof msg, "BRISA_BENCH caminho=%s carga=%d nivel=%d objetos=%u tris=%u quadro_us=%u cpu_us=%u t3d_us=%u\n",
                 nome_caminho(r.caminho), r.carga ? 1 : 0, r.nivel + 1, static_cast<unsigned>(r.objetos),
                 static_cast<unsigned>(r.triangulos), static_cast<unsigned>(r.quadro_us),
                 static_cast<unsigned>(r.cpu_us), static_cast<unsigned>(r.t3d_us));
        plataforma::log(msg);
    }
    bench_cfg_++;
    int total = (NIVEIS + NIVEIS_CARGA) * (plataforma::caminho_disponivel(Caminho3D::VU1) ? 2 : 1);
    if (bench_cfg_ >= total) {
        bench_ativo_ = false;
        plataforma::log("BRISA_BENCH_FIM\n");
        return;
    }
    aplicar_config_bench();
}
