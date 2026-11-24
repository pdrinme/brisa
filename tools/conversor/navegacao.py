"""Grafo de navegação dos bots (Etapa 7): porta da construção do NavGrade do
Ventania (game/gameplay/ai/nav_grade.gd), calculada offline a partir do layout.

Nós:
  - CHÃO: 150 × 150 células de 4 m (bloqueadas: mar/praia rasa, obstáculos grandes,
    o cômodo térreo das escadas);
  - ANDARES: uma por peça de PISO acima do relevo (> 2,5 m), por nível.
Arestas do chão: 8 vizinhos; parede = aresta bloqueada (a porta, pelo ladrilho, abre);
desnível > 3,4 m bloqueia. Andares: 4 vizinhos sem parede. Ligações entre níveis com
pontos de passagem exatos: escadas em U e rampas do mapa.
A busca (A*) roda no jogo (src/mundo/navegacao.cpp). Formato: docs/FORMATOS.md (BNAV).
"""
import math
import struct

import formatos

T = 4.0
LIMITE = 300.0
N = 150
ALTURA_ANDAR = 2.5
DESNIVEL_MAX = 3.4
L_N, L_L, L_S, L_O = range(4)
DIRS = [(0, -1), (1, 0), (0, 1), (-1, 0)]
ROTA_ESCADA = [(1.5, 0.1, 1.0), (-1.25, 2.1, 1.0), (-1.25, 2.1, -1.0), (-0.3, 2.1, -1.0), (2.0, 4.1, -1.0)]
VERSAO_NAV = 1


def centro(i, j):
    return (-LIMITE + (i + 0.5) * T, -LIMITE + (j + 0.5) * T)


class Nav:
    def __init__(self, layout, relevo):
        self.relevo = relevo
        costa = layout["costa"]
        self.costa = costa
        nn = N * N
        self.bloq = bytearray(nn)
        self.arestas = bytearray(nn)
        self.portas = bytearray(nn)
        self.altura = [0.0] * nn
        self.sup = {}            # (i, j, nivel) -> id
        self.sup_pos = []
        self.sup_cel = []
        self.sup_paredes = set()
        self.ligacoes = {}       # id -> [(id2, custo, [pontos])]
        self.rampas_pendentes = {}
        self.escadas_pendentes = []
        self.escadas = 0
        for j in range(N):
            for i in range(N):
                cx, cz = centro(i, j)
                h = relevo.altura(cx, cz)
                self.altura[j * N + i] = h
                if h < 0.2 or self.inland(cx, cz) < 3.0:
                    self.bloq[j * N + i] = 1
        for j in range(N):
            for i in range(N):
                if i + 1 < N and abs(self.altura[j * N + i] - self.altura[j * N + i + 1]) > DESNIVEL_MAX:
                    self.bloquear_aresta(i, j, L_L)
                if j + 1 < N and abs(self.altura[j * N + i] - self.altura[(j + 1) * N + i]) > DESNIVEL_MAX:
                    self.bloquear_aresta(i, j, L_S)
        for o in layout["obstaculos_nav"]:
            if len(o) == 5:
                self.obstaculo_ret((o[0], o[1]), (o[2], o[3]), o[4])
            else:
                self.obstaculo((o[0], o[1]), o[2])
        for e in layout["escadas"]:
            self.add_stair(tuple(e["p"]), float(e["rot"]))
        # pisos e paredes primeiro (as rampas e escadas se ligam a eles)
        for p in layout["pecas"]:
            if p["t"] != 3:
                self.set_piece(p["t"], p["c"], p["r"], p["mask"], True)
        for p in layout["pecas"]:
            if p["t"] == 3:
                self.set_piece(p["t"], p["c"], p["r"], p["mask"], True)
        self.resolve_stairs()

    def inland(self, x, z):
        n = len(self.costa)
        a = math.atan2(z, x)
        f = (a / (2 * math.pi) * n) % n
        k = int(f)
        u = f - k
        r = self.costa[k] + (self.costa[(k + 1) % n] - self.costa[k]) * u
        return r - math.hypot(x, z)

    def cell_of(self, x, z):
        return (min(max(int(math.floor((x + LIMITE) / T)), 0), N - 1), min(max(int(math.floor((z + LIMITE) / T)), 0), N - 1))

    def obstaculo(self, p, raio):
        a = self.cell_of(p[0] - raio, p[1] - raio)
        b = self.cell_of(p[0] + raio, p[1] + raio)
        for j in range(a[1], b[1] + 1):
            for i in range(a[0], b[0] + 1):
                cx, cz = centro(i, j)
                if math.hypot(cx - p[0], cz - p[1]) < raio + 0.6:
                    self.bloq[j * N + i] = 1

    def obstaculo_ret(self, p, meio, ang):
        r = math.hypot(*meio) + 1.0
        a = self.cell_of(p[0] - r, p[1] - r)
        b = self.cell_of(p[0] + r, p[1] + r)
        c, s = math.cos(-ang), math.sin(-ang)
        for j in range(a[1], b[1] + 1):
            for i in range(a[0], b[0] + 1):
                cx, cz = centro(i, j)
                dx, dz = cx - p[0], cz - p[1]
                lx, lz = dx * c - dz * s, dx * s + dz * c   # Vector2.rotated(-ang)
                if abs(lx) < meio[0] + 0.45 and abs(lz) < meio[1] + 0.45:
                    self.bloq[j * N + i] = 1

    def bloquear_aresta(self, i, j, lado, liberar=False, portas=False):
        vx, vz = DIRS[lado]
        oposto = (lado + 2) % 4
        k = j * N + i
        ni, nj = i + vx, j + vz
        dentro = 0 <= ni < N and 0 <= nj < N
        arr = self.portas if portas else self.arestas
        if portas:
            liberar_bit = liberar
        else:
            liberar_bit = liberar
        if liberar_bit:
            arr[k] &= ~(1 << lado) & 0xFF
            if dentro:
                arr[nj * N + ni] &= ~(1 << oposto) & 0xFF
        else:
            arr[k] |= 1 << lado
            if dentro:
                arr[nj * N + ni] |= 1 << oposto

    def set_piece(self, tipo, cell, rot, mask, existe):
        i = cell[0] + N // 2
        j = cell[2] + N // 2
        if not (0 <= i < N and 0 <= j < N):
            return
        if tipo in (0, 1):
            lado = L_N if tipo == 0 else L_O
            porta = mask & (1 << 1) == 0
            aberta = (not existe) or porta
            base = self.relevo.altura(cell[0] * T + 2.0, cell[2] * T + 2.0)
            if abs(cell[1] * T - base) <= ALTURA_ANDAR:
                self.bloquear_aresta(i, j, lado, aberta)
                self.bloquear_aresta(i, j, lado, not (existe and porta), True)
            else:
                vx, vz = DIRS[lado]
                k1 = (i, j, cell[1], lado)
                k2 = (i + vx, j + vz, cell[1], (lado + 2) % 4)
                if aberta:
                    self.sup_paredes.discard(k1)
                    self.sup_paredes.discard(k2)
                else:
                    self.sup_paredes.add(k1)
                    self.sup_paredes.add(k2)
        elif tipo == 2:
            topo = cell[1] * T + 0.1
            if topo - self.relevo.altura(cell[0] * T + 2.0, cell[2] * T + 2.0) <= ALTURA_ANDAR:
                return
            chave = (i, j, cell[1])
            if existe and chave not in self.sup:
                self.sup[chave] = N * N + len(self.sup_pos)
                self.sup_pos.append((cell[0] * T + 2.0, topo, cell[2] * T + 2.0))
                self.sup_cel.append(chave)
                for rc in list(self.rampas_pendentes):
                    self.rampa(rc, self.rampas_pendentes[rc])
        elif tipo == 3:
            self.rampa(tuple(cell), rot)

    def rampa(self, cell, rot):
        d = [(0, -1), (-1, 0), (0, 1), (1, 0)][rot % 4]
        baixo = (cell[0] - d[0], cell[1], cell[2] - d[1])
        alto = (cell[0] + d[0], cell[1] + 1, cell[2] + d[1])
        a = self.no_em_celula(baixo)
        b = self.no_em_celula(alto)
        if a < 0 or b < 0:
            self.rampas_pendentes[cell] = rot
            return
        self.rampas_pendentes.pop(cell, None)
        c = (cell[0] * T + 2.0, cell[1] * T, cell[2] * T + 2.0)
        pontos = [(c[0] - d[0] * 2.0, c[1] + 0.1, c[2] - d[1] * 2.0), (c[0], c[1] + 2.1, c[2]),
                  (c[0] + d[0] * 2.0, c[1] + 4.1, c[2] + d[1] * 2.0)]
        self.ligar(a, b, 1.6, pontos)

    def no_em_celula(self, cel):
        i = cel[0] + N // 2
        j = cel[2] + N // 2
        if not (0 <= i < N and 0 <= j < N):
            return -1
        if cel[1] * T + 0.1 - self.altura[j * N + i] <= ALTURA_ANDAR:
            return -1 if self.bloq[j * N + i] else j * N + i
        return self.sup.get((i, j, cel[1]), -1)

    def add_stair(self, base, rot):
        a_r = math.radians(rot)
        ca, sa = math.cos(a_r), math.sin(a_r)
        cel = (int(math.floor(base[0] / T)), int(round(base[1] / T)), int(math.floor(base[2] / T)))
        dx = 1 if abs(rot) < 1.0 else -1
        i = cel[0] + N // 2
        j = cel[2] + N // 2
        if base[1] + 0.1 - self.altura[j * N + i] <= ALTURA_ANDAR:
            self.bloq[j * N + i] = 1
        a = self.no_em_celula((cel[0] + dx, cel[1], cel[2]))
        b = self.no_em_celula((cel[0] + dx, cel[1] + 1, cel[2]))
        if a < 0 or b < 0:
            self.escadas_pendentes.append((base, rot))
            return
        # Basis(UP, rot) · r
        pontos = [(base[0] + ca * r[0] + sa * r[2], base[1] + r[1], base[2] - sa * r[0] + ca * r[2]) for r in ROTA_ESCADA]
        self.ligar(a, b, 3.0, pontos)
        self.escadas += 1

    def resolve_stairs(self):
        p = self.escadas_pendentes
        self.escadas_pendentes = []
        for base, rot in p:
            self.add_stair(base, rot)

    def ligar(self, a, b, custo, pontos):
        self.ligacoes.setdefault(a, []).append((b, custo, list(pontos)))
        self.ligacoes.setdefault(b, []).append((a, custo, list(reversed(pontos))))

    def vizinhos_andar(self, idx):
        i, j, nivel = self.sup_cel[idx]
        out = []
        for lado in range(4):
            if (i, j, nivel, lado) in self.sup_paredes:
                out.append(-1)
                continue
            vx, vz = DIRS[lado]
            out.append(self.sup.get((i + vx, j + vz, nivel), -1))
        return out

    def escrever(self, comodos):
        nn = N * N
        b_alt = struct.pack(f"<{nn}h", *[max(-32767, min(32767, int(round(h * 100)))) for h in self.altura])
        b_bloq = bytes(self.bloq)
        b_are = bytes(self.arestas)
        b_por = bytes(self.portas)
        b_and = b""
        for k, (pos, cel) in enumerate(zip(self.sup_pos, self.sup_cel)):
            b_and += struct.pack("<3fhhhh4i", *pos, cel[0], cel[1], cel[2], 0, *self.vizinhos_andar(k))
        ligs, pontos = [], []
        for a in sorted(self.ligacoes):
            for b, custo, pts in self.ligacoes[a]:
                ligs.append(struct.pack("<iifHH", a, b, custo, len(pontos), len(pts)))
                pontos += pts
        b_lig = b"".join(ligs)
        b_pts = b"".join(struct.pack("<3f", *p) for p in pontos)
        b_com = b"".join(struct.pack("<3f4x", *c) for c in comodos)
        blocos = [b_alt, b_bloq, b_are, b_por, b_and, b_lig, b_pts, b_com]
        corpo, offs, off = b"", [], 96
        for b in blocos:
            b = formatos._alinhar(b)
            offs.append(off)
            corpo += b
            off += len(b)
        cab = b"BNAV" + struct.pack("<IIffIIII", VERSAO_NAV, N, T, LIMITE, len(self.sup_pos), len(ligs), len(pontos), len(comodos))
        cab += struct.pack("<8I", *offs)
        cab += b"\0" * (96 - len(cab))
        assert len(cab) == 96
        return cab + corpo


def construir(layout, relevo):
    nav = Nav(layout, relevo)
    dados = nav.escrever(layout["comodos"])
    info = {"andares": len(nav.sup_pos), "ligacoes": sum(len(v) for v in nav.ligacoes.values()) // 2, "escadas": nav.escadas,
            "bloqueadas": sum(nav.bloq), "pendentes": len(nav.escadas_pendentes) + len(nav.rampas_pendentes)}
    return dados, info
