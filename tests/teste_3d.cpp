#include <math.h>

#include "jogo/cena_teste.h"
#include "nucleo/mat.h"
#include "plataforma/host/rasterizador.h"
#include "render3d/malha.h"
#include "render3d/pipeline_sw.h"
#include "teste.h"

namespace {
bool perto(f32 a, f32 b, f32 e = 1e-4f) { return fabsf(a - b) <= e; }

struct Coletor : SaidaTriangulos {
    int n = 0;
    VerticeTela v[64][3];
    void triangulo(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c) override {
        if (n < 64) { v[n][0] = a; v[n][1] = b; v[n][2] = c; }
        n++;
    }
};

alignas(16) u8 g_mem[256 * 1024];

// Normal de cada face aponta para fora do centro da malha.
bool normais_para_fora(const Malha& m) {
    for (u32 t = 0; t < m.n; t += 3) {
        const f32* p = m.pos + t * 4;
        Vec3 c((p[0] + p[4] + p[8]) / 3, (p[1] + p[5] + p[9]) / 3, (p[2] + p[6] + p[10]) / 3);
        Vec3 n = desempacotar_normal(m.normal[t]);
        if (ponto(n, c - m.centro) <= 0) return false;
    }
    return true;
}
}  // namespace

TESTE(mat_perspectiva_mapeia_perto_e_longe) {
    Mat4 p = Mat4::perspectiva(1.0f, 4.0f / 3.0f, 0.5f, 400.0f);
    Vec4 a = transformar(p, Vec3(0, 0, -0.5f)), b = transformar(p, Vec3(0, 0, -400.0f));
    VERIFICA(perto(a.z / a.w, -1.0f, 1e-3f) && perto(b.z / b.w, 1.0f, 1e-3f));
    VERIFICA(perto(a.w, 0.5f) && perto(b.w, 400.0f, 1e-2f));  // w = distância
}

TESTE(mat_olhar_para_e_inversa_rigida) {
    Mat4 v = Mat4::olhar_para(Vec3(10, 5, 10), Vec3(0, 0, 0), Vec3(0, 1, 0));
    Vec4 o = transformar(v, Vec3(0, 0, 0));
    VERIFICA(perto(o.x, 0, 1e-4f) && perto(o.y, 0, 1e-4f) && o.z < 0);  // o alvo fica à frente (−z)
    Mat4 m = Mat4::translacao(Vec3(3, -2, 7)) * Mat4::rotacao_y(0.7f) * Mat4::escala(2.0f);
    Mat4 i = inversa_rigida(m) * m;
    bool id = true;
    for (int k = 0; k < 16; k++) id = id && perto(i.m[k], Mat4::identidade().m[k], 1e-4f);
    VERIFICA(id);
}

TESTE(frustum_classifica_esferas) {
    Camera3D cam;
    cam.configurar(Vec3(0, 0, 10), Vec3(0, 0, 0), 1.0f, 4.0f / 3.0f, 0.5f, 100.0f);
    Frustum f = Frustum::de(cam.vp);
    VERIFICA(f.esfera(Vec3(0, 0, 0), 1.0f) == 1);
    VERIFICA(f.esfera(Vec3(0, 0, 20), 1.0f) == 0);   // atrás da câmera
    VERIFICA(f.esfera(Vec3(500, 0, 0), 1.0f) == 0);  // muito ao lado
    bool cruza = false;
    VERIFICA(f.esfera(Vec3(0, 0, 9.7f), 1.0f, &cruza) == 2 && cruza);
}

TESTE(geradores_com_normais_para_fora) {
    Arena a(g_mem, sizeof g_mem);
    Cor c = rgb(0xff0000);
    {
        ConstrutorMalha m(a, 12);
        m.caixa(Vec3(-1, -1, -1), Vec3(1, 1, 1), c, true);
        Malha k = m.terminar();
        VERIFICA(k.triangulos() == 12 && normais_para_fora(k));
    }
    {
        ConstrutorMalha m(a, 400);
        m.esfera(Vec3(0, 0, 0), 2.0f, 2, c);
        Malha k = m.terminar();
        VERIFICA(k.triangulos() == 320 && normais_para_fora(k));
        VERIFICA(perto(k.raio, 2.0f, 1e-3f));
    }
    {
        ConstrutorMalha m(a, 40);
        m.cilindro(Vec3(0, -1, 0), 1.0f, 2.0f, 8, c);
        Malha k = m.terminar();
        VERIFICA(normais_para_fora(k));
    }
    {
        ConstrutorMalha m(a, 40);
        m.cone(Vec3(0, 0, 0), 1.0f, 1.0f, 8, c);
        m.piramide(Vec3(-1, 0, -1), Vec3(1, 0, 1), 1.0f, c);
        Malha k = m.terminar();
        VERIFICA(k.triangulos() == 20);
    }
    {
        ConstrutorMalha m(a, 2);
        m.caixa(Vec3(0, 0, 0), Vec3(1, 1, 1), c);  // 10 triângulos não cabem em 2
        VERIFICA(m.estourou() && m.triangulos() == 2);
    }
}

TESTE(normal_empacotada_ida_e_volta) {
    Vec3 n = normalizar(Vec3(0.3f, -0.8f, 0.52f));
    Vec3 r = desempacotar_normal(empacotar_normal(n));
    VERIFICA(perto(r.x, n.x, 0.01f) && perto(r.y, n.y, 0.01f) && perto(r.z, n.z, 0.01f));
    VERIFICA((empacotar_normal(n) >> 24) == 0);  // 4º byte zero (UNPACK V4-8)
}

TESTE(toon_duas_faixas) {
    LuzToon l;
    CoeficientesToon k = CoeficientesToon::de(l);
    Cor base = rgb(0xc8c8c8);
    Cor sol = sombrear_toon(base, 1.0f, l, k), somb = sombrear_toon(base, -1.0f, l, k);
    VERIFICA(sol.r == static_cast<u8>(200 * l.luz.x));
    VERIFICA(somb.b == static_cast<u8>(200 * l.sombra.z));
    VERIFICA(somb.b > somb.r);  // sombra puxada para o azul
    Cor meio = sombrear_toon(base, l.limiar, l, k);  // no limiar: metade do caminho
    VERIFICA(meio.r > somb.r && meio.r < sol.r);
}

TESTE(pipeline_descarta_faces_traseiras_e_recorta_no_perto) {
    Arena a(g_mem, sizeof g_mem);
    ConstrutorMalha m(a, 12);
    m.caixa(Vec3(-1, -1, -1), Vec3(1, 1, 1), rgb(0xffffff), true);
    Malha cubo = m.terminar();
    Cena3D* cena = new Cena3D();
    cena->camera.configurar(Vec3(3, 2.5f, 4), Vec3(0, 0, 0), 1.0f, 4.0f / 3.0f, 0.5f, 100.0f);
    cena->adicionar(&cubo, Mat4::identidade());
    Coletor col;
    Estatisticas3D est;
    pipeline_cena(*cena, ConfigTela(), col, est);
    VERIFICA(est.triangulos_enviados == 12);
    VERIFICA(col.n == 6);  // 3 faces visíveis de um canto
    // Câmera dentro do cubo: vê as faces de dentro? Não (todas viradas para fora).
    cena->camera.configurar(Vec3(0, 0, 0.2f), Vec3(0, 0, -5), 1.0f, 4.0f / 3.0f, 0.5f, 100.0f);
    Coletor col2;
    Estatisticas3D est2;
    pipeline_cena(*cena, ConfigTela(), col2, est2);
    VERIFICA(col2.n == 0);
    // Triângulo atravessando o plano perto: os vértices novos ficam em w ≥ perto.
    ConstrutorMalha m2(a, 1);
    m2.triangulo(Vec3(-1, 0, 2), Vec3(1, 0, 2), Vec3(0, 0, -6), rgb(0xffffff));
    Malha tri = m2.terminar();
    cena->limpar();
    cena->camera.configurar(Vec3(0, 1, 0), Vec3(0, 1, -10), 1.0f, 4.0f / 3.0f, 0.5f, 100.0f);
    cena->adicionar(&tri, Mat4::identidade());
    Coletor col3;
    Estatisticas3D est3;
    pipeline_cena(*cena, ConfigTela(), col3, est3);
    VERIFICA(col3.n >= 1);
    for (int t = 0; t < col3.n && t < 64; t++)
        for (int i = 0; i < 3; i++) VERIFICA(col3.v[t][i].q <= 1.0f / 0.5f + 1e-3f);
    delete cena;
}

TESTE(rasterizador_z_maior_ou_igual_ganha) {
    static Rasterizador r;
    r.iniciar_3d(rgb(0), rgb(0x808080), 0.5f);
    auto vt = [](f32 x, f32 y, f32 q, u32 cor) {
        VerticeTela v;
        v.x = x; v.y = y; v.q = q; v.neblina = 255; v.cor = rgb(cor);
        return v;
    };
    // Perto (q grande) desenhado primeiro, longe depois: o perto continua.
    r.triangulo(vt(100, 100, 1.0f, 0xff0000), vt(300, 100, 1.0f, 0xff0000), vt(100, 300, 1.0f, 0xff0000));
    r.triangulo(vt(100, 100, 0.1f, 0x00ff00), vt(300, 100, 0.1f, 0x00ff00), vt(100, 300, 0.1f, 0x00ff00));
    VERIFICA(r.pixel(150, 150) == 0xff0000);
    // Neblina total: só a cor da neblina.
    r.triangulo(vt(400, 100, 2.0f, 0x0000ff), vt(600, 100, 2.0f, 0x0000ff), vt(400, 300, 2.0f, 0x0000ff));
    VERIFICA(r.pixel(450, 150) == 0x0000ff);
    VerticeTela a = vt(400, 320, 2.1f, 0x0000ff), b = vt(600, 320, 2.1f, 0x0000ff), c = vt(400, 440, 2.1f, 0x0000ff);
    a.neblina = b.neblina = c.neblina = 0;
    r.triangulo(a, b, c);
    VERIFICA(r.pixel(450, 340) == 0x808080);
}

TESTE(cena_teste_niveis_e_benchmark) {
    static u8 mem[2 * 1024 * 1024] __attribute__((aligned(64)));
    Arena a(mem, sizeof mem);
    CenaTeste* c = new CenaTeste();
    VERIFICA(c->iniciar(a));
    c->definir_nivel(0, false);
    VERIFICA(c->instancias() == 24);
    c->definir_nivel(99, false);
    VERIFICA(c->instancias() == (24u << (CenaTeste::NIVEIS - 1)));
    c->definir_nivel(4, true);
    VERIFICA(c->instancias() == 48);
    // O benchmark percorre as configurações do caminho disponível no PC (EE) e termina.
    c->iniciar_bench();
    Estatisticas3D e;
    e.triangulos_enviados = 1000;
    int quadros = 0;
    while (c->bench_ativo() && quadros < 100000) {
        c->medir(33367, 5000, e);
        quadros++;
    }
    VERIFICA(!c->bench_ativo());
    VERIFICA(c->n_resultados() == CenaTeste::NIVEIS + CenaTeste::NIVEIS_CARGA);
    VERIFICA(c->resultado(0).quadro_us == 33367 && c->resultado(0).cpu_us == 5000);
    static Cena3D cena;
    static ListaDesenho2D hud;
    EstatisticasQuadro est;
    InfoPlataforma info;
    c->montar(cena, hud, est, e, info);
    VERIFICA(hud.descartadas() == 0 && cena.n > 1);
    delete c;
}
