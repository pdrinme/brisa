"""Escritores dos formatos binários do Brisa (little-endian, alinhados a 16).
A especificação byte a byte está em docs/FORMATOS.md; o leitor do jogo é
src/conteudo/*.cpp. Mudou aqui, mude lá (e suba a versão)."""
import math
import struct
import zlib

VERSAO_MODELO = 2  # v2: colisões com tipo (caixa/rampa) e espessura
VERSAO_ANIM = 1
VERSAO_TEXTURA = 1
VERSAO_PAK = 1


def _alinhar(b, n=16):
    return b + b"\0" * ((-len(b)) % n)


def _nome(s, n):
    e = s.encode("utf-8")[: n - 1]
    return e + b"\0" * (n - len(e))


def _normal_i8(n):
    def q(v):
        i = int(round(v * 127.0))
        return max(-127, min(127, i)) & 0xFF
    return q(n[0]) | (q(n[1]) << 8) | (q(n[2]) << 16)


def _esfera(pontos):
    mn = [min(p[i] for p in pontos) for i in range(3)]
    mx = [max(p[i] for p in pontos) for i in range(3)]
    c = [(a + b) / 2 for a, b in zip(mn, mx)]
    r = max(math.dist(c, p) for p in pontos)
    return c, r, mn, mx


def escrever_modelo(m):
    """Modelo (BMOD): cabeçalho 128 B, partes, nós, marcadores, colisões, posições, normais, cores."""
    pos, nrm, cor, partes = [], [], [], []
    for p in m.partes:
        # Cada parte começa num múltiplo de 4 vértices: o VIF lê normal/cor de 16 em
        # 16 bytes e o DMA exige endereço alinhado. Os vértices de folga ficam zerados.
        while len(pos) % 4:
            pos.append((0.0, 0.0, 0.0))
            nrm.append(0)
            cor.append(0)
        ini = len(pos)
        pts = []
        for ps, ns, c in p["tris"]:
            for v, n in zip(ps, ns):
                pos.append(v)
                nrm.append(_normal_i8(n))
                cor.append(c[0] | (c[1] << 8) | (c[2] << 16) | (255 << 24))
                pts.append(v)
        if pts:
            centro, raio, _, _ = _esfera(pts)
            partes.append((p["no"], ini, len(pos) - ini, centro, raio))
    nv = len(pos)
    centro, raio, mn, mx = _esfera(pos) if pos else ([0, 0, 0], 0, [0, 0, 0], [0, 0, 0])
    b_partes = b"".join(struct.pack("<iIII3ff", no, ini, n, 0, *c, r) for no, ini, n, c, r in partes)
    b_nos = b""
    for no in m.nos:
        flags = (1 if no.animado else 0) | (2 if no.osso else 0)
        b_nos += _nome(no.nome, 24) + struct.pack("<iI", no.pai, flags)
        b_nos += struct.pack("<3ff4f3ff", *no.t, 0.0, *no.r, *no.s, 0.0)
        b_nos += struct.pack("<16f", *no.inv_bind)
    b_marc = b"".join(_nome(k["nome"], 24) + struct.pack("<i3f4x4x", k["no"], *k["pos"]) for k in m.marcadores)
    b_col = b"".join(struct.pack("<3fI3ff", *a, tipo, *b, esp) for a, b, tipo, esp in m.colisoes)
    nv_pad = (nv + 4 + 3) // 4 * 4  # folga zerada: o VIF lê de 16 em 16 bytes
    b_pos = b"".join(struct.pack("<4f", *v, 1.0) for v in pos)
    b_nrm = struct.pack(f"<{nv}I", *nrm) + b"\0" * 4 * (nv_pad - nv)
    b_cor = struct.pack(f"<{nv}I", *cor) + b"\0" * 4 * (nv_pad - nv)
    corpo = b""
    offs = []
    off = 128
    for bloco in (b_partes, b_nos, b_marc, b_col, b_pos, b_nrm, b_cor):
        bloco = _alinhar(bloco)
        offs.append(off if bloco else 0)
        corpo += bloco
        off += len(bloco)
    cab = b"BMOD" + struct.pack("<IIIIIII", VERSAO_MODELO, nv, len(partes), len(m.nos), len(m.marcadores),
                                len(m.colisoes), 1 if m.esqueletico else 0)
    cab += struct.pack("<3ff", *centro, raio) + struct.pack("<3ff", *mn, 0.0) + struct.pack("<3ff", *mx, 0.0)
    cab += struct.pack("<7I", *offs) + b"\0" * 4
    assert len(cab) == 112, len(cab)
    cab = _alinhar(cab + b"\0" * 16, 128)[:128]
    return cab + corpo


def assinatura_esqueleto(nos):
    """CRC32 de "índice:nome" dos ossos e nós animados, unidos por '|'. Os trajes
    compartilham o esqueleto, mas o nó da malha tem o nome de cada traje."""
    partes = [f"{i}:{n.nome[:23]}" for i, n in enumerate(nos) if n.osso or n.animado]
    return zlib.crc32("|".join(partes).encode("utf-8"))


def escrever_animacoes(m, clipes, fps=30):
    """Animações (BANI): cabeçalho 32 B, clipes (48 B), quadros (16 B por nó: quat i16×4, translação em mm i16×3)."""
    assinatura = assinatura_esqueleto(m.nos)
    tab = b""
    dados = b""
    base = 32 + 48 * len(clipes)
    for c in clipes:
        off = base + len(dados)
        tab += _nome(c["nome"], 24) + struct.pack("<IIII4x4x", len(c["quadros"]), 1 if c["loop"] else 0, off, len(m.nos))
        for linha in c["quadros"]:
            for q, t in linha:
                qi = [max(-32767, min(32767, int(round(v * 32767)))) for v in q]
                ti = [max(-32768, min(32767, int(round(v * 1000)))) for v in t]
                dados += struct.pack("<4h3hh", *qi, *ti, 0)
    cab = b"BANI" + struct.pack("<IIIII", VERSAO_ANIM, len(m.nos), len(clipes), fps, assinatura) + b"\0" * 8
    return cab + tab + dados


def escrever_textura(larg, alt, psm, indices, clut):
    """Textura (BTEX): psm 0x13 = 8 bits (CLUT 256, já no arranjo CSM1 do GS), 0x14 = 4 bits (CLUT 16).
    clut: [(r, g, b, a 0..128)]."""
    if psm == 0x13:
        pix = bytes(indices)
        n = 256
        clut = list(clut) + [(0, 0, 0, 0)] * (256 - len(clut))
        clut = clut_csm1(clut)
    else:
        n = 16
        clut = list(clut) + [(0, 0, 0, 0)] * (16 - len(clut))
        pix = bytes((indices[i] & 15) | ((indices[i + 1] & 15) << 4) for i in range(0, len(indices), 2))
    b_clut = b"".join(struct.pack("<4B", *c) for c in clut)
    off_pix = 64
    off_clut = off_pix + len(_alinhar(pix, 64))
    cab = b"BTEX" + struct.pack("<IHHBBHIII", VERSAO_TEXTURA, larg, alt, psm, 0, n, off_pix, off_clut, len(pix))
    cab = cab + b"\0" * (64 - len(cab))
    return cab + _alinhar(pix, 64) + b_clut


def clut_csm1(c):
    """CLUT de 256 cores no arranjo CSM1 do GS: em cada bloco de 32, as entradas 8–15 e 16–23 trocam de lugar."""
    out = list(c)
    for b in range(0, 256, 32):
        out[b + 8:b + 16], out[b + 16:b + 24] = c[b + 16:b + 24], c[b + 8:b + 16]
    return out


def escrever_pak(arquivos, versao_conteudo):
    """Pacote (BPAK): cabeçalho 64 B, índice (64 B por arquivo), dados alinhados a 64."""
    arquivos = sorted(arquivos, key=lambda a: a[0])
    n = len(arquivos)
    off = 64 + 64 * n
    off = (off + 63) // 64 * 64
    indice = b""
    dados = b""
    for nome, conteudo, tipo in arquivos:
        if len(nome.encode("utf-8")) > 43:
            raise ValueError(f"nome longo demais no pacote: {nome}")
        pos = off + len(dados)
        indice += _nome(nome, 44) + struct.pack("<III4s4x", pos, len(conteudo), zlib.crc32(conteudo), tipo.encode()[:4].ljust(4, b"\0"))
        dados += _alinhar(conteudo, 64)
    total = off + len(dados)
    cab = b"BPAK" + struct.pack("<IIIIII", VERSAO_PAK, versao_conteudo, n, 64, total, zlib.crc32(indice))
    cab += b"\0" * (64 - len(cab))
    return _alinhar(cab + indice, 64) + dados
