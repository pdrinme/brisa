#include "rasterizador.h"

#include <math.h>
#include <stdio.h>

void Rasterizador::misturar(int x, int y, f32 r, f32 g, f32 b, f32 a) {
    if (x < 0 || y < 0 || x >= L || y >= A) return;
    u32& d = px_[y * L + x];
    f32 dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
    // Igual ao GS com alfa (Cs - Cd) * As + Cd, e As quantizado em 0..128.
    f32 q = static_cast<f32>((static_cast<int>(a + 0.5f) + 1) >> 1) / 128.0f;
    if (q > 1) q = 1;
    int nr = static_cast<int>(dr + (r - dr) * q + 0.5f);
    int ng = static_cast<int>(dg + (g - dg) * q + 0.5f);
    int nb = static_cast<int>(db + (b - db) * q + 0.5f);
    d = (static_cast<u32>(nr) << 16) | (static_cast<u32>(ng) << 8) | static_cast<u32>(nb);
}

static f32 aresta(f32 ax, f32 ay, f32 bx, f32 by, f32 px, f32 py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void Rasterizador::triangulo(const f32* xs, const f32* ys, const Cor* cs) {
    f32 area = aresta(xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
    if (fabsf(area) < 1e-6f) return;
    int x0 = static_cast<int>(floorf(fminf(xs[0], fminf(xs[1], xs[2]))));
    int x1 = static_cast<int>(ceilf(fmaxf(xs[0], fmaxf(xs[1], xs[2]))));
    int y0 = static_cast<int>(floorf(fminf(ys[0], fminf(ys[1], ys[2]))));
    int y1 = static_cast<int>(ceilf(fmaxf(ys[0], fmaxf(ys[1], ys[2]))));
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > L) x1 = L;
    if (y1 > A) y1 = A;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            f32 px = x + 0.5f, py = y + 0.5f;
            f32 w0 = aresta(xs[1], ys[1], xs[2], ys[2], px, py) / area;
            f32 w1 = aresta(xs[2], ys[2], xs[0], ys[0], px, py) / area;
            f32 w2 = 1.0f - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            misturar(x, y, w0 * cs[0].r + w1 * cs[1].r + w2 * cs[2].r, w0 * cs[0].g + w1 * cs[1].g + w2 * cs[2].g,
                     w0 * cs[0].b + w1 * cs[1].b + w2 * cs[2].b, w0 * cs[0].a + w1 * cs[1].a + w2 * cs[2].a);
        }
}

void Rasterizador::iniciar_3d(Cor f, Cor neblina, f32 perto) {
    u32 fundo = (static_cast<u32>(f.r) << 16) | (static_cast<u32>(f.g) << 8) | f.b;
    for (u32& p : px_) p = fundo;
    for (u32& z : z_) z = 0;
    neblina_ = neblina;
    perto_ = perto;
}

void Rasterizador::triangulo(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c) {
    const VerticeTela* v[3] = {&a, &b, &c};
    f32 xs[3], ys[3];
    for (int i = 0; i < 3; i++) { xs[i] = v[i]->x; ys[i] = v[i]->y; }
    f32 area = aresta(xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
    if (fabsf(area) < 1e-6f) return;
    int x0 = static_cast<int>(floorf(fminf(xs[0], fminf(xs[1], xs[2]))));
    int x1 = static_cast<int>(ceilf(fmaxf(xs[0], fmaxf(xs[1], xs[2]))));
    int y0 = static_cast<int>(floorf(fminf(ys[0], fminf(ys[1], ys[2]))));
    int y1 = static_cast<int>(ceilf(fmaxf(ys[0], fmaxf(ys[1], ys[2]))));
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > L) x1 = L;
    if (y1 > A) y1 = A;
    f32 zs[3];
    for (int i = 0; i < 3; i++) zs[i] = static_cast<f32>(gs3d::z_de(v[i]->q, perto_));
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            f32 px = x + 0.5f, py = y + 0.5f;
            f32 w0 = aresta(xs[1], ys[1], xs[2], ys[2], px, py) / area;
            f32 w1 = aresta(xs[2], ys[2], xs[0], ys[0], px, py) / area;
            f32 w2 = 1.0f - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            u32 zp = static_cast<u32>(w0 * zs[0] + w1 * zs[1] + w2 * zs[2]);
            u32& zd = z_[y * L + x];
            if (zp < zd) continue;
            zd = zp;
            f32 f = (w0 * v[0]->neblina + w1 * v[1]->neblina + w2 * v[2]->neblina) / 255.0f;
            f32 r = w0 * v[0]->cor.r + w1 * v[1]->cor.r + w2 * v[2]->cor.r;
            f32 g = w0 * v[0]->cor.g + w1 * v[1]->cor.g + w2 * v[2]->cor.g;
            f32 bl = w0 * v[0]->cor.b + w1 * v[1]->cor.b + w2 * v[2]->cor.b;
            int nr = static_cast<int>(r * f + neblina_.r * (1 - f) + 0.5f);
            int ng = static_cast<int>(g * f + neblina_.g * (1 - f) + 0.5f);
            int nb = static_cast<int>(bl * f + neblina_.b * (1 - f) + 0.5f);
            px_[y * L + x] = (static_cast<u32>(nr) << 16) | (static_cast<u32>(ng) << 8) | static_cast<u32>(nb);
        }
}

// Textura com modulação do GS (TFX = MODULATE): C = Ct·Cv/128, A = At·Av/128.
static void amostrar(const TexturaBrisa& t, f32 s, f32 u, u32 mod, f32* rgba) {
    int x = static_cast<int>(s * t.largura), y = static_cast<int>(u * t.altura);
    x = x < 0 ? 0 : (x >= t.largura ? t.largura - 1 : x);
    y = y < 0 ? 0 : (y >= t.altura ? t.altura - 1 : y);
    u32 c = t.rgba(x, y);
    f32 m[4] = {static_cast<f32>(mod & 255), static_cast<f32>((mod >> 8) & 255), static_cast<f32>((mod >> 16) & 255),
                static_cast<f32>(mod >> 24)};
    for (int k = 0; k < 4; k++) rgba[k] = static_cast<f32>((c >> (8 * k)) & 255) * m[k] / 128.0f;
    // alfa da textura já está em 0..128; leva para 0..255 (a mistura usa 0..255)
    rgba[3] = rgba[3] * 255.0f / 128.0f;
}

void Rasterizador::triangulo_textura(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c, int tex) {
    if (tex < 0 || tex >= n_texturas) return;
    const TexturaBrisa& t = texturas[tex];
    const VerticeTela* v[3] = {&a, &b, &c};
    f32 xs[3] = {a.x, b.x, c.x}, ys[3] = {a.y, b.y, c.y};
    f32 area = aresta(xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
    if (fabsf(area) < 1e-6f) return;
    int x0 = static_cast<int>(floorf(fminf(xs[0], fminf(xs[1], xs[2])))), x1 = static_cast<int>(ceilf(fmaxf(xs[0], fmaxf(xs[1], xs[2]))));
    int y0 = static_cast<int>(floorf(fminf(ys[0], fminf(ys[1], ys[2])))), y1 = static_cast<int>(ceilf(fmaxf(ys[0], fmaxf(ys[1], ys[2]))));
    x0 = x0 < 0 ? 0 : x0; y0 = y0 < 0 ? 0 : y0; x1 = x1 > L ? L : x1; y1 = y1 > A ? A : y1;
    f32 zs[3];
    for (int i = 0; i < 3; i++) zs[i] = static_cast<f32>(gs3d::z_de(v[i]->q, perto_));
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            f32 px = x + 0.5f, py = y + 0.5f;
            f32 w0 = aresta(xs[1], ys[1], xs[2], ys[2], px, py) / area;
            f32 w1 = aresta(xs[2], ys[2], xs[0], ys[0], px, py) / area;
            f32 w2 = 1.0f - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            u32 zp = static_cast<u32>(w0 * zs[0] + w1 * zs[1] + w2 * zs[2]);
            if (zp < z_[y * L + x]) continue;
            // perspectiva correta: interpola s·q, t·q e q (STQ do GS)
            f32 q = w0 * a.q + w1 * b.q + w2 * c.q;
            f32 s = (w0 * a.s * a.q + w1 * b.s * b.q + w2 * c.s * c.q) / q;
            f32 u = (w0 * a.t * a.q + w1 * b.t * b.q + w2 * c.t * c.q) / q;
            f32 rgba[4];
            amostrar(t, s, u, 0x80808080u, rgba);
            misturar(x, y, rgba[0], rgba[1], rgba[2], rgba[3]);
            z_[y * L + x] = zp;
        }
}

void Rasterizador::desenhar(const ListaDesenho2D& l, bool limpar) {
    Cor f = l.fundo();
    u32 fundo = (static_cast<u32>(f.r) << 16) | (static_cast<u32>(f.g) << 8) | f.b;
    if (limpar)
        for (u32& p : px_) p = fundo;
    for (int i = 0; i < l.tamanho(); i++) {
        const Prim& p = l[i];
        if (p.tipo == TipoPrim::SPRITE_TEX) {
            if (p.textura < 0 || p.textura >= n_texturas) continue;
            const TexturaBrisa& t = texturas[p.textura];
            int xa = static_cast<int>(p.x[0] + 0.5f), xb = static_cast<int>(p.x[1] + 0.5f);
            int ya = static_cast<int>(p.y[0] + 0.5f), yb = static_cast<int>(p.y[1] + 0.5f);
            u32 mod = p.cor[0].r | (p.cor[0].g << 8) | (p.cor[0].b << 16) | (static_cast<u32>(p.cor[0].a) << 24);
            for (int y = ya; y < yb; y++)
                for (int x = xa; x < xb; x++) {
                    f32 rgba[4];
                    f32 su = (x - p.x[0] + 0.5f) / (p.x[1] - p.x[0]), sv = (y - p.y[0] + 0.5f) / (p.y[1] - p.y[0]);
                    amostrar(t, p.x[2] + (p.x[3] - p.x[2]) * su, p.y[2] + (p.y[3] - p.y[2]) * sv, mod, rgba);
                    misturar(x, y, rgba[0], rgba[1], rgba[2], rgba[3]);
                }
        } else if (p.tipo == TipoPrim::RETANGULO) {
            int xa = static_cast<int>(p.x[0] + 0.5f), xb = static_cast<int>(p.x[1] + 0.5f);
            int ya = static_cast<int>(p.y[0] + 0.5f), yb = static_cast<int>(p.y[1] + 0.5f);
            for (int y = ya; y < yb; y++)
                for (int x = xa; x < xb; x++) misturar(x, y, p.cor[0].r, p.cor[0].g, p.cor[0].b, p.cor[0].a);
        } else {
            f32 xs[4], ys[4];
            Cor cs[4];
            if (p.tipo == TipoPrim::GRADIENTE) {
                f32 gx[4] = {p.x[0], p.x[1], p.x[0], p.x[1]}, gy[4] = {p.y[0], p.y[0], p.y[1], p.y[1]};
                for (int k = 0; k < 4; k++) { xs[k] = gx[k]; ys[k] = gy[k]; cs[k] = p.cor[k]; }
            } else {
                for (int k = 0; k < 4; k++) { xs[k] = p.x[k]; ys[k] = p.y[k]; cs[k] = p.cor[0]; }
            }
            triangulo(xs, ys, cs);
            f32 xs2[3] = {xs[1], xs[2], xs[3]}, ys2[3] = {ys[1], ys[2], ys[3]};
            Cor cs2[3] = {cs[1], cs[2], cs[3]};
            triangulo(xs2, ys2, cs2);
        }
    }
}

// PNG sem compressão (blocos "stored" do deflate): só a biblioteca padrão.
namespace {
u32 crc_tab[256];
void crc_init() {
    for (u32 n = 0; n < 256; n++) {
        u32 c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_tab[n] = c;
    }
}
u32 crc(u32 c, const u8* b, size_t n) {
    c ^= 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = crc_tab[(c ^ b[i]) & 255] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
void be32(u8* p, u32 v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
void bloco(FILE* f, const char* tipo, const u8* dados, u32 n) {
    u8 h[8];
    be32(h, n);
    for (int i = 0; i < 4; i++) h[4 + i] = static_cast<u8>(tipo[i]);
    fwrite(h, 1, 8, f);
    if (n) fwrite(dados, 1, n, f);
    u32 c = crc(0, h + 4, 4);
    if (n) {
        c ^= 0xFFFFFFFFu;  // continua o CRC
        for (u32 i = 0; i < n; i++) c = crc_tab[(c ^ dados[i]) & 255] ^ (c >> 8);
        c ^= 0xFFFFFFFFu;
    }
    u8 t[4];
    be32(t, c);
    fwrite(t, 1, 4, f);
}
}  // namespace

bool Rasterizador::salvar_png(const char* caminho) const {
    crc_init();
    FILE* f = fopen(caminho, "wb");
    if (!f) return false;
    static const u8 assinatura[8] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
    fwrite(assinatura, 1, 8, f);
    u8 ihdr[13];
    be32(ihdr, L);
    be32(ihdr + 4, A);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    bloco(f, "IHDR", ihdr, 13);
    const u32 linha = 1 + L * 3;
    const u32 bruto = linha * A;
    static u8 cru[(1 + L * 3) * A];
    for (int y = 0; y < A; y++) {
        u8* p = cru + y * linha;
        p[0] = 0;
        for (int x = 0; x < L; x++) {
            u32 v = px_[y * L + x];
            p[1 + x * 3] = v >> 16; p[2 + x * 3] = v >> 8; p[3 + x * 3] = v;
        }
    }
    u32 nblocos = (bruto + 65534) / 65535;
    static u8 z[2 + (1 + L * 3) * A + 5 * 32 + 4];
    u32 k = 0;
    z[k++] = 0x78; z[k++] = 0x01;
    u32 a = 1, b = 0;
    for (u32 i = 0; i < bruto; i++) { a = (a + cru[i]) % 65521; b = (b + a) % 65521; }
    for (u32 i = 0; i < nblocos; i++) {
        u32 ini = i * 65535, n = bruto - ini < 65535 ? bruto - ini : 65535;
        z[k++] = (i == nblocos - 1) ? 1 : 0;
        z[k++] = n & 255; z[k++] = n >> 8; z[k++] = ~n & 255; z[k++] = (~n >> 8) & 255;
        for (u32 j = 0; j < n; j++) z[k++] = cru[ini + j];
    }
    be32(z + k, (b << 16) | a);
    k += 4;
    bloco(f, "IDAT", z, k);
    bloco(f, "IEND", nullptr, 0);
    fclose(f);
    return true;
}

// Gouraud com alfa e teste Z (sem gravar Z), como o GS com ABE e ZTST = GEQUAL.
void Rasterizador::triangulo_alfa(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c) {
    const VerticeTela* v[3] = {&a, &b, &c};
    f32 xs[3] = {a.x, b.x, c.x}, ys[3] = {a.y, b.y, c.y};
    f32 area = aresta(xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
    if (fabsf(area) < 1e-6f) return;
    int x0 = static_cast<int>(floorf(fminf(xs[0], fminf(xs[1], xs[2])))), x1 = static_cast<int>(ceilf(fmaxf(xs[0], fmaxf(xs[1], xs[2]))));
    int y0 = static_cast<int>(floorf(fminf(ys[0], fminf(ys[1], ys[2])))), y1 = static_cast<int>(ceilf(fmaxf(ys[0], fmaxf(ys[1], ys[2]))));
    x0 = x0 < 0 ? 0 : x0; y0 = y0 < 0 ? 0 : y0; x1 = x1 > L ? L : x1; y1 = y1 > A ? A : y1;
    f32 zs[3];
    for (int i = 0; i < 3; i++) zs[i] = static_cast<f32>(gs3d::z_de(v[i]->q, perto_));
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            f32 px = x + 0.5f, py = y + 0.5f;
            f32 w0 = aresta(xs[1], ys[1], xs[2], ys[2], px, py) / area;
            f32 w1 = aresta(xs[2], ys[2], xs[0], ys[0], px, py) / area;
            f32 w2 = 1.0f - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            u32 zp = static_cast<u32>(w0 * zs[0] + w1 * zs[1] + w2 * zs[2]);
            if (zp < z_[y * L + x]) continue;
            misturar(x, y, w0 * a.cor.r + w1 * b.cor.r + w2 * c.cor.r, w0 * a.cor.g + w1 * b.cor.g + w2 * c.cor.g,
                     w0 * a.cor.b + w1 * b.cor.b + w2 * c.cor.b, w0 * a.cor.a + w1 * b.cor.a + w2 * c.cor.a);
        }
}
