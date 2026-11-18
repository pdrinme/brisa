#!/usr/bin/env python3
"""Converte PPM (P6) em PNG só com a biblioteca padrão.

Uso: ppm_para_png.py entrada.ppm saida.png
     ppm_para_png.py --todos <pasta>   (converte cada .ppm e apaga o .ppm)
"""
import os
import struct
import sys
import zlib


def ler_ppm(caminho):
    d = open(caminho, "rb").read()
    partes = d.split(b"\n", 3)
    assert partes[0] == b"P6", "não é PPM P6"
    larg, alt = map(int, partes[1].split())
    return larg, alt, partes[3]


def salvar_png(caminho, larg, alt, rgb):
    def bloco(tipo, dados):
        c = zlib.crc32(tipo + dados) & 0xFFFFFFFF
        return struct.pack(">I", len(dados)) + tipo + dados + struct.pack(">I", c)
    linhas = b"".join(b"\x00" + rgb[y * larg * 3:(y + 1) * larg * 3] for y in range(alt))
    png = b"\x89PNG\r\n\x1a\n" + bloco(b"IHDR", struct.pack(">IIBBBBB", larg, alt, 8, 2, 0, 0, 0))
    png += bloco(b"IDAT", zlib.compress(linhas, 9)) + bloco(b"IEND", b"")
    open(caminho, "wb").write(png)


if __name__ == "__main__":
    if sys.argv[1] == "--todos":
        pasta = sys.argv[2]
        for nome in sorted(os.listdir(pasta)):
            if nome.endswith(".ppm"):
                p = os.path.join(pasta, nome)
                salvar_png(p[:-4] + ".png", *ler_ppm(p))
                os.remove(p)
                print("png:", p[:-4] + ".png")
    else:
        salvar_png(sys.argv[2], *ler_ppm(sys.argv[1]))
