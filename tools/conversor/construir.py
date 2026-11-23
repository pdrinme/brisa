#!/usr/bin/env python3
"""Pipeline Ventania → Brisa (Etapa 3).

Lê uma cópia LOCAL do Ventania (somente leitura), converte os assets
reaproveitáveis (docs/auditoria/REUSO_GLB.md) e grava em assets/ do Brisa:
  assets/modelos/<id>.bmod (+ _lod1, _lod2 nos personagens)
  assets/animacoes/<nome>.bani
  assets/texturas/<nome>.btex
  assets/catalogo_brisa.json  (origem, autor, licença, commit do Ventania, medidas)
e um relatório em docs/auditoria/CONVERSAO.md.

Uso: python3 tools/conversor/construir.py --ventania ../ventania
"""
import argparse
import json
import os
import subprocess
import sys
import time
import zlib

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import decimar  # noqa: E402
import formatos  # noqa: E402
import mapa  # noqa: E402
import modelo  # noqa: E402
import textura  # noqa: E402

RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
# Orçamentos de LOD0 (triângulos) do Brisa — docs/PIPELINE_ASSETS.md §2.
ORCAMENTO = {"personagens": 3500, "armas": 600, "mochilas_acessorios": 400, "construcoes": 200,
             "cenario": 600, "baus_coletaveis": 500}
LODS_PERSONAGEM = (1000, 300)
NAO_CONVERTER = {"caixote_exemplo_01": "asset de teste do pipeline do Ventania"}
LICENCAS_OK = {"Original", "CC0-1.0", "CC-BY-4.0", "CC-BY-3.0", "OFL-1.1", "MIT", "Apache-2.0"}
ICONES = ["rajada", "trovao", "zumbido", "faisca", "gaviao", "picareta", "curativo", "kit_cura", "celula_p", "celula_g", "municao"]


def reduzir_modelo(m, alvo):
    """Reduz cada parte na proporção do alvo total (partes pequenas ficam com ao menos 2 triângulos)."""
    tot = m.triangulos()
    if tot <= alvo:
        return False
    for p in m.partes:
        n = len(p["tris"])
        p["tris"] = decimar.reduzir(p["tris"], max(2, int(round(n * alvo / tot))))
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ventania", required=True)
    ap.add_argument("--saida", default=os.path.join(RAIZ, "assets"))
    a = ap.parse_args()
    v = os.path.abspath(a.ventania)
    commit = subprocess.run(["git", "-C", v, "log", "-1", "--format=%h"], capture_output=True, text=True).stdout.strip()
    paleta = modelo.carregar_paleta(os.path.join(v, "tools/assets/kit/paleta.json"))
    cat_v = {x["id"]: x for x in json.load(open(os.path.join(v, "assets_catalog.json"), encoding="utf-8"))["assets"]}
    for sub in ("modelos", "animacoes", "texturas", "mapa"):
        os.makedirs(os.path.join(a.saida, sub), exist_ok=True)
    catalogo = []
    relatorio = []
    t0 = time.time()

    def registrar(id_brisa, arquivo, dados, tipo, origem, extra):
        caminho = os.path.join(a.saida, arquivo)
        with open(caminho, "wb") as f:
            f.write(dados)
        catalogo.append(dict({"id": id_brisa, "arquivo": "assets/" + arquivo, "tipo": tipo, "bytes": len(dados),
                              "crc32": "%08x" % zlib.crc32(dados)}, **origem, **extra))

    assinatura_humanoide = None
    convertidos = {}  # id → Modelo (para o mapa: LODs e paredes com aberturas)
    for cat in sorted(ORCAMENTO):
        pasta = os.path.join(v, "game/assets", cat)
        for nome in sorted(os.listdir(pasta)):
            if not nome.endswith(".glb"):
                continue
            aid = nome[:-4]
            if aid in NAO_CONVERTER:
                relatorio.append((aid, cat, "não convertido: " + NAO_CONVERTER[aid]))
                continue
            info = cat_v.get(aid)
            if not info or info.get("licenca") not in LICENCAS_OK:
                raise SystemExit(f"{aid}: sem registro de licença válido no catálogo do Ventania")
            origem = {"origem": f"ventania:game/assets/{cat}/{nome}@{commit}", "autor": info.get("autor", ""),
                      "licenca": info["licenca"], "autorizacao": "titular (ASSET_LICENSES.md §2)",
                      "categoria": cat}
            glb = os.path.join(pasta, nome)
            m = modelo.converter(glb, paleta)
            tris0 = m.triangulos()
            simplificado = reduzir_modelo(m, ORCAMENTO[cat])
            notas = []
            if simplificado:
                notas.append(f"LOD0 reduzido de {tris0} para {m.triangulos()} triângulos (orçamento {ORCAMENTO[cat]})")
            if any(cel is None for cel in []):
                pass
            mats = {"M_vidro_ventania", "M_agua_ventania"} & set(open(glb, "rb").read(4096).decode("latin-1").split('"'))
            if mats:
                notas.append("vidro/água do Ventania viram cor opaca (sem transparência no 3D ainda)")
            registrar(aid, f"modelos/{aid}.bmod", formatos.escrever_modelo(m), "modelo", origem,
                      {"triangulos": m.triangulos(), "triangulos_ventania": tris0, "partes": len(m.partes),
                       "nos": len(m.nos), "marcadores": [k["nome"] for k in m.marcadores], "notas": "; ".join(notas)})
            linha = f"{m.triangulos()} tris" + (f" (de {tris0})" if simplificado else "")
            if cat != "personagens":
                convertidos[aid] = m
            clipes = modelo.animacoes(glb, m) if any(n.animado for n in m.nos) else []
            if cat == "personagens":
                ass = zlib.crc32("|".join(n.nome for n in m.nos[:]).encode())
                ossos = "|".join(n.nome for n in m.nos if n.osso)
                if assinatura_humanoide is None:
                    assinatura_humanoide = ossos
                    registrar("anim_humanoide", "animacoes/humanoide.bani", formatos.escrever_animacoes(m, clipes),
                              "animacao", dict(origem, origem=origem["origem"] + " (animações do esqueleto padrão)"),
                              {"clipes": [c["nome"] for c in clipes], "quadros": sum(len(c["quadros"]) for c in clipes),
                               "nos": len(m.nos), "notas": "esqueleto padrão de 20 ossos, compartilhado por todos os trajes"})
                elif ossos != assinatura_humanoide:
                    raise SystemExit(f"{aid}: esqueleto diferente do padrão")
                for k, alvo in enumerate(LODS_PERSONAGEM, start=1):
                    mk = modelo.converter(glb, paleta)
                    reduzir_modelo(mk, alvo)
                    registrar(f"{aid}_lod{k}", f"modelos/{aid}_lod{k}.bmod", formatos.escrever_modelo(mk), "modelo",
                              origem, {"triangulos": mk.triangulos(), "lod_de": aid, "notas": f"LOD{k} (alvo {alvo})"})
                    linha += f", LOD{k} {mk.triangulos()}"
                _ = ass
            elif clipes:
                registrar(f"anim_{aid}", f"animacoes/{aid}.bani", formatos.escrever_animacoes(m, clipes), "animacao",
                          origem, {"clipes": [c["nome"] for c in clipes], "nos": len(m.nos)})
                linha += f", animações: {', '.join(c['nome'] for c in clipes)}"
            relatorio.append((aid, cat, linha))

    # Texturas: sombra blob (original do Brisa) e ícones do inventário do Ventania.
    blob = textura.sombra_blob(32)
    pal, ind = textura.quantizar(blob, 16)
    registrar("sombra_blob", "texturas/sombra_blob.btex", formatos.escrever_textura(32, 32, 0x14, ind, textura.alfa_gs(pal)),
              "textura", {"origem": "original do Brisa (tools/conversor/textura.py: sombra_blob)", "autor": "Projeto Brisa",
                          "licenca": "Original", "autorizacao": "—", "categoria": "texturas"},
              {"largura": 32, "altura": 32, "formato": "PSMT4"})
    relatorio.append(("sombra_blob", "texturas", "32×32 PSMT4 (original)"))
    for ic in ICONES:
        caminho = os.path.join(v, "game/assets/ui/icones", ic + ".png")
        info = cat_v.get(ic)  # os ícones não estão no catálogo de modelos; licença em ASSET_LICENSES do Ventania
        larg, alt, px = textura.ler_png(caminho)
        px = textura.reduzir(larg, alt, px, 64)
        pal, ind = textura.quantizar(px, 256)
        registrar(f"icone_{ic}", f"texturas/icone_{ic}.btex", formatos.escrever_textura(64, 64, 0x13, ind, textura.alfa_gs(pal)),
                  "textura", {"origem": f"ventania:game/assets/ui/icones/{ic}.png@{commit}", "autor": "Projeto Ventania",
                              "licenca": "Original", "autorizacao": "titular (ASSET_LICENSES.md §2)", "categoria": "texturas"},
                  {"largura": 64, "altura": 64, "formato": "PSMT8", "notas": f"reduzido de {larg}×{alt}; {len(pal)} cores"})
        relatorio.append((f"icone_{ic}", "texturas", f"64×64 PSMT8, {len(pal)} cores (de {larg}×{alt})"))
        _ = info

    # Mapa (Etapa 5): a Ilha Ventania a partir do layout extraído.
    layout = json.load(open(os.path.join(RAIZ, "docs/auditoria/ilha_ventania_layout.json"), encoding="utf-8"))
    t_mapa = time.time()
    mapa.construir(v, paleta, commit, layout, convertidos, registrar, relatorio)
    relatorio.append(("(tempo)", "mapa", f"{time.time() - t_mapa:.1f} s"))

    catalogo.sort(key=lambda x: x["arquivo"])
    json.dump({"_doc": "Catálogo dos assets do Brisa (gerado por tools/conversor/construir.py; ver ASSET_LICENSES.md). "
                       "O empacotador recusa arquivos que não estejam aqui.",
               "ventania_commit": commit, "assets": catalogo},
              open(os.path.join(a.saida, "catalogo_brisa.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    tot = sum(x["bytes"] for x in catalogo)
    linhas = ["# Conversão Ventania → Brisa (gerado)", "",
              f"Ventania `{commit}`. {len(catalogo)} arquivos, {tot / 1024:.0f} KB no total. "
              f"Tempo: {time.time() - t0:.1f} s.", "",
              "| id | categoria | resultado |", "|---|---|---|"]
    linhas += [f"| {i} | {c} | {r} |" for i, c, r in relatorio]
    por_tipo = {}
    for x in catalogo:
        por_tipo[x["tipo"]] = por_tipo.get(x["tipo"], 0) + x["bytes"]
    linhas += ["", "| tipo | KB |", "|---|---|"] + [f"| {k} | {b / 1024:.0f} |" for k, b in sorted(por_tipo.items())]
    open(os.path.join(RAIZ, "docs/auditoria/CONVERSAO.md"), "w", encoding="utf-8").write("\n".join(linhas) + "\n")
    print("\n".join(linhas[-6:]))
    print(f"CONVERSAO_OK {len(catalogo)} arquivos, {tot} bytes")


if __name__ == "__main__":
    main()
