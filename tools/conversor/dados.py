#!/usr/bin/env python3
"""Compilador de dados de jogo (Etapa 6): data/itens.json → assets/dados/itens.bdat.

O JSON é a fonte legível (cópia revisada do Ventania); o jogo lê o binário sem
interpretar texto. Valida as regras antes de gravar e registra o arquivo no
catálogo (assets/catalogo_brisa.json). Formato: docs/FORMATOS.md (BDAT).

Uso: python3 tools/conversor/dados.py
"""
import json
import os
import struct
import sys
import zlib

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import formatos  # noqa: E402

RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
VERSAO_DADOS = 1
# Pose do item na mão (personagem/portador): categoria pelo tipo da arma (item_rig.gd do Ventania).
POSE_RIFLE, POSE_PRECISAO, POSE_PISTOLA = 1, 2, 3


class ErroDados(Exception):
    pass


def _texto(s, n, campo):
    b = s.encode("utf-8")
    if len(b) >= n:
        raise ErroDados(f"{campo}: '{s}' passa de {n - 1} bytes")
    return b + b"\0" * (n - len(b))


def _cor(h):
    h = h.lstrip("#")
    return int(h, 16)


def validar(d, modelos_existentes):
    """Regras do PIPELINE_ASSETS §5: tudo que é referenciado existe."""
    erros = []
    mun = [k for k in d["municao"] if not k.startswith("_")]
    for wid, w in d["armas"].items():
        if w["municao"] not in mun:
            erros.append(f"arma {wid}: munição '{w['municao']}' não existe")
        if w["modelo"] not in modelos_existentes:
            erros.append(f"arma {wid}: modelo '{w['modelo']}' não está em assets/modelos")
        for c in ("dano", "cadencia", "pente", "recarga_s", "alcance", "projeteis"):
            if float(w[c]) <= 0:
                erros.append(f"arma {wid}: {c} deve ser positivo")
    for cid, c in d["consumiveis"].items():
        if c["modelo"] not in modelos_existentes:
            erros.append(f"consumível {cid}: modelo '{c['modelo']}' não está em assets/modelos")
        if ("vida" in c) == ("escudo" in c):
            erros.append(f"consumível {cid}: precisa de 'vida' ou 'escudo' (só um)")
        if int(c["quantidade"]) > int(c["pilha"]):
            erros.append(f"consumível {cid}: quantidade maior que a pilha")
    for tid, t in d["ferramentas"].items():
        if t["modelo"] not in modelos_existentes:
            erros.append(f"ferramenta {tid}: modelo '{t['modelo']}' não está em assets/modelos")
    pesos = [float(r["peso_bau"]) for r in d["raridades"]]
    if sum(pesos) <= 0:
        erros.append("raridades: nenhuma sorteável")
    if erros:
        raise ErroDados("\n".join(erros))


def compilar_itens(d):
    mun = [k for k in d["municao"] if not k.startswith("_")]
    mat = [k for k in d["materiais"] if not k.startswith("_")]
    b_rar = b""
    for r in d["raridades"]:
        b_rar += _texto(r["id"], 12, "raridade") + _texto(r["nome"], 16, "raridade") + struct.pack(
            "<IIffff", _cor(r["cor"]), _cor(r.get("cor2", r["cor"])), r["dano"], r["recarga"], r["peso_bau"], r["peso_chao"])
    b_mun = b"".join(_texto(k, 12, "munição") + _texto(d["municao"][k]["nome"], 24, "munição") +
                     struct.pack("<II4x", d["municao"][k]["max"], d["municao"][k]["caixa"]) for k in mun)
    b_mat = b"".join(_texto(k, 12, "material") + _texto(d["materiais"][k]["nome"], 16, "material") +
                     struct.pack("<I", d["materiais"][k]["max"]) for k in mat)
    b_arm = b""
    for wid, w in d["armas"].items():
        pose = POSE_PISTOLA if w["tipo"] == "Pistola" else (POSE_PRECISAO if w["tipo"] == "Rifle de Precisão" else POSE_RIFLE)
        b_arm += (_texto(wid, 12, "arma") + _texto(w["nome"], 16, "arma") + _texto(w["tipo"], 24, "arma") +
                  _texto(w["modelo"], 24, "arma") +
                  struct.pack("<IffIIfffIffffI", mun.index(w["municao"]), w["dano"], w["cadencia"], 1 if w["automatica"] else 0,
                              w["pente"], w["recarga_s"], w["alcance"], w["dispersao"], w["projeteis"], w["cabeca"],
                              w["peso"], float(w.get("dispersao_mirando", -1.0)), 0.0, pose))
    b_con = b""
    for cid, c in d["consumiveis"].items():
        b_con += (_texto(cid, 12, "consumível") + _texto(c["nome"], 32, "consumível") + _texto(c["modelo"], 24, "consumível") +
                  struct.pack("<ffffIIf4x", float(c.get("vida", 0)), float(c.get("escudo", 0)), c["limite"], c["tempo_s"],
                              c["pilha"], c["quantidade"], c["peso"]))
    b_fer = b""
    for tid, t in d["ferramentas"].items():
        b_fer += (_texto(tid, 12, "ferramenta") + _texto(t["nome"], 16, "ferramenta") + _texto(t["modelo"], 24, "ferramenta") +
                  struct.pack("<fffII4x", t["cadencia"], t["impacto"], t["alcance"], t["dano_jogador"], t["dano_construcao"]))
    m = d["modificadores_dispersao"]
    b_mod = struct.pack("<ffff", m["mirando"], m["movendo"], m["agachado"], m["no_ar"])
    bau = d["bau"]
    b_bau = struct.pack("<III4x", bau["armas"], bau["consumiveis"], bau["caixas_municao"])
    blocos = [b_rar, b_mun, b_mat, b_arm, b_con, b_fer, b_mod, b_bau]
    corpo, offs, off = b"", [], 64
    for b in blocos:
        b = formatos._alinhar(b)
        offs.append(off)
        corpo += b
        off += len(b)
    cab = b"BDAT" + struct.pack("<I6I", VERSAO_DADOS, len(d["raridades"]), len(mun), len(mat), len(d["armas"]),
                                len(d["consumiveis"]), len(d["ferramentas"]))
    cab += struct.pack("<8I", *offs)
    cab += b"\0" * (64 - len(cab))
    assert len(cab) == 64
    return cab + corpo


def main():
    d = json.load(open(os.path.join(RAIZ, "data/itens.json"), encoding="utf-8"))
    modelos = {n[:-5] for n in os.listdir(os.path.join(RAIZ, "assets/modelos")) if n.endswith(".bmod")}
    validar(d, modelos)
    dados = compilar_itens(d)
    os.makedirs(os.path.join(RAIZ, "assets/dados"), exist_ok=True)
    caminho = os.path.join(RAIZ, "assets/dados/itens.bdat")
    open(caminho, "wb").write(dados)
    # Registro no catálogo (substitui a entrada anterior, se houver).
    cat_p = os.path.join(RAIZ, "assets/catalogo_brisa.json")
    cat = json.load(open(cat_p, encoding="utf-8"))
    cat["assets"] = [x for x in cat["assets"] if x["id"] != "dados_itens"]
    cat["assets"].append({"id": "dados_itens", "arquivo": "assets/dados/itens.bdat", "tipo": "dados", "bytes": len(dados),
                          "crc32": "%08x" % zlib.crc32(dados),
                          "origem": f"data/itens.json (cópia revisada de ventania:game/data/itens.json@{cat.get('ventania_commit', '')})",
                          "autor": "Projeto Ventania / Projeto Brisa", "licenca": "Original",
                          "autorizacao": "titular (ASSET_LICENSES.md §2)", "categoria": "dados",
                          "armas": len(d["armas"]), "consumiveis": len(d["consumiveis"]), "raridades": len(d["raridades"])})
    cat["assets"].sort(key=lambda x: x["arquivo"])
    json.dump(cat, open(cat_p, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print(f"DADOS_OK itens.bdat {len(dados)} bytes ({len(d['armas'])} armas, {len(d['consumiveis'])} consumíveis)")


if __name__ == "__main__":
    try:
        main()
    except ErroDados as e:
        print("[ERRO]", e)
        print("DADOS_RECUSADOS")
        sys.exit(1)
