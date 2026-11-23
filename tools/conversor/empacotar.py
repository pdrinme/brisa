#!/usr/bin/env python3
"""Monta o pacote de conteúdo do Brisa (BRISA.PAK) a partir de assets/.

Recusa o build (código de saída 1) se:
  - um arquivo de assets/{modelos,animacoes,texturas} não está no catálogo;
  - o catálogo aponta para um arquivo que não existe ou cujo CRC mudou;
  - a licença não é uma das permitidas (ASSET_LICENSES.md §1).

Uso: python3 tools/conversor/empacotar.py [--saida build/conteudo/BRISA.PAK] [--versao-conteudo 1]
"""
import argparse
import json
import os
import sys
import zlib

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import formatos  # noqa: E402

RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
LICENCAS_OK = {"Original", "CC0-1.0", "CC-BY-4.0", "CC-BY-3.0", "OFL-1.1", "MIT", "Apache-2.0"}
PASTAS = ("modelos", "animacoes", "texturas", "mapa")
TIPO = {"modelo": "BMOD", "animacao": "BANI", "textura": "BTEX", "mapa": "BMAP"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--saida", default=os.path.join(RAIZ, "build/conteudo/BRISA.PAK"))
    ap.add_argument("--versao-conteudo", type=int, default=1)
    a = ap.parse_args()
    cat = json.load(open(os.path.join(RAIZ, "assets/catalogo_brisa.json"), encoding="utf-8"))
    erros = []
    no_catalogo = {}
    for x in cat["assets"]:
        caminho = os.path.join(RAIZ, x["arquivo"])
        if x.get("licenca") not in LICENCAS_OK:
            erros.append(f"{x['id']}: licença '{x.get('licenca')}' não permitida")
        if not os.path.exists(caminho):
            erros.append(f"{x['id']}: arquivo {x['arquivo']} não existe")
            continue
        dados = open(caminho, "rb").read()
        if "%08x" % zlib.crc32(dados) != x["crc32"]:
            erros.append(f"{x['id']}: CRC de {x['arquivo']} não confere com o catálogo")
        no_catalogo[os.path.relpath(caminho, os.path.join(RAIZ, "assets"))] = (dados, TIPO[x["tipo"]])
    for p in PASTAS:
        d = os.path.join(RAIZ, "assets", p)
        for nome in sorted(os.listdir(d)) if os.path.isdir(d) else []:
            rel = f"{p}/{nome}"
            if rel not in no_catalogo:
                erros.append(f"{rel}: arquivo sem registro no catálogo (assets/catalogo_brisa.json)")
    if erros:
        print("\n".join("[ERRO] " + e for e in erros))
        print("PAK_RECUSADO")
        sys.exit(1)
    pak = formatos.escrever_pak([(n, d, t) for n, (d, t) in no_catalogo.items()], a.versao_conteudo)
    os.makedirs(os.path.dirname(a.saida), exist_ok=True)
    open(a.saida, "wb").write(pak)
    print(f"PAK_OK {len(no_catalogo)} arquivos, {len(pak)} bytes -> {os.path.relpath(a.saida, RAIZ)}")


if __name__ == "__main__":
    main()
