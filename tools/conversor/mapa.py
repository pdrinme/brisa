"""Conversor do mapa (Etapa 5): layout da Ilha Ventania → formatos do Brisa.

Entrada: docs/auditoria/ilha_ventania_layout.json (extraído do gerador do
Ventania por tools/auditoria/extrair_ilha.sh, sem alterar o Ventania) e os GLB
das peças de construção do Ventania.
Saída (via `registrar` do construir.py):
  mapa/ilha.bmap               relevo, cores do chão, costa, modelos, setores, instâncias, POIs, spawns
  modelos/parede_<mat>_m<mask>.bmod   paredes com porta/janela (ladrilhos recortados, como grade.gd)
  modelos/<id>_lod1.bmod       LOD1 (QEM) das peças, árvores e props grandes usados no mapa
  modelos/mapa_estrada_<s>.bmod  estradas, calçadas e faixas pré-malhadas por setor (estrada.gdshader)
  texturas/minimapa.btex       vista de cima 256×256 (PSMT8)
Formato: docs/FORMATOS.md (BMAP).
"""
import copy
import math
import os
import struct

import decimar
import formatos
import modelo
import navegacao
import textura

N = 201
PASSO = 3.0
ORIGEM = -300.0
SETOR = 30.0
S = 20  # setores por lado (20 × 30 m = 600 m)
VERSAO_MAPA = 2  # v2: pontos de loot
MATERIAIS = ["madeira", "pedra", "metal"]
TIPOS = {0: "parede", 1: "parede", 2: "piso", 3: "rampa", 4: "teto"}

# Cores do terreno.gdshader / estrada.gdshader do Ventania (source_color = sRGB).
GRAMA_A, GRAMA_B = (0.42, 0.76, 0.29), (0.33, 0.64, 0.25)
AREIA, ROCHA, TERRA = (0.91, 0.83, 0.60), (0.56, 0.54, 0.50), (0.62, 0.47, 0.30)
ASFALTO = (0.30, 0.31, 0.34)
BRANCO, AMARELO = (0.94, 0.94, 0.94), (1.0, 0.82, 0.25)
CONCRETO, MEIO_FIO = (0.80, 0.78, 0.73), (0.55, 0.55, 0.56)
MAR = (0x3f / 255, 0xb6 / 255, 0xd9 / 255)

# Categorias (distância de desenho, LOD) — BRISA_ARCHITECTURE §18.
CAT_PECA, CAT_ARVORE, CAT_PEQUENO, CAT_MOVEL, CAT_GRANDE, CAT_BAU, CAT_ESTRADA = range(7)
DIST_MAX = {  # distância máxima de desenho (m)
    CAT_PECA: 230.0, CAT_ARVORE: 210.0, CAT_PEQUENO: 85.0, CAT_MOVEL: 24.0,
    CAT_GRANDE: 190.0, CAT_BAU: 90.0, CAT_ESTRADA: 230.0,
}
# Cadeia de LODs por categoria: (a partir de quantos metros, fração dos triângulos do LOD0).
# Medido nas vistas da Etapa 5: as peças de construção dominam (Vila Brisa).
LODS = {
    CAT_PECA: [(45.0, 0.30), (110.0, 0.12)],
    CAT_ARVORE: [(70.0, 0.30)],
    CAT_GRANDE: [(70.0, 0.30)],
    CAT_BAU: [(40.0, 0.30)],
}
ARVORES = {"pinheiro_01", "arvore_redonda_01", "tronco_01"}
GRANDES = {"conteiner_01", "conteiner_02", "conteiner_03", "silo_01", "tenda_01", "maquina_01", "poste_01",
           "escada_madeira_01", "escada_madeira_topo_01", "escada_concreto_01", "escada_concreto_topo_01",
           "escada_metal_01", "escada_metal_topo_01", "cerca_01", "feno_01", "tambores_01"}
MOVEIS = {"prateleira_01", "tapete_01", "luminaria_01", "cama_01", "mesa_01", "cadeira_01", "armario_01",
          "geladeira_01", "sofa_01", "fogao_01", "pia_cozinha_01", "tv_01", "expositor_01", "armario_metal_01",
          "balcao_caixa_01", "gondola_01", "bancada_01", "carrinho_01", "lixeira_01", "ferramentas_01", "banco_01"}
LOD_MIN_TRIS = 80


def categoria(mid):
    if mid.startswith(("parede_", "piso_", "rampa_", "teto_")):
        return CAT_PECA
    if mid in ARVORES:
        return CAT_ARVORE
    if mid in GRANDES:
        return CAT_GRANDE
    if mid in MOVEIS:
        return CAT_MOVEL
    if mid.startswith("bau_"):
        return CAT_BAU
    return CAT_PEQUENO


# ---------------- relevo e cores (portas das contas do Ventania) ----------------

class Relevo:
    def __init__(self, alturas):
        self.h = alturas

    def amostra(self, i, j):
        i = min(max(i, 0), N - 1)
        j = min(max(j, 0), N - 1)
        return self.h[j * N + i]

    def altura(self, x, z):
        """ground_height do Ventania (bilinear)."""
        fx, fz = (x - ORIGEM) / PASSO, (z - ORIGEM) / PASSO
        i, j = math.floor(fx), math.floor(fz)
        tx, tz = fx - i, fz - j
        a = self.amostra(i, j) + (self.amostra(i + 1, j) - self.amostra(i, j)) * tx
        b = self.amostra(i, j + 1) + (self.amostra(i + 1, j + 1) - self.amostra(i, j + 1)) * tx
        return a + (b - a) * tz


def _h21(x, y):
    v = math.sin(x * 127.1 + y * 311.7) * 43758.5453
    return v - math.floor(v)


def _ruido(x, y):
    i, j = math.floor(x), math.floor(y)
    fx, fy = x - i, y - j
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    a = _h21(i, j) + (_h21(i + 1, j) - _h21(i, j)) * fx
    b = _h21(i, j + 1) + (_h21(i + 1, j + 1) - _h21(i, j + 1)) * fx
    return a + (b - a) * fy


def _smooth(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def cor_do_chao(x, z, pesos):
    """terreno.gdshader avaliado no vértice. Os ruídos finos (0,21 e 1,7 por metro)
    não cabem numa grade de 3 m e entram pelo valor médio; o ruído grande (manchas
    de grama de ~30 m) entra inteiro."""
    g = [ga + (gb - ga) * _smooth(0.35, 0.75, _ruido(x * 0.035, z * 0.035)) for ga, gb in zip(GRAMA_A, GRAMA_B)]
    g = [c * 0.99 for c in g]
    a = [c * 0.95 for c in AREIA]
    r = [c * 0.945 for c in ROCHA]
    t = [c * 0.96 for c in TERRA]
    wa, wr, wt = pesos
    soma = wa + wr + wt
    if soma > 1:
        wa, wr, wt, soma = wa / soma, wr / soma, wt / soma, 1.0
    return tuple(min(255, int(round(255 * (g[k] * (1 - soma) + a[k] * wa + r[k] * wr + t[k] * wt)))) for k in range(3))


def _rgb(c):
    return tuple(min(255, int(round(255 * v))) for v in c)


# ---------------- peças da grade (gameplay/build/grade.gd) ----------------

def transformacao_peca(t, c, rot):
    """Grade.transform: (posição, yaw em graus) do asset do kit para o slot."""
    x, y, z = c[0] * 4.0, c[1] * 4.0, c[2] * 4.0
    if t == 0:
        return (x + 2.0, y, z), 180.0 if rot == 2 else 0.0
    if t == 1:
        return (x, y, z + 2.0), 90.0 if rot == 1 else -90.0
    return (x + 2.0, y, z + 2.0), (rot % 4) * 90.0


def _cortar(poly, eixo, valor, maior):
    out = []
    n = len(poly)
    for k in range(n):
        a, b = poly[k], poly[(k + 1) % n]
        da = (a[0][eixo] - valor) * (1 if maior else -1)
        db = (b[0][eixo] - valor) * (1 if maior else -1)
        if da >= 0:
            out.append(a)
        if (da >= 0) != (db >= 0):
            f = da / (da - db)
            p = tuple(a[0][q] + (b[0][q] - a[0][q]) * f for q in range(3))
            out.append((p, a[1]))
    return out


def _caixa_tris(centro, tam, cor):
    """Caixa (12 triângulos, normais para fora, anti-horário visto de fora)."""
    cx, cy, cz = centro
    hx, hy, hz = tam[0] / 2, tam[1] / 2, tam[2] / 2
    faces = [((1, 0, 0), [(1, -1, -1), (1, 1, -1), (1, 1, 1), (1, -1, 1)]),
             ((-1, 0, 0), [(-1, -1, 1), (-1, 1, 1), (-1, 1, -1), (-1, -1, -1)]),
             ((0, 1, 0), [(-1, 1, -1), (-1, 1, 1), (1, 1, 1), (1, 1, -1)]),
             ((0, -1, 0), [(-1, -1, 1), (-1, -1, -1), (1, -1, -1), (1, -1, 1)]),
             ((0, 0, 1), [(-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]),
             ((0, 0, -1), [(1, -1, -1), (-1, -1, -1), (-1, 1, -1), (1, 1, -1)])]
    tris = []
    for n, q in faces:
        pts = [(cx + a * hx, cy + b * hy, cz + c * hz) for a, b, c in q]
        for tri in ((pts[0], pts[1], pts[2]), (pts[0], pts[2], pts[3])):
            tris.append(_orientar(tri, n, cor))
    return tris


def _orientar(tri, n, cor):
    """Garante o sentido anti-horário visto do lado da normal n."""
    a, b, c = tri
    u = [b[k] - a[k] for k in range(3)]
    v = [c[k] - a[k] for k in range(3)]
    x = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    if x[0] * n[0] + x[1] * n[1] + x[2] * n[2] < 0:
        a, b, c = a, c, b
    return ((a, b, c), (n, n, n), cor)


def parede_com_aberturas(base, mask, paleta, material):
    """grade.gd tile_meshes + frame_boxes: recorta a parede em 3×3 ladrilhos, mantém
    os presentes e fecha as bordas das aberturas com a moldura (cor escura da paleta)."""
    m = copy.deepcopy(base)
    s = 4.0 / 3.0
    tris = [t for p in base.partes for t in p["tris"]]
    novos = []
    for k in range(9):
        if not mask & (1 << k):
            continue
        i, j = k % 3, k // 3
        x0, a0 = -2.0 + i * s, j * s
        lim = [-1e6 if i == 0 else x0, 1e6 if i == 2 else x0 + s, -1e6 if j == 0 else a0, 1e6 if j == 2 else a0 + s]
        for ps, ns, cor in tris:
            poly = [(ps[q], ns[q]) for q in range(3)]
            for eixo, valor, maior in ((0, lim[0], True), (0, lim[1], False), (1, lim[2], True), (1, lim[3], False)):
                if len(poly) >= 3:
                    poly = _cortar(poly, eixo, valor, maior)
            for q in range(1, len(poly) - 1):
                tri = (poly[0], poly[q], poly[q + 1])
                novos.append((tuple(v[0] for v in tri), tuple(v[1] for v in tri), cor))
    celula = [2, 5, 7][material]
    cor_m, _ = modelo.cor_da_uv(paleta, (celula % 8 + 0.5) / 8.0, (celula // 8 + 0.5) / 8.0)
    caixas = []
    for j in range(3):
        for i in range(3):
            if not mask & (1 << (j * 3 + i)):
                continue
            cx, cy = -2.0 + (i + 0.5) * s, (j + 0.5) * s
            caixas.append(((cx, cy, 0.0), (s, s, 0.24)))
            for di, dj in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                ni, nj = i + di, j + dj
                if ni < 0 or ni > 2 or nj < 0 or nj > 2 or mask & (1 << (nj * 3 + ni)):
                    continue
                if di:
                    novos += _caixa_tris((cx + di * (s * 0.5 - 0.05), cy, 0.0), (0.1, s, 0.32), cor_m)
                else:
                    novos += _caixa_tris((cx, cy + dj * (s * 0.5 - 0.05), 0.0), (s, 0.1, 0.32), cor_m)
    # O recorte fragmenta os triângulos (3–4× a parede inteira); a QEM, que preserva
    # as bordas (e portanto o contorno das aberturas), volta para ~1,7× a parede.
    alvo = int(sum(len(p["tris"]) for p in base.partes) * 1.7)
    if len(novos) > alvo:
        novos = decimar.reduzir(novos, alvo)
    m.partes = [{"no": -1, "tris": novos}]
    # Colisão: uma caixa por ladrilho presente (grade.gd tile_boxes).
    m.colisoes = [((c[0] - t[0] / 2, c[1] - t[1] / 2, c[2] - t[2] / 2), (c[0] + t[0] / 2, c[1] + t[1] / 2, c[2] + t[2] / 2), 0, 0.0)
                  for c, t in caixas]
    return m


def reduzido(m, fracao):
    """LOD por QEM (BR-ADR-11): `fracao` dos triângulos de cada parte; colisão e nós iguais."""
    k = copy.deepcopy(m)
    for p in k.partes:
        n = len(p["tris"])
        p["tris"] = decimar.reduzir(p["tris"], max(2, int(round(n * fracao))))
    return k


def categoria_de(nome):
    import re
    base = re.sub(r"_lod\d$", "", nome)
    return CAT_ESTRADA if base.startswith("mapa_") else categoria(base)


# ---------------- estradas (ilha_ventania.gd _malhas_estradas + estrada.gdshader) ----------------

def _faixa(relevo, pts, de, ate, elev, cor, curso=None, meio_fio=False):
    """Fita ao longo da polilinha, de `de` a `ate` metros do eixo, `elev` acima do
    chão. `curso(ao_longo) -> bool` recorta tracejados. Devolve triângulos (mundo)."""
    tris = []
    ao_longo = 0.0
    ant = None
    for k in range(len(pts)):
        p = pts[k]
        q0, q1 = pts[max(k - 1, 0)], pts[min(k + 1, len(pts) - 1)]
        dx, dz = q1[0] - q0[0], q1[1] - q0[1]
        L = math.hypot(dx, dz) or 1.0
        lx, lz = -dz / L, dx / L
        a = (p[0] + lx * de, p[1] + lz * de)
        b = (p[0] + lx * ate, p[1] + lz * ate)
        va = (a[0], relevo.altura(*a) + elev, a[1])
        vb = (b[0], relevo.altura(*b) + elev, b[1])
        if k > 0:
            v0 = ao_longo
            ao_longo += math.hypot(p[0] - pts[k - 1][0], p[1] - pts[k - 1][1])
            if curso is None or curso((v0 + ao_longo) / 2):
                aa, ab = ant
                for tri in ((aa, vb, ab), (aa, va, vb)):
                    tris.append(_orientar(tri, (0.0, 1.0, 0.0), cor))
                if meio_fio:
                    interno_ant, interno = (aa, va) if abs(de) < abs(ate) else (ab, vb)
                    baixo = lambda v: (v[0], v[1] - elev, v[2])  # noqa: E731
                    # a face do meio-fio olha para o eixo da rua
                    lado = 1.0 if de + ate > 0 else -1.0
                    nx, nz = -lado * lx, -lado * lz
                    for tri in ((interno_ant, baixo(interno), baixo(interno_ant)), (interno_ant, interno, baixo(interno))):
                        tris.append(_orientar(tri, (nx, 0.0, nz), _rgb(MEIO_FIO)))
        ant = (va, vb)
    return tris


def _trechos_urbanos(pts):
    out, atual = [], []
    for p in pts:
        d1 = math.hypot(p[0] + 150, p[1] - 55)
        d2 = math.hypot(p[0] - 185, p[1] - 45)
        if (d1 < 46.0 or d2 < 34.0) and not d1 < 8.0:
            atual.append(p)
        else:
            if len(atual) > 1:
                out.append(atual)
            atual = []
    if len(atual) > 1:
        out.append(atual)
    return out


def malhas_estradas(layout, relevo):
    """Triângulos (mundo) das pistas, marcações, calçadas e faixas de pedestres."""
    tris = []
    k = 0
    for e in layout["estradas"]:
        if e["tipo"] == "terra":
            continue  # caminhos de terra são cor do chão (pesos), sem malha
        pts = [tuple(p) for p in e["pontos"]]
        w = e["largura"]
        elev = 0.06 + 0.01 * k
        k += 1
        tris += _faixa(relevo, pts, -w / 2, w / 2, elev, _rgb(ASFALTO))
        mk = elev + 0.03
        if e["tipo"] == "asfalto":
            # bordas brancas (x 0,015..0,04 e 0,96..0,985) e faixa central amarela tracejada (55% de 6 m)
            tris += _faixa(relevo, pts, -w / 2 + 0.015 * w, -w / 2 + 0.04 * w, mk, _rgb(BRANCO))
            tris += _faixa(relevo, pts, w / 2 - 0.04 * w, w / 2 - 0.015 * w, mk, _rgb(BRANCO))
            tris += _faixa(relevo, pts, -0.018 * w, 0.018 * w, mk, _rgb(AMARELO), lambda y: (y / 6.0) % 1.0 < 0.55)
        else:
            # rua de bairro: bordas tracejadas (60% de 4 m)
            tracejado = lambda y: (y / 4.0) % 1.0 >= 0.4  # noqa: E731
            tris += _faixa(relevo, pts, -w / 2 + 0.015 * w, -w / 2 + 0.04 * w, mk, _rgb(BRANCO), tracejado)
            tris += _faixa(relevo, pts, w / 2 - 0.04 * w, w / 2 - 0.015 * w, mk, _rgb(BRANCO), tracejado)
        for lado in (-1.0, 1.0):
            for tr in _trechos_urbanos(pts):
                de, ate = w / 2 * lado, (w / 2 + 2.4) * lado
                tris += _faixa(relevo, tr, min(de, ate), max(de, ate), 0.2, _rgb(CONCRETO), meio_fio=True)
    for c0, c1, d0, d1, larg in layout["faixas"]:
        pts = [(c0 - d0 * 1.5, c1 - d1 * 1.5), (c0 + d0 * 1.5, c1 + d1 * 1.5)]
        # listras brancas: step(0,5, fract(x·7)) ao longo da largura
        for s in range(7):
            a = -larg / 2 + (s + 0.5) / 7 * larg
            b = -larg / 2 + (s + 1.0) / 7 * larg
            tris += _faixa(relevo, pts, a, b, 0.12, _rgb(BRANCO))
    return tris


# ---------------- minimapa ----------------

def minimapa(layout, relevo, cores, tam=256):
    """Vista de cima (norte = −z para cima): chão com sombreado, mar, pistas, construções,
    árvores e contornos das regiões."""
    px = [[(0, 0, 0)] * tam for _ in range(tam)]
    escala = 600.0 / tam
    sol = (-0.369, 0.766, 0.527)
    nivel = layout["nivel_mar"]
    for v in range(tam):
        for u in range(tam):
            x, z = ORIGEM + (u + 0.5) * escala, ORIGEM + (v + 0.5) * escala
            h = relevo.altura(x, z)
            if h < nivel + 0.15:
                prof = min(max((nivel - h) / 12.0, 0.0), 1.0)
                c = tuple(int(255 * m * (1.0 - 0.35 * prof)) for m in MAR)
            else:
                fx, fz = (x - ORIGEM) / PASSO, (z - ORIGEM) / PASSO
                i, j = min(int(fx), N - 2), min(int(fz), N - 2)
                tx, tz = fx - i, fz - j
                c = []
                for q in range(3):
                    a = cores[j * N + i][q] * (1 - tx) + cores[j * N + i + 1][q] * tx
                    b = cores[(j + 1) * N + i][q] * (1 - tx) + cores[(j + 1) * N + i + 1][q] * tx
                    c.append(a * (1 - tz) + b * tz)
                gx = relevo.altura(x + 1.5, z) - relevo.altura(x - 1.5, z)
                gz = relevo.altura(x, z + 1.5) - relevo.altura(x, z - 1.5)
                n = (-gx / 3.0, 1.0, -gz / 3.0)
                ln = math.sqrt(sum(k * k for k in n))
                luz = max(0.0, sum(n[k] / ln * sol[k] for k in range(3)))
                f = 0.72 + 0.36 * luz
                c = tuple(min(255, int(k * f)) for k in c)
            px[v][u] = c

    def pintar(x, z, raio_m, cor):
        cu, cv = (x - ORIGEM) / escala, (z - ORIGEM) / escala
        r = raio_m / escala
        for v in range(max(0, int(cv - r - 1)), min(tam, int(cv + r + 2))):
            for u in range(max(0, int(cu - r - 1)), min(tam, int(cu + r + 2))):
                if (u + 0.5 - cu) ** 2 + (v + 0.5 - cv) ** 2 <= max(r, 0.5) ** 2:
                    px[v][u] = cor

    for e in layout["estradas"]:
        cor = _rgb(ASFALTO) if e["tipo"] != "terra" else _rgb(TERRA)
        pts = e["pontos"]
        for a, b in zip(pts, pts[1:]):
            L = math.hypot(b[0] - a[0], b[1] - a[1])
            for s in range(int(L / 1.0) + 1):
                t = s / max(1, int(L / 1.0))
                pintar(a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, e["largura"] / 2, cor)
    cor_mat = [(176, 122, 74), (160, 158, 150), (118, 140, 160)]
    for p in layout["pecas"]:
        if p["t"] in (2, 4):  # piso e teto: a planta das casas
            x0, z0 = p["c"][0] * 4.0, p["c"][2] * 4.0
            for a in range(4):
                for b in range(4):
                    pintar(x0 + a + 0.5, z0 + b + 0.5, 0.7, cor_mat[p["mat"]])
    for p in layout["pecas"]:
        if p["t"] in (0, 1):
            (x, _, z), _ = transformacao_peca(p["t"], p["c"], p["r"])
            for s in range(5):
                d = -2 + s
                pintar(x + (d if p["t"] == 0 else 0), z + (d if p["t"] == 1 else 0), 0.6,
                       tuple(int(k * 0.55) for k in cor_mat[p["mat"]]))
    for pr in layout["props"]:
        if pr["m"] in ("pinheiro_01", "arvore_redonda_01"):
            pintar(pr["p"][0], pr["p"][2], 1.6 * pr["s"], (46, 110, 52) if pr["m"] == "pinheiro_01" else (70, 140, 60))
    for poi in layout["pois"]:
        cor = tuple(int(poi["cor"][k:k + 2], 16) for k in (0, 2, 4))
        lim = poi["limite"]
        for a, b in zip(lim, lim[1:] + lim[:1]):
            L = math.hypot(b[0] - a[0], b[1] - a[1])
            for s in range(int(L / 1.2) + 1):
                t = s / max(1, int(L / 1.2))
                pintar(a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, 0.6, cor)
    return [(c[0], c[1], c[2], 255) for linha in px for c in linha]


# ---------------- montagem ----------------

def _setor(x, z):
    i = int(math.floor((x - ORIGEM) / SETOR))
    j = int(math.floor((z - ORIGEM) / SETOR))
    return min(max(i, 0), S - 1), min(max(j, 0), S - 1)


def _modelo_de_tris(base, tris):
    m = copy.deepcopy(base)
    m.partes = [{"no": -1, "tris": tris}]
    m.colisoes = []
    m.marcadores = []
    return m


def construir(v, paleta, commit, layout, modelos_convertidos, registrar, relatorio):
    """`modelos_convertidos`: id → Modelo (do construir.py), para os LODs e paredes."""
    origem_mapa = {"origem": f"ventania:game/world/ilha/ilha_ventania.gd@{commit} (layout extraído; tools/auditoria/extrair_ilha.sh)",
                   "autor": "Projeto Ventania", "licenca": "Original", "autorizacao": "titular (ASSET_LICENSES.md §2)",
                   "categoria": "mapa"}
    relevo = Relevo(layout["relevo"]["alturas"])
    assert layout["relevo"]["n"] == N and layout["relevo"]["passo"] == PASSO and layout["relevo"]["origem"] == ORIGEM
    pesos = layout["pesos"]
    cores = []
    for j in range(N):
        for i in range(N):
            k = 3 * (j * N + i)
            cores.append(cor_do_chao(ORIGEM + i * PASSO, ORIGEM + j * PASSO, (pesos[k], pesos[k + 1], pesos[k + 2])))

    # Tabela de modelos do mapa.
    nomes, idx = [], {}

    def id_modelo(nome):
        if nome not in idx:
            idx[nome] = len(nomes)
            nomes.append(nome)
        return idx[nome]

    gerados = {}  # nome → Modelo (paredes com abertura e estradas)
    inst = []     # (modelo, categoria, x, y, z, rot, escala)
    for p in layout["pecas"]:
        mat = MATERIAIS[p["mat"]]
        nome = f"{TIPOS[p['t']]}_{mat}"
        if p["t"] in (0, 1) and p["mask"] != 511:
            base = nome
            nome = f"{base}_m{p['mask']}"
            if nome not in gerados:
                gerados[nome] = parede_com_aberturas(modelos_convertidos[base], p["mask"], paleta, p["mat"])
        pos, yaw = transformacao_peca(p["t"], p["c"], p["r"])
        inst.append((id_modelo(nome), CAT_PECA, *pos, yaw, 1.0))
    for pr in layout["props"]:
        inst.append((id_modelo(pr["m"]), categoria(pr["m"]), *pr["p"], pr["ry"], pr["s"]))
    for b in layout["baus"]:
        inst.append((id_modelo("bau_ventania_01"), CAT_BAU, *b["p"], b["ry"], 1.0))

    # Estradas: triângulos por setor (centroide), modelo local ao centro do setor.
    tris_est = malhas_estradas(layout, relevo)
    por_setor = {}
    for t in tris_est:
        cx = sum(p[0] for p in t[0]) / 3
        cz = sum(p[2] for p in t[0]) / 3
        por_setor.setdefault(_setor(cx, cz), []).append(t)
    base_estatica = modelos_convertidos["piso_madeira"]
    for (si, sj), ts in sorted(por_setor.items()):
        ox, oz = ORIGEM + (si + 0.5) * SETOR, ORIGEM + (sj + 0.5) * SETOR
        loc = [(tuple((p[0] - ox, p[1], p[2] - oz) for p in ps), ns, c) for ps, ns, c in ts]
        nome = f"mapa_estrada_{sj:02d}{si:02d}"
        gerados[nome] = _modelo_de_tris(base_estatica, loc)
        inst.append((id_modelo(nome), CAT_ESTRADA, ox, 0.0, oz, 0.0, 1.0))

    # Grava os modelos gerados e os LOD1.
    todos = dict(modelos_convertidos)
    todos.update(gerados)
    for nome, m in sorted(gerados.items()):
        if nome.startswith("mapa_estrada_"):
            o = dict(origem_mapa, origem=origem_mapa["origem"] + " + game/world/ilha/estrada.gdshader (cores e faixas)")
            extra = {"triangulos": m.triangulos(), "notas": "estradas, calçadas e faixas pré-malhadas de um setor de 30 m"}
        else:
            base = nome.rsplit("_m", 1)[0]
            o = dict(origem_mapa, origem=f"ventania:game/assets/construcoes/{base}.glb@{commit} + game/gameplay/build/grade.gd (ladrilhos)")
            extra = {"triangulos": m.triangulos(), "notas": f"parede com aberturas (máscara {nome.rsplit('_m', 1)[1]})"}
        registrar(nome, f"modelos/{nome}.bmod", formatos.escrever_modelo(m), "modelo", o, extra)
    proximo = {}  # nome → (próximo LOD, a partir de quantos metros)
    for nome in list(nomes):
        cat = categoria_de(nome)
        m = todos[nome]
        if cat not in LODS or m.triangulos() < LOD_MIN_TRIS:
            continue
        ant = nome
        for k, (dist, fracao) in enumerate(LODS[cat], start=1):
            mk = reduzido(m, fracao)
            if mk.triangulos() >= todos[ant].triangulos():
                break
            nl = f"{nome}_lod{k}"
            proximo[ant] = (nl, dist)
            todos[nl] = mk
            o = dict(origem_mapa, origem=f"derivado de modelos/{nome}.bmod (QEM, BR-ADR-11)")
            registrar(nl, f"modelos/{nl}.bmod", formatos.escrever_modelo(mk), "modelo", o,
                      {"triangulos": mk.triangulos(), "lod_de": nome, "notas": f"LOD{k} do mapa ({int(fracao * 100)}%, a partir de {dist:.0f} m)"})
            id_modelo(nl)
            ant = nl
    lods = proximo

    cache_y, cache_r = {}, {}

    def faixa_y(nome):
        if nome not in cache_y:
            ys = [v[1] for p in todos[nome].partes for tri in p["tris"] for v in tri[0]] or [0.0]
            cache_y[nome] = (min(ys), max(ys))
        return cache_y[nome]

    # Setores: instâncias ordenadas por setor e, dentro dele, por modelo.
    def chave(t):
        si, sj = _setor(t[2], t[4])
        return (sj * S + si, t[0])
    inst.sort(key=chave)
    setores = [[0, 0, 1e9, -1e9] for _ in range(S * S)]
    for k, t in enumerate(inst):
        si, sj = _setor(t[2], t[4])
        s = setores[sj * S + si]
        if s[1] == 0:
            s[0] = k
        s[1] += 1
        y0, y1 = faixa_y(nomes[t[0]])
        s[2] = min(s[2], t[3] + y0 * t[6])
        s[3] = max(s[3], t[3] + y1 * t[6])
    for sj in range(S):
        for si in range(S):
            s = setores[sj * S + si]
            for j in range(sj * 10, sj * 10 + 11):
                for i in range(si * 10, si * 10 + 11):
                    h = relevo.amostra(i, j)
                    s[2], s[3] = min(s[2], h), max(s[3], h)

    # Raio de cada modelo (esfera em volta da origem, para o corte por distância).
    def raio_origem(m):
        if id(m) not in cache_r:
            cache_r[id(m)] = max((math.sqrt(sum(c * c for c in v)) for p in m.partes for tri in p["tris"] for v in tri[0]), default=0.0)
        return cache_r[id(m)]

    # Arquivo BMAP.
    b_rel = struct.pack(f"<{N * N}h", *[max(-32767, min(32767, int(round(h * 100)))) for h in layout["relevo"]["alturas"]])
    b_cor = b"".join(struct.pack("<I", c[0] | (c[1] << 8) | (c[2] << 16) | (255 << 24)) for c in cores)
    b_costa = struct.pack(f"<{len(layout['costa'])}f", *layout["costa"])
    b_mod = b""
    for nome in nomes:
        cat = categoria_de(nome)
        d_max = DIST_MAX[cat]
        lod, d_lod = (idx[lods[nome][0]], lods[nome][1]) if nome in lods else (-1, 0.0)
        b_mod += formatos._nome(nome, 32) + struct.pack("<iffI", lod, d_lod, d_max, cat)
    b_set = b"".join(struct.pack("<IIff", *s) for s in setores)
    b_inst = b"".join(struct.pack("<HH6fI", t[0], t[1], t[2], t[3], t[4], t[5], t[6], raio_origem(todos[nomes[t[0]]]) * t[6], 0)
                      for t in inst)
    b_poi, pontos = b"", []
    for p in layout["pois"]:
        cor = int(p["cor"], 16)
        b_poi += formatos._nome(p["id"], 24) + formatos._nome(p["nome"], 40) + struct.pack(
            "<4fIII4x", *p["pos"], p["raio"], cor, len(pontos), len(p["limite"]))
        pontos += [tuple(q) for q in p["limite"]]
    b_pts = b"".join(struct.pack("<2f", *q) for q in pontos)
    b_spw = b"".join(struct.pack("<3f4x", *s) for s in layout["spawns"])
    b_loot = b"".join(struct.pack("<3f4x", *p) for p in layout["loot"])
    blocos = [b_rel, b_cor, b_costa, b_mod, b_set, b_inst, b_poi, b_pts, b_spw, b_loot]
    corpo, offs, off = b"", [], 128
    for b in blocos:
        b = formatos._alinhar(b)
        offs.append(off)
        corpo += b
        off += len(b)
    cab = b"BMAP" + struct.pack("<IIfffIfIIIIII", VERSAO_MAPA, N, PASSO, ORIGEM, layout["nivel_mar"], S, SETOR,
                                len(nomes), len(inst), len(layout["pois"]), len(layout["spawns"]), len(pontos),
                                len(layout["costa"]))
    cab += struct.pack("<9I", *offs[:9]) + struct.pack("<II", len(layout["loot"]), offs[9])
    cab = cab + b"\0" * (128 - len(cab))
    dados = cab + corpo
    tris_mapa = sum(todos[nomes[t[0]]].triangulos() for t in inst)
    registrar("mapa_ilha", "mapa/ilha.bmap", dados, "mapa", origem_mapa,
              {"instancias": len(inst), "modelos": len(nomes), "triangulos_instancias": tris_mapa,
               "notas": f"{len(layout['pecas'])} peças, {len(layout['props'])} props, {len(layout['baus'])} baús, "
                        f"{len(por_setor)} setores com estrada; relevo {N}×{N} a {PASSO} m"})
    relatorio.append(("mapa_ilha", "mapa", f"{len(inst)} instâncias de {len(nomes)} modelos, {tris_mapa} triângulos "
                                           f"(sem o relevo), {len(gerados)} modelos gerados, {len(lods)} LODs (LOD1 e LOD2)"))

    # Navegação dos bots (Etapa 7): o NavGrade do Ventania calculado offline.
    dados_nav, info_nav = navegacao.construir(layout, relevo)
    registrar("mapa_navegacao", "mapa/ilha.bnav", dados_nav, "nav",
              dict(origem_mapa, origem=origem_mapa["origem"] + " + game/gameplay/ai/nav_grade.gd (construção do grafo)"),
              dict(info_nav, notas="grafo dos bots: 150×150 células de 4 m, andares e ligações (escadas, rampas)"))
    relatorio.append(("mapa_navegacao", "mapa", f"{info_nav['andares']} andares, {info_nav['ligacoes']} ligações "
                                                f"({info_nav['escadas']} escadas), {info_nav['bloqueadas']} células bloqueadas, "
                                                f"{info_nav['pendentes']} pendentes"))

    # Minimapa.
    px = minimapa(layout, relevo, cores)
    pal, ind = textura.quantizar(px, 256)
    registrar("minimapa", "texturas/minimapa.btex", formatos.escrever_textura(256, 256, 0x13, ind, textura.alfa_gs(pal)),
              "textura", dict(origem_mapa, origem="gerado pelo Brisa a partir do layout da Ilha Ventania (tools/conversor/mapa.py)",
                              autor="Projeto Brisa / Projeto Ventania"),
              {"largura": 256, "altura": 256, "formato": "PSMT8", "notas": "600 m em 256 px (2,34 m/px), norte para cima"})
    relatorio.append(("minimapa", "texturas", f"256×256 PSMT8, {len(pal)} cores"))
    return {"instancias": len(inst), "modelos": len(nomes), "triangulos": tris_mapa}
