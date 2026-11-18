#!/usr/bin/env python3
"""Densidade de geometria da Ilha Ventania por setor (Etapa 0 do Brisa).

Cruza o layout extraído (extrair_ilha.sh) com a auditoria dos GLB
(auditar_ventania.py) e soma triângulos por setor quadrado. Serve para
dimensionar setores, distância de visão e LOD no PS2.

Uso:
    python3 tools/auditoria/densidade_mapa.py --layout docs/auditoria/ilha_ventania_layout.json \
        --assets docs/auditoria/assets_ventania.json --setor 64
"""
import argparse
import json
import math

TIPOS = ["parede", "parede", "piso", "rampa", "teto"]  # Grade: PAREDE_X, PAREDE_Z, PISO, RAMPA, TETO
MATS = ["madeira", "pedra", "metal"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--layout", required=True)
    ap.add_argument("--assets", required=True)
    ap.add_argument("--setor", type=float, default=64.0)
    ap.add_argument("--json")
    a = ap.parse_args()
    lay = json.load(open(a.layout, encoding="utf-8"))
    tris = {x["id"]: x["tris"] for x in json.load(open(a.assets, encoding="utf-8"))["assets"]}
    setores = {}

    def somar(x, z, t, tipo):
        k = (math.floor(x / a.setor), math.floor(z / a.setor))
        s = setores.setdefault(k, {"tris": 0, "objetos": 0, "pecas": 0, "props": 0})
        s["tris"] += t
        s["objetos"] += 1
        s[tipo] += 1

    for p in lay["pecas"]:
        nome = f"{TIPOS[p['t']]}_{MATS[p['mat']]}"
        x = p["c"][0] * 4 + 2
        z = p["c"][2] * 4 + 2
        somar(x, z, tris.get(nome, 150), "pecas")
    for p in lay["props"]:
        somar(p["p"][0], p["p"][2], tris.get(p["m"], 150), "props")
    tot_p = sum(tris.get(f"{TIPOS[p['t']]}_{MATS[p['mat']]}", 150) for p in lay["pecas"])
    tot_r = sum(tris.get(p["m"], 150) for p in lay["props"])
    n = lay["relevo"]["n"]
    tris_relevo = (n - 1) * (n - 1) * 2
    ordem = sorted(setores.items(), key=lambda kv: -kv[1]["tris"])
    res = {
        "setor_m": a.setor,
        "pecas_mapa": len(lay["pecas"]), "tris_pecas": tot_p,
        "props": len(lay["props"]), "tris_props": tot_r,
        "tris_relevo_lod0": tris_relevo,
        "setores_ocupados": len(setores),
        "top10": [{"setor": list(k), "centro_m": [(k[0] + 0.5) * a.setor, (k[1] + 0.5) * a.setor], **v} for k, v in ordem[:10]],
        "mediana_tris_setor": sorted(v["tris"] for v in setores.values())[len(setores) // 2],
    }
    if a.json:
        json.dump(res, open(a.json, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print(json.dumps(res, ensure_ascii=False, indent=1))


if __name__ == "__main__":
    main()
