#!/usr/bin/env python3
"""Auditoria dos assets do Ventania para o Brisa (Etapa 0).

Lê (sem alterar) os GLB de <ventania>/game/assets/ e mede, por arquivo:
triângulos, vértices únicos, materiais, ossos, influências por vértice
(confirma skinning rígido), animações (nome, duração, chaves), células da
paleta usadas (pela UV) e uma estimativa de memória no formato do PS2.

Uso:
    python3 tools/auditoria/auditar_ventania.py --ventania ../ventania \
        --json docs/auditoria/assets_ventania.json --md docs/auditoria/assets_ventania.md
"""
import argparse
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from glb import Glb  # noqa: E402

# Estimativa do formato do Brisa (BRISA_ARCHITECTURE.md §6): vértice em tiras
# para o VU1 = posição 3×int16 + cor RGBA8 (da paleta) + normal 3×int8 ≈ 12 B,
# arredondado a 16 B por vértice (alinhamento de qword do VIF).
BYTES_VERTICE_PS2 = 16


def celulas_paleta(uvs, atlas=256, celula=32):
    por_linha = atlas // celula
    cels = set()
    for u, v in uvs:
        cx = min(int(u * por_linha), por_linha - 1)
        cy = min(int(v * por_linha), por_linha - 1)
        cels.add(cy * por_linha + cx)
    return cels


def auditar(caminho, nomes_paleta):
    g = Glb(caminho)
    j = g.json
    tris = verts = 0
    max_infl = 0
    infl_hist = {}
    cels = set()
    materiais = set()
    tris_col = 0
    malhas_col = {n["mesh"] for n in j.get("nodes", []) if "mesh" in n and "colonly" in n.get("name", "")}
    for mi, _, p in g.primitivas():
        if p.get("mode", 4) != 4:
            continue
        at = p["attributes"]
        if mi in malhas_col:  # colisão (-colonly/-convcolonly): não é desenhada
            ia = p.get("indices")
            tris_col += (j["accessors"][ia]["count"] if ia is not None else j["accessors"][at["POSITION"]]["count"]) // 3
            continue
        nv = j["accessors"][at["POSITION"]]["count"]
        verts += nv
        if "indices" in p:
            tris += j["accessors"][p["indices"]]["count"] // 3
        else:
            tris += nv // 3
        if "material" in p:
            materiais.add(j["materials"][p["material"]].get("name", str(p["material"])))
        if "TEXCOORD_0" in at:
            cels |= celulas_paleta(g.accessor(at["TEXCOORD_0"]))
        if "WEIGHTS_0" in at:
            for w in g.accessor(at["WEIGHTS_0"]):
                k = sum(1 for x in w if x > 0.001)
                infl_hist[k] = infl_hist.get(k, 0) + 1
                max_infl = max(max_infl, k)
    ossos = sum(len(s["joints"]) for s in j.get("skins", []))
    anims = []
    for a in j.get("animations", []):
        dur = 0.0
        chaves = 0
        for c in a["channels"]:
            s = a["samplers"][c["sampler"]]
            t = g.accessor(s["input"])
            chaves += len(t)
            if t:
                dur = max(dur, t[-1][0])
        anims.append({"nome": a.get("name", ""), "duracao_s": round(dur, 3), "canais": len(a["channels"]), "chaves": chaves})
    nos_marcadores = [n.get("name") for n in j.get("nodes", []) if "mesh" not in n and "skin" not in n and n.get("name") and not n.get("children")]
    texturas = [im.get("name") or im.get("uri") or "embutida" for im in j.get("images", [])]
    return {
        "arquivo": caminho,
        "tris": tris,
        "tris_colisao": tris_col,
        "vertices_gltf": verts,
        "materiais": sorted(materiais),
        "texturas": texturas,
        "ossos": ossos,
        "max_influencias": max_infl,
        "skinning_rigido": max_infl <= 1 if ossos else None,
        "animacoes": anims,
        "celulas_paleta": sorted(nomes_paleta[c] if c < len(nomes_paleta) else f"vazia_{c}" for c in cels),
        "marcadores": nos_marcadores[:16],
        "estimativa_ps2_kb": round(verts * BYTES_VERTICE_PS2 / 1024, 1),
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ventania", required=True)
    ap.add_argument("--json")
    ap.add_argument("--md")
    a = ap.parse_args()
    raiz = os.path.abspath(a.ventania)
    paleta = json.load(open(os.path.join(raiz, "tools/assets/kit/paleta.json"), encoding="utf-8"))
    nomes = [c[0] for c in paleta["cores"]]
    catalogo = {x["id"]: x for x in json.load(open(os.path.join(raiz, "assets_catalog.json"), encoding="utf-8"))["assets"]}
    res = []
    base = os.path.join(raiz, "game", "assets")
    for cat in sorted(os.listdir(base)):
        d = os.path.join(base, cat)
        if not os.path.isdir(d):
            continue
        for nome in sorted(os.listdir(d)):
            if nome.endswith(".glb"):
                r = auditar(os.path.join(d, nome), nomes)
                aid = nome[:-4]
                c = catalogo.get(aid, {})
                r["arquivo"] = os.path.relpath(r["arquivo"], raiz)
                r.update({"id": aid, "categoria": cat, "origem": c.get("origem", "SEM REGISTRO"),
                          "licenca": c.get("licenca", "SEM REGISTRO"), "autor": c.get("autor", ""),
                          "no_catalogo": aid in catalogo})
                res.append(r)
    if a.json:
        os.makedirs(os.path.dirname(a.json), exist_ok=True)
        try:
            commit = subprocess.run(["git", "-C", raiz, "log", "-1", "--format=%h %s"], capture_output=True, text=True).stdout.strip()
        except OSError:
            commit = ""
        json.dump({"ventania_commit": commit, "assets": res}, open(a.json, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    linhas = ["| id | categoria | tris | col. | vért. | ossos | rígido | anims | mat. | licença | PS2 (KB) |", "|---|---|---|---|---|---|---|---|---|---|---|"]
    for r in res:
        rig = "—" if r["skinning_rigido"] is None else ("sim" if r["skinning_rigido"] else f"não ({r['max_influencias']})")
        linhas.append(f"| {r['id']} | {r['categoria']} | {r['tris']} | {r['tris_colisao']} | {r['vertices_gltf']} | {r['ossos']} | {rig} | {len(r['animacoes'])} | {len(r['materiais'])} | {r['licenca']} | {r['estimativa_ps2_kb']} |")
    tot_t = sum(r["tris"] for r in res)
    linhas.append(f"\nTotal: {len(res)} GLB, {tot_t} triângulos, {sum(r['vertices_gltf'] for r in res)} vértices; sem registro no catálogo: {sum(1 for r in res if not r['no_catalogo'])}.")
    txt = "\n".join(linhas)
    if a.md:
        open(a.md, "w", encoding="utf-8").write(txt + "\n")
    print(txt)


if __name__ == "__main__":
    main()
