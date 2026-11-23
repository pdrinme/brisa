"""GLB do Ventania → modelo intermediário do Brisa (stdlib).

Um modelo é uma hierarquia de nós e uma lista de PARTES rígidas: cada parte é um
trecho da lista de triângulos que segue um único nó. A posição final de um
vértice é  mundo(nó) · inv_bind(nó) · p  (BRISA_ARCHITECTURE §8).
  - malha com pele (personagens): p = vértice da malha, inv_bind = a do glTF;
    o skinning do Ventania é rígido (1 osso por vértice), e o conversor exige isso;
  - malha presa a um nó animado (tampa do baú): p = vértice no espaço do modelo
    em repouso, inv_bind = inversa(mundo_repouso(nó));
  - o resto é estático (nó −1): p = vértice no espaço do modelo.
A cor vem da UV → célula da paleta de 40 cores (tools/assets/kit/paleta.json).
"""
import json
import math
import os
import sys

import mat

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(AQUI, "..", "auditoria"))
from glb import Glb  # noqa: E402

# Materiais especiais do Ventania (sem textura). O Brisa ainda não tem
# transparência no 3D: usa uma cor opaca da paleta (registrado no catálogo).
COR_MATERIAL_ESPECIAL = {"M_vidro_ventania": (0xA6, 0xEC, 0xFF), "M_agua_ventania": (0x4F, 0xB7, 0xE8)}


class ErroConversao(Exception):
    pass


def carregar_paleta(caminho):
    p = json.load(open(caminho, encoding="utf-8"))
    cores = [(int(h[1:3], 16), int(h[3:5], 16), int(h[5:7], 16)) for _, h in p["cores"]]
    return {"cores": cores, "nomes": [n for n, _ in p["cores"]], "atlas": p["atlas_px"], "celula": p["celula_px"]}


def cor_da_uv(paleta, u, v):
    por_linha = paleta["atlas"] // paleta["celula"]
    cx = min(max(int(u * por_linha), 0), por_linha - 1)
    cy = min(max(int(v * por_linha), 0), por_linha - 1)
    i = cy * por_linha + cx
    if i >= len(paleta["cores"]):
        raise ErroConversao(f"UV ({u:.3f}, {v:.3f}) cai na célula vazia {i} da paleta (magenta)")
    return paleta["cores"][i], i


class No:
    def __init__(self, nome, pai, t, r, s):
        self.nome, self.pai, self.t, self.r, self.s = nome, pai, t, r, s
        self.animado = False
        self.osso = False
        self.inv_bind = mat.identidade()
        self.mundo_repouso = mat.identidade()


class Modelo:
    def __init__(self):
        self.nos = []          # ordem: pais antes dos filhos
        self.partes = []       # {"no": i ou -1, "tris": [(p0,p1,p2,n0,n1,n2,cor)]}
        self.marcadores = []   # {"nome", "no", "pos"}
        self.colisoes = []     # (min, max) no espaço do modelo
        self.esqueletico = False
        self.celulas_usadas = set()

    def triangulos(self):
        return sum(len(p["tris"]) for p in self.partes)


def _trs_no(n):
    if "matrix" in n:
        raise ErroConversao("nó com 'matrix' não suportado (o exportador do Ventania usa TRS)")
    return (tuple(n.get("translation", (0, 0, 0))), tuple(n.get("rotation", (0, 0, 0, 1))), tuple(n.get("scale", (1, 1, 1))))


def converter(caminho_glb, paleta):
    g = Glb(caminho_glb)
    j = g.json
    nos_glb = j["nodes"]
    pai_de = {}
    for i, n in enumerate(nos_glb):
        for c in n.get("children", []):
            pai_de[c] = i
    colisao = {i for i, n in enumerate(nos_glb) if "colonly" in n.get("name", "")}
    # Ordem topológica a partir das raízes da cena, sem os nós de colisão.
    ordem = []
    def visitar(i):
        if i in colisao:
            return
        ordem.append(i)
        for c in nos_glb[i].get("children", []):
            visitar(c)
    for r in j["scenes"][j.get("scene", 0)]["nodes"]:
        visitar(r)
    idx = {g_i: k for k, g_i in enumerate(ordem)}
    m = Modelo()
    for gi in ordem:
        t, r, s = _trs_no(nos_glb[gi])
        pai = idx.get(pai_de.get(gi), -1)
        m.nos.append(No(nos_glb[gi].get("name", f"no{gi}"), pai, t, r, s))
    for no in m.nos:
        local = mat.trs(no.t, no.r, no.s)
        no.mundo_repouso = local if no.pai < 0 else mat.mul(m.nos[no.pai].mundo_repouso, local)
    for a in j.get("animations", []):
        for c in a["channels"]:
            gi = c["target"].get("node")
            if gi in idx:
                m.nos[idx[gi]].animado = True
    # um nó "segue animação" se ele ou um ancestral é animado
    def anima(k):
        while k >= 0:
            if m.nos[k].animado:
                return True
            k = m.nos[k].pai
        return False

    juntas = set()
    for sk in j.get("skins", []):
        ibm = g.accessor(sk["inverseBindMatrices"])
        for k, gi in enumerate(sk["joints"]):
            no = m.nos[idx[gi]]
            no.osso = True
            no.inv_bind = list(ibm[k])
            juntas.add(idx[gi])
    m.esqueletico = bool(juntas)

    partes = {}  # no -> lista de triângulos
    for gi in ordem:
        n = nos_glb[gi]
        if "mesh" not in n:
            continue
        k = idx[gi]
        pele = n.get("skin")
        for prim in j["meshes"][n["mesh"]]["primitives"]:
            if prim.get("mode", 4) != 4:
                raise ErroConversao("primitiva que não é lista de triângulos")
            at = prim["attributes"]
            pos = g.accessor(at["POSITION"])
            nrm = g.accessor(at["NORMAL"]) if "NORMAL" in at else None
            uv = g.accessor(at["TEXCOORD_0"]) if "TEXCOORD_0" in at else None
            nome_mat = j["materials"][prim["material"]].get("name", "") if "material" in prim else ""
            especial = COR_MATERIAL_ESPECIAL.get(nome_mat)
            ind = [i[0] for i in g.accessor(prim["indices"])] if "indices" in prim else list(range(len(pos)))
            ossos_v = None
            if pele is not None:
                jt = g.accessor(at["JOINTS_0"])
                wt = g.accessor(at["WEIGHTS_0"])
                juntas_pele = j["skins"][pele]["joints"]
                ossos_v = []
                for a, b in zip(jt, wt):
                    usados = [(w, a[q]) for q, w in enumerate(b) if w > 0.001]
                    if len(usados) != 1:
                        raise ErroConversao(f"vértice com {len(usados)} influências: o Brisa exige skinning rígido")
                    ossos_v.append(idx[juntas_pele[usados[0][1]]])
            if pele is None:
                dono = k if anima(k) else -1
                if dono >= 0:
                    m.nos[dono].inv_bind = mat.inversa(m.nos[dono].mundo_repouso)
                mw = m.nos[k].mundo_repouso
            for t0 in range(0, len(ind), 3):
                tri = ind[t0:t0 + 3]
                if pele is not None:
                    ossos_tri = {ossos_v[i] for i in tri}
                    if len(ossos_tri) != 1:
                        raise ErroConversao("triângulo dividido entre ossos: o Brisa exige partes rígidas")
                    dono = ossos_tri.pop()
                    ps = [tuple(pos[i]) for i in tri]
                    ns = [mat.normalizar(nrm[i]) if nrm else None for i in tri]
                else:
                    ps = [mat.ponto(mw, pos[i]) for i in tri]
                    ns = [mat.normalizar(mat.direcao(mw, nrm[i])) if nrm else None for i in tri]
                if ns[0] is None:
                    fn = mat.normalizar(mat.vetorial(mat.sub(ps[1], ps[0]), mat.sub(ps[2], ps[0])))
                    ns = [fn, fn, fn]
                if especial:
                    cor = especial
                elif uv:
                    u = sum(uv[i][0] for i in tri) / 3
                    v = sum(uv[i][1] for i in tri) / 3
                    cor, cel = cor_da_uv(paleta, u, v)
                    m.celulas_usadas.add(cel)
                else:
                    cor = (200, 200, 200)
                partes.setdefault(dono, []).append((ps, ns, cor))
    for no, tris in sorted(partes.items()):
        m.partes.append({"no": no, "tris": tris})
    # Marcadores: nós sem malha que não são ossos (grip_r, muzzle, alca_tampa...).
    for gi in ordem:
        n = nos_glb[gi]
        k = idx[gi]
        if "mesh" in n or m.nos[k].osso or "skin" in n or n.get("children"):
            continue
        m.marcadores.append({"nome": m.nos[k].nome, "no": m.nos[k].pai, "pos": m.nos[k].t})
    # Colisão: AABB de cada malha "-colonly" no espaço do modelo.
    for gi in sorted(colisao):
        n = nos_glb[gi]
        if "mesh" not in n:
            continue
        t, r, s = _trs_no(n)
        mw = mat.trs(t, r, s)
        if gi in pai_de and pai_de[gi] in idx:
            mw = mat.mul(m.nos[idx[pai_de[gi]]].mundo_repouso, mw)
        pts = []
        for prim in j["meshes"][n["mesh"]]["primitives"]:
            pts += [mat.ponto(mw, p) for p in g.accessor(prim["attributes"]["POSITION"])]
        m.colisoes += decompor_colisao(pts)
    return m


def classificar_colisao(pts, tol=0.01):
    """Convexo de colisão → (min, max, tipo, espessura).
    tipo 0 = caixa; 1..4 = rampa (laje inclinada) que sobe para +x, −x, +z, −z.
    Outros convexos (telhado em pirâmide, escada em cunha) viram a caixa envolvente."""
    mn = tuple(min(p[q] for p in pts) for q in range(3))
    mx = tuple(max(p[q] for p in pts) for q in range(3))
    unicos = {tuple(round(c, 3) for c in p) for p in pts}
    def eh_canto(p):
        return all(abs(p[q] - mn[q]) < tol or abs(p[q] - mx[q]) < tol for q in range(3))
    if all(eh_canto(p) for p in unicos):
        return (mn, mx, 0, 0.0)
    if len(unicos) == 8:
        for eixo, (sinal_tipo) in ((0, (1, 2)), (2, (3, 4))):
            outro = 2 - eixo
            # Em cada ponta do eixo, as alturas (topo e base) são as mesmas nos dois lados do outro eixo.
            pontas = {}
            for p in unicos:
                if abs(p[eixo] - mn[eixo]) < 0.25:
                    pontas.setdefault("min", []).append(p)
                elif abs(p[eixo] - mx[eixo]) < 0.25:
                    pontas.setdefault("max", []).append(p)
            if len(pontas.get("min", [])) != 4 or len(pontas.get("max", [])) != 4:
                continue
            ys_min = sorted(p[1] for p in pontas["min"])
            ys_max = sorted(p[1] for p in pontas["max"])
            if abs(ys_min[0] - ys_min[1]) > tol or abs(ys_min[2] - ys_min[3]) > tol:
                continue
            if abs(ys_max[0] - ys_max[1]) > tol or abs(ys_max[2] - ys_max[3]) > tol:
                continue
            esp_a, esp_b = ys_min[2] - ys_min[0], ys_max[2] - ys_max[0]
            if abs(esp_a - esp_b) > tol or abs(ys_max[2] - ys_min[2]) < 0.2:
                continue
            sobe_para_max = ys_max[2] > ys_min[2]
            tipo = sinal_tipo[0] if sobe_para_max else sinal_tipo[1]
            return (mn, mx, tipo, round(esp_a, 4))
    return (mn, mx, 0, 0.0)


def decompor_colisao(pts, tol=0.01):
    """Convexo de colisão → lista de (min, max, tipo, espessura). Caixas e rampas
    simples saem de classificar_colisao; um prisma extrudado (perfil convexo num
    plano vertical, como os lances das escadas do Ventania em gerar_interiores.py)
    é cortado nos vértices do perfil em caixas, rampas (lajes) e cunhas (tipos
    5–8: topo inclinado com a base plana). O resto vira a caixa envolvente."""
    r = classificar_colisao(pts, tol)
    unicos = {tuple(round(c, 3) for c in p) for p in pts}
    mn, mx = r[0], r[1]
    if r[2] != 0 or all(all(abs(p[q] - mn[q]) < tol or abs(p[q] - mx[q]) < tol for q in range(3)) for p in unicos):
        return [r]
    pedacos = _prisma(unicos, tol)
    return pedacos if pedacos else [r]


def _cadeias(perfil):
    """Perfil convexo (u, y) → (cadeia de cima, cadeia de baixo), de u mínimo a máximo."""
    pts = sorted(set(perfil))
    def vira(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    baixo, cima = [], []
    for p in pts:
        while len(baixo) >= 2 and vira(baixo[-2], baixo[-1], p) <= 1e-9:
            baixo.pop()
        baixo.append(p)
    for p in reversed(pts):
        while len(cima) >= 2 and vira(cima[-2], cima[-1], p) <= 1e-9:
            cima.pop()
        cima.append(p)
    cima.reverse()
    # pontos com o mesmo u nas pontas: a cadeia de cima pega o mais alto, a de baixo o mais baixo
    return cima, baixo


def _em(cadeia, u, maior):
    """Altura da cadeia em u (interpolada); nas verticais, a mais alta (cima) ou a mais baixa (baixo)."""
    ys = []
    for a, b in zip(cadeia, cadeia[1:]):
        if a[0] - 1e-6 <= u <= b[0] + 1e-6:
            if abs(b[0] - a[0]) < 1e-9:
                ys += [a[1], b[1]]
            else:
                ys.append(a[1] + (b[1] - a[1]) * (u - a[0]) / (b[0] - a[0]))
    if not ys:
        ys = [p[1] for p in cadeia if abs(p[0] - u) < 1e-6] or [cadeia[0][1]]
    return max(ys) if maior else min(ys)


def _prisma(unicos, tol):
    for e, u_eixo, tipos in ((2, 0, (1, 2)), (0, 2, (3, 4))):  # e: eixo da extrusão; u: eixo do perfil
        lados = sorted({round(p[e], 2) for p in unicos})
        if len(lados) != 2:
            continue
        perfis = [sorted({(round(p[u_eixo], 3), round(p[1], 3)) for p in unicos if abs(round(p[e], 2) - lado) < 1e-6}) for lado in lados]
        if perfis[0] != perfis[1] or len(perfis[0]) < 3:
            continue
        cima, baixo = _cadeias(perfis[0])
        us = sorted({p[0] for p in cima} | {p[0] for p in baixo})
        out = []
        for ua, ub in zip(us, us[1:]):
            if ub - ua < 0.02:
                continue
            ta, tb = _em(cima, ua + 1e-4, True), _em(cima, ub - 1e-4, True)
            ba, bb = _em(baixo, ua + 1e-4, False), _em(baixo, ub - 1e-4, False)
            y0, y1 = min(ba, bb), max(ta, tb)
            lo = [0.0, y0, 0.0]
            hi = [0.0, y1, 0.0]
            lo[u_eixo], hi[u_eixo] = ua, ub
            lo[e], hi[e] = lados[0], lados[1]
            sobe = tb > ta
            if abs(tb - ta) < tol and abs(bb - ba) < tol:
                out.append((tuple(lo), tuple(hi), 0, 0.0))
            elif abs(bb - ba) < tol:
                # cunha: base plana em y0, topo de min(ta, tb) a max(ta, tb)
                out.append((tuple(lo), tuple(hi), (tipos[0] if sobe else tipos[1]) + 4, round(min(ta, tb) - y0, 4)))
            elif abs(tb - ta) >= tol:
                # laje inclinada (rampa): o topo é exato; a espessura é a da ponta baixa
                # (numa laje de espessura variável, a base fica um pouco mais alta na outra ponta)
                t_baixo, b_baixo = (ta, ba) if sobe else (tb, bb)
                lo[1] = b_baixo
                out.append((tuple(lo), tuple(hi), tipos[0] if sobe else tipos[1], round(t_baixo - b_baixo, 4)))
            else:
                out.append((tuple(lo), tuple(hi), 0, 0.0))  # topo plano: caixa
        if out:
            return out
    return None


def animacoes(caminho_glb, modelo, fps=30):
    """Amostra as animações a `fps` quadros por segundo. Devolve
    [{"nome", "loop", "quadros": [[(q, t) por nó] por quadro]}]."""
    g = Glb(caminho_glb)
    j = g.json
    nomes = {n.get("name"): k for k, n in enumerate(j["nodes"])}
    por_nome = {no.nome: k for k, no in enumerate(modelo.nos)}
    clipes = []
    for a in j.get("animations", []):
        canais = {}
        dur = 0.0
        for c in a["channels"]:
            gi = c["target"].get("node")
            nome = j["nodes"][gi].get("name")
            if nome not in por_nome:
                continue
            s = a["samplers"][c["sampler"]]
            if s.get("interpolation", "LINEAR") not in ("LINEAR", "STEP"):
                raise ErroConversao(f"interpolação {s.get('interpolation')} não suportada")
            ts = [x[0] for x in g.accessor(s["input"])]
            vs = [tuple(x) for x in g.accessor(s["output"])]
            canais[(por_nome[nome], c["target"]["path"])] = (ts, vs, s.get("interpolation", "LINEAR"))
            dur = max(dur, ts[-1])
        n_quadros = max(1, int(round(dur * fps)) + 1)
        quadros = []
        for f in range(n_quadros):
            t = f / fps
            linha = []
            for k, no in enumerate(modelo.nos):
                q = _amostrar(canais.get((k, "rotation")), t, no.r, True)
                p = _amostrar(canais.get((k, "translation")), t, no.t, False)
                linha.append((q, p))
            quadros.append(linha)
        nome = a.get("name", f"anim{len(clipes)}")
        loop = nome.endswith("-loop")
        clipes.append({"nome": nome[:-5] if loop else nome, "loop": loop, "quadros": quadros})
    return clipes


def _amostrar(canal, t, padrao, rotacao):
    if canal is None:
        return tuple(padrao)
    ts, vs, interp = canal
    if t <= ts[0]:
        return vs[0]
    if t >= ts[-1]:
        return vs[-1]
    i = 0
    while ts[i + 1] < t:
        i += 1
    if interp == "STEP":
        return vs[i]
    u = (t - ts[i]) / (ts[i + 1] - ts[i])
    if rotacao:
        return mat.slerp(vs[i], vs[i + 1], u)
    return tuple(a + (b - a) * u for a, b in zip(vs[i], vs[i + 1]))
