"""PNG → textura CLUT do GS (stdlib): leitor de PNG (8 bits, RGB/RGBA/paleta),
quantização por corte mediano e conversão do alfa para a escala do GS (0..128)."""
import struct
import zlib


def ler_png(caminho):
    d = open(caminho, "rb").read()
    assert d[:8] == b"\x89PNG\r\n\x1a\n", "não é PNG"
    off = 8
    idat = b""
    plte = trns = None
    while off < len(d):
        n, tipo = struct.unpack(">I4s", d[off:off + 8])
        corpo = d[off + 8:off + 8 + n]
        if tipo == b"IHDR":
            larg, alt, prof, ctipo, _, _, entrel = struct.unpack(">IIBBBBB", corpo)
        elif tipo == b"PLTE":
            plte = [tuple(corpo[i:i + 3]) for i in range(0, len(corpo), 3)]
        elif tipo == b"tRNS":
            trns = corpo
        elif tipo == b"IDAT":
            idat += corpo
        off += 12 + n
    if prof != 8 or entrel:
        raise ValueError("só PNG de 8 bits por canal, sem entrelaçamento")
    canais = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctipo]
    raw = zlib.decompress(idat)
    lb = larg * canais
    linhas = []
    ant = bytearray(lb)
    p = 0
    for _ in range(alt):
        filtro = raw[p]
        cur = bytearray(raw[p + 1:p + 1 + lb])
        p += 1 + lb
        for i in range(lb):
            a = cur[i - canais] if i >= canais else 0
            b = ant[i]
            c = ant[i - canais] if i >= canais else 0
            if filtro == 1:
                cur[i] = (cur[i] + a) & 255
            elif filtro == 2:
                cur[i] = (cur[i] + b) & 255
            elif filtro == 3:
                cur[i] = (cur[i] + (a + b) // 2) & 255
            elif filtro == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                cur[i] = (cur[i] + pr) & 255
        linhas.append(bytes(cur))
        ant = cur
    px = []
    for l in linhas:
        for x in range(larg):
            v = l[x * canais:(x + 1) * canais]
            if ctipo == 6:
                px.append(tuple(v))
            elif ctipo == 2:
                px.append((v[0], v[1], v[2], 255))
            elif ctipo == 3:
                r, g, b = plte[v[0]]
                a = trns[v[0]] if trns and v[0] < len(trns) else 255
                px.append((r, g, b, a))
            elif ctipo == 4:
                px.append((v[0], v[0], v[0], v[1]))
            else:
                px.append((v[0], v[0], v[0], 255))
    return larg, alt, px


def quantizar(px, n_cores):
    """Corte mediano sobre as cores distintas (com alfa). Devolve (paleta, índices)."""
    distintas = {}
    for p in px:
        distintas[p] = distintas.get(p, 0) + 1
    caixas = [list(distintas.items())]
    while len(caixas) < n_cores:
        caixas.sort(key=lambda c: -_extensao(c))
        c = caixas.pop(0)
        if len(c) < 2:
            caixas.append(c)
            break
        canal = max(range(4), key=lambda k: max(x[0][k] for x in c) - min(x[0][k] for x in c))
        c.sort(key=lambda x: x[0][canal])
        meio = len(c) // 2
        caixas += [c[:meio], c[meio:]]
    paleta = []
    for c in caixas:
        tot = sum(w for _, w in c)
        paleta.append(tuple(int(round(sum(p[k] * w for p, w in c) / tot)) for k in range(4)))
    cache = {}
    def mais_perto(p):
        if p not in cache:
            cache[p] = min(range(len(paleta)), key=lambda i: sum((paleta[i][k] - p[k]) ** 2 for k in range(4)))
        return cache[p]
    return paleta, [mais_perto(p) for p in px]


def _extensao(c):
    if len(c) < 2:
        return -1
    return max(max(x[0][k] for x in c) - min(x[0][k] for x in c) for k in range(4)) * len(c) ** 0.25


def alfa_gs(paleta):
    return [(r, g, b, (a * 128 + 127) // 255) for r, g, b, a in paleta]


def reduzir(larg, alt, px, nova):
    """Reamostra por média de blocos (lado nova×nova, nova divide larg e alt)."""
    fx, fy = larg // nova, alt // nova
    out = []
    for y in range(nova):
        for x in range(nova):
            bloco = [px[(y * fy + j) * larg + x * fx + i] for j in range(fy) for i in range(fx)]
            out.append(tuple(sum(p[k] for p in bloco) // len(bloco) for k in range(4)))
    return out


def sombra_blob(lado=32):
    """Sombra "blob" redonda e suave (original do Brisa): preto com alfa radial."""
    px = []
    for y in range(lado):
        for x in range(lado):
            dx = (x + 0.5) / lado * 2 - 1
            dy = (y + 0.5) / lado * 2 - 1
            d = (dx * dx + dy * dy) ** 0.5
            a = 0 if d >= 1 else int(255 * (1 - d * d) ** 1.5 * 0.55)
            px.append((20, 24, 40, a))
    return px
