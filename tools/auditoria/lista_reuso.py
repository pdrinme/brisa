#!/usr/bin/env python3
"""Lista de reaproveitamento dos assets do Ventania no Brisa (Etapa 0).

Lê docs/auditoria/assets_ventania.json (de auditar_ventania.py) e classifica
cada GLB com as regras de ASSET_LICENSES.md §3. A decisão final de cada asset é
registrada no catálogo do Brisa quando ele for convertido (Etapa 3).

Uso:
    python3 tools/auditoria/lista_reuso.py docs/auditoria/assets_ventania.json > docs/auditoria/REUSO_GLB.md
"""
import json
import sys

# Orçamentos de LOD0 do Brisa (triângulos). Ver BRISA_ARCHITECTURE.md §5.2.
ORCAMENTO = {"personagens": 3500, "armas": 600, "mochilas_acessorios": 400, "construcoes": 200,
             "cenario": 600, "baus_coletaveis": 500}
NAO_USAR = {"caixote_exemplo_01": "asset de teste do pipeline do Ventania, não é final"}


def classificar(a):
    if a["id"] in NAO_USAR:
        return "não usar", NAO_USAR[a["id"]]
    if not a["no_catalogo"] or a["licenca"] not in ("Original", "CC0-1.0", "CC-BY-4.0", "OFL-1.1", "MIT"):
        return "bloqueado", "sem registro de licença no catálogo do Ventania"
    notas = []
    dec = "reaproveitar"
    lim = ORCAMENTO.get(a["categoria"], 600)
    if a["tris"] > lim:
        dec = "reaproveitar com simplificação"
        notas.append(f"{a['tris']} tris > orçamento LOD0 {lim}")
    if a["categoria"] == "personagens":
        dec = "reaproveitar com LODs"
        notas.append("LOD1 ≈ 1000 e LOD2 ≈ 300 tris; skinning rígido confirmado" if a["skinning_rigido"] else "ATENÇÃO: skinning não rígido")
    if len(a["materiais"]) > 1:
        notas.append("materiais especiais (vidro/água) → alfa do GS + cor de vértice")
    if a["animacoes"]:
        notas.append(f"{len(a['animacoes'])} animações")
    if a["tris_colisao"]:
        notas.append(f"colisão própria {a['tris_colisao']} tris → caixa/convexo simplificado")
    return dec, "; ".join(notas) or "dentro do orçamento"


def main():
    d = json.load(open(sys.argv[1], encoding="utf-8"))
    print("# Reaproveitamento dos GLB do Ventania (gerado)\n")
    print("Gerado por `tools/auditoria/lista_reuso.py` a partir da auditoria. Origem de todos: "
          "Projeto Ventania, geradores Blender por script (`tools/assets/kit/blender/`).\n")
    print("| id | categoria | autor | licença | tris | decisão | notas |")
    print("|---|---|---|---|---|---|---|")
    cont = {}
    for a in d["assets"]:
        dec, nota = classificar(a)
        cont[dec] = cont.get(dec, 0) + 1
        print(f"| {a['id']} | {a['categoria']} | {a['autor']} | {a['licenca']} | {a['tris']} | {dec} | {nota} |")
    print("\nResumo: " + ", ".join(f"{k}: {v}" for k, v in sorted(cont.items())) + ".")


if __name__ == "__main__":
    main()
