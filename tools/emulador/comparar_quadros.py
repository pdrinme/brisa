#!/usr/bin/env python3
"""Compara um quadro do emulador com a prévia do PC no mesmo passo do roteiro.

Ignora o painel de cima e o de baixo (textos diferentes: PS2/HOST, tempos) e
conta os pixels cuja diferença em algum canal passa de `--limiar`. Falha se a
fração passar de `--max`. Pega o caso "o VU1 não desenhou nada" (que a contagem
de lotes não pega) e erros grosseiros de geometria ou cor.
Uso: comparar_quadros.py emulador.png pc.png [--max 0.04] [--limiar 48]
"""
import argparse
import struct
import sys
import zlib


def ler_png(caminho):
    d = open(caminho, "rb").read()
    assert d[:8] == b"\x89PNG\r\n\x1a\n", caminho
    i, idat, w, h, cor = 8, b"", 0, 0, 2
    while i < len(d):
        n = struct.unpack(">I", d[i:i + 4])[0]
        t, c = d[i + 4:i + 8], d[i + 8:i + 8 + n]
        if t == b"IHDR":
            w, h, prof, cor = struct.unpack(">IIBB", c[:10])
            assert prof == 8 and cor in (2, 6), "só RGB/RGBA de 8 bits"
        elif t == b"IDAT":
            idat += c
        i += 12 + n
    bpp = 3 if cor == 2 else 4
    cru = zlib.decompress(idat)
    linhas, ant = [], bytearray(w * bpp)
    for y in range(h):
        f = cru[y * (w * bpp + 1)]
        lin = bytearray(cru[y * (w * bpp + 1) + 1:(y + 1) * (w * bpp + 1)])
        for x in range(len(lin)):
            a = lin[x - bpp] if x >= bpp else 0
            b = ant[x]
            c = ant[x - bpp] if x >= bpp else 0
            if f == 1:
                lin[x] = (lin[x] + a) & 255
            elif f == 2:
                lin[x] = (lin[x] + b) & 255
            elif f == 3:
                lin[x] = (lin[x] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                lin[x] = (lin[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        linhas.append(bytes(lin))
        ant = lin
    return w, h, bpp, linhas


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("emulador")
    ap.add_argument("pc")
    ap.add_argument("--max", type=float, default=0.04)
    ap.add_argument("--limiar", type=int, default=48)
    ap.add_argument("--y0", type=int, default=92)
    ap.add_argument("--y1", type=int, default=384)
    a = ap.parse_args()
    w1, h1, b1, l1 = ler_png(a.emulador)
    w2, h2, b2, l2 = ler_png(a.pc)
    if (w1, h1) != (w2, h2):
        print(f"COMPARAR_FALHOU tamanhos diferentes {w1}x{h1} e {w2}x{h2}")
        return 1
    dif = tot = 0
    for y in range(a.y0, min(a.y1, h1)):
        r1, r2 = l1[y], l2[y]
        for x in range(w1):
            tot += 1
            p, q = x * b1, x * b2
            if max(abs(r1[p] - r2[q]), abs(r1[p + 1] - r2[q + 1]), abs(r1[p + 2] - r2[q + 2])) > a.limiar:
                dif += 1
    fr = dif / tot
    ok = fr <= a.max
    print(f"{'COMPARAR_OK' if ok else 'COMPARAR_FALHOU'} {dif} de {tot} pixels diferentes ({fr * 100:.2f}%, máximo {a.max * 100:.1f}%)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
