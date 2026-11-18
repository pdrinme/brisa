"""Redução de polígonos por colapso de arestas com métricas de erro quádrico
(Garland & Heckbert, 1997), só com a biblioteca padrão.

Feito para o estilo do Ventania (formas grandes, cor chapada por face):
  - cada parte rígida é reduzida sozinha (nunca junta dois ossos);
  - bordas abertas e fronteiras entre cores diferentes recebem planos de
    restrição de peso alto, para a silhueta e as manchas de cor não "escorrerem";
  - um colapso que vira uma face ao contrário é rejeitado;
  - a cor de cada face sobrevivente é mantida; as normais viram planas
    (o sombreamento low-poly do kit).
"""
import heapq
import math

import mat

PESO_BORDA = 1000.0


def _plano(a, b, c):
    n = mat.vetorial(mat.sub(b, a), mat.sub(c, a))
    area2 = math.sqrt(sum(x * x for x in n))
    if area2 < 1e-12:
        return None, 0.0
    n = tuple(x / area2 for x in n)
    d = -sum(n[i] * a[i] for i in range(3))
    return (n[0], n[1], n[2], d), area2 * 0.5


def _quadrica(p, peso):
    a, b, c, d = p
    v = (a, b, c, d)
    return [peso * v[i] * v[j] for i in range(4) for j in range(4)]


def _somar(q1, q2):
    return [x + y for x, y in zip(q1, q2)]


def _erro(q, v):
    x = (v[0], v[1], v[2], 1.0)
    return sum(q[i * 4 + j] * x[i] * x[j] for i in range(4) for j in range(4))


def _otimo(q, a, b):
    # Resolve o sistema 3×3 do mínimo da quádrica; se mal condicionado, testa a, b e o meio.
    m = [[q[0], q[1], q[2]], [q[4], q[5], q[6]], [q[8], q[9], q[10]]]
    rhs = [-q[3], -q[7], -q[11]]
    det = (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
           + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]))
    cands = [a, b, tuple((x + y) / 2 for x, y in zip(a, b))]
    if abs(det) > 1e-9:
        def subs(col):
            mm = [row[:] for row in m]
            for r in range(3):
                mm[r][col] = rhs[r]
            return (mm[0][0] * (mm[1][1] * mm[2][2] - mm[1][2] * mm[2][1]) - mm[0][1] * (mm[1][0] * mm[2][2] - mm[1][2] * mm[2][0])
                    + mm[0][2] * (mm[1][0] * mm[2][1] - mm[1][1] * mm[2][0]))
        v = (subs(0) / det, subs(1) / det, subs(2) / det)
        # o ótimo não pode fugir muito da aresta (evita espetos)
        meio = cands[2]
        lim = 2.0 * math.dist(a, b) + 1e-6
        if math.dist(v, meio) <= lim:
            cands.insert(0, v)
    melhor = min(cands, key=lambda v: _erro(q, v))
    return melhor, _erro(q, melhor)


def reduzir(tris, alvo):
    """tris: [(ps(3), ns(3), cor)] → nova lista com ≤ alvo triângulos (se possível)."""
    if len(tris) <= alvo:
        return list(tris)
    # Solda por posição.
    vid = {}
    pos = []
    faces = []
    for ps, _, cor in tris:
        f = []
        for p in ps:
            k = (round(p[0], 5), round(p[1], 5), round(p[2], 5))
            if k not in vid:
                vid[k] = len(pos)
                pos.append(tuple(p))
            f.append(vid[k])
        if len(set(f)) == 3:
            faces.append([f, cor, True])
    nv = len(pos)
    q = [[0.0] * 16 for _ in range(nv)]
    faces_de = [set() for _ in range(nv)]
    for fi, (f, cor, _) in enumerate(faces):
        pl, area = _plano(*(pos[i] for i in f))
        if pl is None:
            continue
        qf = _quadrica(pl, max(area, 1e-6))
        for i in f:
            q[i] = _somar(q[i], qf)
            faces_de[i].add(fi)
    # Arestas: bordas abertas e fronteiras de cor ganham planos de restrição.
    arestas = {}
    for fi, (f, cor, _) in enumerate(faces):
        for e in ((f[0], f[1]), (f[1], f[2]), (f[2], f[0])):
            arestas.setdefault(tuple(sorted(e)), []).append(fi)
    for (a, b), fs in arestas.items():
        cores = {faces[fi][1] for fi in fs}
        if len(fs) == 1 or len(cores) > 1:
            for fi in fs:
                f = faces[fi][0]
                pl, _ = _plano(*(pos[i] for i in f))
                if pl is None:
                    continue
                n = pl[:3]
                e = mat.sub(pos[b], pos[a])
                c = mat.normalizar(mat.vetorial(e, n))
                d = -sum(c[i] * pos[a][i] for i in range(3))
                qb = _quadrica((c[0], c[1], c[2], d), PESO_BORDA * max(math.dist(pos[a], pos[b]) ** 2, 1e-6))
                q[a] = _somar(q[a], qb)
                q[b] = _somar(q[b], qb)
    vivo = [True] * nv
    versao = [0] * nv
    heap = []

    def empilhar(a, b):
        v, e = _otimo(_somar(q[a], q[b]), pos[a], pos[b])
        heapq.heappush(heap, (e, a, b, versao[a], versao[b], v))

    for a, b in arestas:
        empilhar(a, b)
    n_faces = len(faces)

    def normal(f, sub_de=None, novo=None):
        ps = [novo if (sub_de is not None and i == sub_de) else pos[i] for i in f]
        return mat.vetorial(mat.sub(ps[1], ps[0]), mat.sub(ps[2], ps[0]))

    while n_faces > alvo and heap:
        e, a, b, va, vb, v = heapq.heappop(heap)
        if not (vivo[a] and vivo[b]) or versao[a] != va or versao[b] != vb:
            continue
        # Rejeita se alguma face que sobrevive vira ao contrário.
        compartilhadas = faces_de[a] & faces_de[b]
        ok = True
        for vert in (a, b):
            for fi in faces_de[vert]:
                if fi in compartilhadas or not faces[fi][2]:
                    continue
                f = faces[fi][0]
                antes = normal(f)
                depois = normal(f, vert, v)
                if sum(x * y for x, y in zip(antes, depois)) <= 0:
                    ok = False
                    break
            if not ok:
                break
        if not ok:
            continue
        # Colapsa b em a.
        pos[a] = v
        q[a] = _somar(q[a], q[b])
        vivo[b] = False
        versao[a] += 1
        for fi in compartilhadas:
            if faces[fi][2]:
                faces[fi][2] = False
                n_faces -= 1
        for fi in faces_de[b]:
            if not faces[fi][2]:
                continue
            f = faces[fi][0]
            faces[fi][0] = [a if i == b else i for i in f]
            faces_de[a].add(fi)
        faces_de[a] = {fi for fi in faces_de[a] if faces[fi][2]}
        vizinhos = set()
        for fi in faces_de[a]:
            vizinhos.update(faces[fi][0])
        vizinhos.discard(a)
        for w in vizinhos:
            empilhar(a, w) if a < w else empilhar(w, a)
    out = []
    for f, cor, vivo_f in faces:
        if not vivo_f:
            continue
        ps = [pos[i] for i in f]
        n = mat.normalizar(mat.vetorial(mat.sub(ps[1], ps[0]), mat.sub(ps[2], ps[0])))
        if n == (0.0, 0.0, 0.0):
            continue
        out.append((ps, [n, n, n], cor))
    return out
