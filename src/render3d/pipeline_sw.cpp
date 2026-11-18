#include "pipeline_sw.h"

#include <math.h>

void Camera3D::configurar(Vec3 olho, Vec3 alvo, f32 fovy, f32 aspecto, f32 p, f32 l) {
    perto = p;
    longe = l;
    posicao = olho;
    vista = Mat4::olhar_para(olho, alvo, Vec3(0, 1, 0));
    projecao = Mat4::perspectiva(fovy, aspecto, p, l);
    vp = projecao * vista;
}

namespace {

const f32 GUARDA = 4.0f;  // igual ao VU1 (toon.vsm, qw 10.w = 1/4)

struct VClip {
    f32 x, y, z, w;
    f32 r, g, b;
};

VClip interpolar(const VClip& a, const VClip& b, f32 t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t,
            a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}

VerticeTela projetar(const VClip& v, const ConfigTela& t, const Neblina& n) {
    f32 q = 1.0f / v.w;
    VerticeTela s;
    s.x = (v.x * q * 0.5f + 0.5f) * t.largura;
    s.y = (0.5f - v.y * q * 0.5f) * t.altura;
    s.q = q;
    s.neblina = gs3d::neblina_de(v.w, n);
    s.cor = Cor{static_cast<u8>(v.r), static_cast<u8>(v.g), static_cast<u8>(v.b), 255};
    return s;
}

}  // namespace

u32 pipeline_objeto(const Cena3D& cena, const Objeto3D& obj, const ConfigTela& tela, SaidaTriangulos& saida) {
    const Malha& m = *obj.malha;
    Mat4 mvp = cena.camera.vp * obj.modelo;
    // Luz no espaço do objeto: N·L sem transformar as normais.
    Mat4 inv = inversa_rigida(obj.modelo);
    Vec3 l = normalizar(transformar_direcao(inv, normalizar(cena.luz.direcao)));
    CoeficientesToon k = CoeficientesToon::de(cena.luz);
    const f32 perto = cena.camera.perto;
    u32 desenhados = 0;
    for (u32 t = 0; t < m.n; t += 3) {
        VClip v[3];
        bool todos_atras = true;
        for (int i = 0; i < 3; i++) {
            Vec4 c = transformar(mvp, m.posicao(t + i));
            Cor cor = sombrear_toon(desempacotar_cor(m.cor[t + i]), ponto(desempacotar_normal(m.normal[t + i]), l),
                                    cena.luz, k);
            v[i] = {c.x, c.y, c.z, c.w, static_cast<f32>(cor.r), static_cast<f32>(cor.g), static_cast<f32>(cor.b)};
            if (c.w >= perto) todos_atras = false;
        }
        if (todos_atras) continue;
        // Fora do volume por x/y (todos do mesmo lado): descarta cedo.
        if ((v[0].x > v[0].w && v[1].x > v[1].w && v[2].x > v[2].w) ||
            (v[0].x < -v[0].w && v[1].x < -v[1].w && v[2].x < -v[2].w) ||
            (v[0].y > v[0].w && v[1].y > v[1].w && v[2].y > v[2].w) ||
            (v[0].y < -v[0].w && v[1].y < -v[1].w && v[2].y < -v[2].w))
            continue;
        // Recorte (Sutherland–Hodgman) no plano perto e nos planos da faixa de
        // guarda do GS (|x|, |y| ≤ GUARDA·w): fora dela as coordenadas não cabem
        // no sistema de 0 a 4095 do GS. Até 3 + 5 vértices.
        VClip poli[8], tmp[8];
        int np = 3;
        for (int i = 0; i < 3; i++) poli[i] = v[i];
        for (int plano = 0; plano < 5 && np >= 3; plano++) {
            auto dist = [&](const VClip& p) -> f32 {
                switch (plano) {
                case 0: return p.w - perto;
                case 1: return GUARDA * p.w - p.x;
                case 2: return GUARDA * p.w + p.x;
                case 3: return GUARDA * p.w - p.y;
                default: return GUARDA * p.w + p.y;
                }
            };
            int nt = 0;
            for (int i = 0; i < np; i++) {
                const VClip& a = poli[i];
                const VClip& b = poli[(i + 1) % np];
                f32 da = dist(a), db = dist(b);
                if (da >= 0) tmp[nt++] = a;
                if ((da >= 0) != (db >= 0)) tmp[nt++] = interpolar(a, b, da / (da - db));
            }
            np = nt;
            for (int i = 0; i < np; i++) poli[i] = tmp[i];
        }
        if (np < 3) continue;
        VerticeTela s[8];
        for (int i = 0; i < np; i++) s[i] = projetar(poli[i], tela, cena.neblina);
        // Face traseira: anti-horário no mundo vira horário na tela (y para baixo).
        f32 area = (s[1].x - s[0].x) * (s[2].y - s[0].y) - (s[2].x - s[0].x) * (s[1].y - s[0].y);
        if (area >= 0) continue;
        for (int i = 1; i + 1 < np; i++) {
            saida.triangulo(s[0], s[i], s[i + 1]);
            desenhados++;
        }
    }
    return desenhados;
}
void pipeline_cena(const Cena3D& cena, const ConfigTela& tela, SaidaTriangulos& saida, Estatisticas3D& est) {
    Frustum f = Frustum::de(cena.camera.vp);
    for (int i = 0; i < cena.n; i++) {
        const Objeto3D& o = cena.objetos[i];
        est.objetos++;
        f32 escala = sqrtf(o.modelo(0, 0) * o.modelo(0, 0) + o.modelo(1, 0) * o.modelo(1, 0) + o.modelo(2, 0) * o.modelo(2, 0));
        Vec4 c = transformar(o.modelo, o.malha->centro);
        if (f.esfera(Vec3(c.x, c.y, c.z), o.malha->raio * escala) == 0) continue;
        est.objetos_visiveis++;
        est.triangulos_enviados += o.malha->triangulos();
        est.triangulos_desenhados += pipeline_objeto(cena, o, tela, saida);
    }
}

u32 projetar_sombras(const Cena3D& cena, const ConfigTela& tela, SaidaTriangulos& saida) {
    u32 n = 0;
    for (int i = 0; i < cena.n_sombras; i++) {
        const Sombra3D& sb = cena.sombras[i];
        const f32 cs[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
        VerticeTela v[4];
        bool ok = true;
        for (int k = 0; k < 4 && ok; k++) {
            Vec4 c = transformar(cena.camera.vp, Vec3(sb.centro.x + cs[k][0] * sb.raio, sb.centro.y + 0.03f, sb.centro.z + cs[k][1] * sb.raio));
            if (c.w < cena.camera.perto || fabsf(c.x) > GUARDA * c.w || fabsf(c.y) > GUARDA * c.w) { ok = false; break; }
            VClip vc{c.x, c.y, c.z, c.w, 128, 128, 128};
            v[k] = projetar(vc, tela, cena.neblina);
            v[k].cor.a = 128;
            v[k].s = (cs[k][0] + 1) * 0.5f;
            v[k].t = (cs[k][1] + 1) * 0.5f;
        }
        if (!ok) continue;
        saida.triangulo_textura(v[0], v[1], v[2], cena.textura_sombra);
        saida.triangulo_textura(v[1], v[3], v[2], cena.textura_sombra);
        n += 2;
    }
    return n;
}
