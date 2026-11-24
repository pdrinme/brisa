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


CAMPOS_DIF = ["reacao_s", "erro_mira_graus", "alcance_visao_m", "campo_visao_graus", "engaja_m", "cura_abaixo", "constroi",
              "coleta", "salto_cedo", "decisao_hz", "mira_assenta_s", "memoria_s", "investiga", "usa_cobertura",
              "cobertura_vida", "recua_vida", "atira_andando", "rajada_ticks", "pausa_ticks", "ritmo_lateral", "explora_predios"]


def validar_bots(d, nomes, modelos_existentes):
    erros = []
    difs = list(d["dificuldades"])
    for k in d["mistura"]:
        if k not in difs:
            erros.append(f"bots: mistura cita a dificuldade '{k}', que não existe")
    for k, v in d["dificuldades"].items():
        for c in CAMPOS_DIF:
            if c not in v:
                erros.append(f"bots: dificuldade {k} sem '{c}'")
    for t in d["trajes_dos_bots"]:
        if t not in modelos_existentes:
            erros.append(f"bots: traje '{t}' não está em assets/modelos")
    if len(nomes) < d["preencher_ate"]:
        erros.append("bots: nomes de menos para encher a partida")
    if erros:
        raise ErroDados("\n".join(erros))


def compilar_bots(d, nomes):
    difs = list(d["dificuldades"])
    b_dif = b""
    for k in difs:
        v = d["dificuldades"][k]
        b_dif += _texto(v["nome"], 16, "dificuldade") + struct.pack(f"<{len(CAMPOS_DIF)}f", *[float(v[c]) for c in CAMPOS_DIF])
    b_mis = bytes(difs.index(k) for k in d["mistura"])
    b_nom = b"".join(_texto(n, 24, "nome de jogador") for n in nomes)
    b_tra = b"".join(_texto(t, 24, "traje") for t in d["trajes_dos_bots"])
    blocos = [b_dif, b_mis, b_nom, b_tra]
    corpo, offs, off = b"", [], 64
    for b in blocos:
        b = formatos._alinhar(b)
        offs.append(off)
        corpo += b
        off += len(b)
    cab = b"BBOT" + struct.pack("<IIIIIIfffIf", 1, d["preencher_ate"], len(difs), len(d["mistura"]), len(nomes),
                                len(d["trajes_dos_bots"]), d["decisao_hz"], d["decisao_longe_hz"], d["lod_distancia_m"],
                                d["raios_visao_por_tick"], d["audicao_tiro_m"])
    cab += struct.pack("<4I", *offs)
    assert len(cab) <= 64
    cab += b"\0" * (64 - len(cab))
    return cab + corpo


def registrar_dados(cat, id_, arquivo, dados, origem, extra):
    cat["assets"] = [x for x in cat["assets"] if x["id"] != id_]
    cat["assets"].append(dict({"id": id_, "arquivo": arquivo, "tipo": "dados", "bytes": len(dados), "crc32": "%08x" % zlib.crc32(dados),
                               "origem": origem, "autor": "Projeto Ventania / Projeto Brisa", "licenca": "Original",
                               "autorizacao": "titular (ASSET_LICENSES.md §2)", "categoria": "dados"}, **extra))


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
    # Bots (Etapa 7): data/bots.json + data/nomes_jogadores.txt.
    db = json.load(open(os.path.join(RAIZ, "data/bots.json"), encoding="utf-8"))
    nomes = [l.strip() for l in open(os.path.join(RAIZ, "data/nomes_jogadores.txt"), encoding="utf-8") if l.strip()]
    validar_bots(db, nomes, modelos)
    dados_b = compilar_bots(db, nomes)
    open(os.path.join(RAIZ, "assets/dados/bots.bdat"), "wb").write(dados_b)
    registrar_dados(cat, "dados_bots", "assets/dados/bots.bdat", dados_b,
                    f"data/bots.json + data/nomes_jogadores.txt (cópias de ventania:game/data/@{cat.get('ventania_commit', '')})",
                    {"dificuldades": len(db["dificuldades"]), "nomes": len(nomes)})
    cat["assets"].sort(key=lambda x: x["arquivo"])
    json.dump(cat, open(cat_p, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print(f"DADOS_OK itens.bdat {len(dados)} bytes ({len(d['armas'])} armas, {len(d['consumiveis'])} consumíveis); "
          f"bots.bdat {len(dados_b)} bytes ({len(db['dificuldades'])} dificuldades, {len(nomes)} nomes)")


if __name__ == "__main__":
    try:
        main()
    except ErroDados as e:
        print("[ERRO]", e)
        print("DADOS_RECUSADOS")
        sys.exit(1)
