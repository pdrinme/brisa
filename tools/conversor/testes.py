#!/usr/bin/env python3
"""Testes do pipeline de conversão (stdlib unittest).
Uso: python3 tools/conversor/testes.py [--ventania ../ventania]"""
import math
import os
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import decimar  # noqa: E402
import formatos  # noqa: E402
import mat  # noqa: E402
import modelo  # noqa: E402
import textura  # noqa: E402

RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
VENTANIA = os.environ.get("VENTANIA", os.path.join(RAIZ, "..", "ventania"))


def esfera(sub=2):
    t = (1 + 5 ** 0.5) / 2
    v = [mat.normalizar(p) for p in [(-1, t, 0), (1, t, 0), (-1, -t, 0), (1, -t, 0), (0, -1, t), (0, 1, t), (0, -1, -t),
                                     (0, 1, -t), (t, 0, -1), (t, 0, 1), (-t, 0, -1), (-t, 0, 1)]]
    f = [(0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11), (1, 5, 9), (5, 11, 4), (11, 10, 2), (10, 7, 6), (7, 1, 8),
         (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9), (4, 9, 5), (2, 4, 11), (6, 2, 10), (8, 6, 7), (9, 8, 1)]
    tris = [[v[a], v[b], v[c]] for a, b, c in f]
    for _ in range(sub):
        novo = []
        for a, b, c in tris:
            ab, bc, ca = (mat.normalizar(tuple((x + y) / 2 for x, y in zip(p, q))) for p, q in ((a, b), (b, c), (c, a)))
            novo += [[a, ab, ca], [ab, b, bc], [ca, bc, c], [ab, bc, ca]]
        tris = novo
    out = []
    for ps in tris:
        n = mat.normalizar(mat.vetorial(mat.sub(ps[1], ps[0]), mat.sub(ps[2], ps[0])))
        cor = (255, 0, 0) if ps[0][1] > 0 else (0, 0, 255)
        out.append((ps, [n] * 3, cor))
    return out


class TesteMat(unittest.TestCase):
    def test_trs_e_inversa(self):
        m = mat.trs((1, 2, 3), mat.normalizar((0.2, 0.5, -0.1, 0.8)), (2, 2, 2))
        i = mat.mul(mat.inversa(m), m)
        for k in range(16):
            self.assertAlmostEqual(i[k], 1.0 if k % 5 == 0 else 0.0, places=6)

    def test_slerp(self):
        q = mat.slerp((0, 0, 0, 1), mat.normalizar((0, 1, 0, 1)), 0.5)
        self.assertAlmostEqual(sum(x * x for x in q), 1.0, places=6)


class TesteDecimar(unittest.TestCase):
    def test_reduz_ate_o_alvo_sem_virar_faces(self):
        tris = esfera(2)  # 320
        out = decimar.reduzir(tris, 80)
        self.assertLessEqual(len(out), 84)
        self.assertGreaterEqual(len(out), 40)
        for ps, ns, _ in out:  # normais para fora (centro na origem)
            c = tuple(sum(p[i] for p in ps) / 3 for i in range(3))
            self.assertGreater(sum(a * b for a, b in zip(ns[0], c)), 0)

    def test_mantem_as_duas_cores(self):
        out = decimar.reduzir(esfera(2), 60)
        self.assertEqual({c for _, _, c in out}, {(255, 0, 0), (0, 0, 255)})

    def test_nao_mexe_abaixo_do_alvo(self):
        tris = esfera(0)
        self.assertEqual(len(decimar.reduzir(tris, 100)), 20)


class TesteFormatos(unittest.TestCase):
    def test_csm1_e_involucao(self):
        c = [(i, 0, 0, 128) for i in range(256)]
        s = formatos.clut_csm1(c)
        self.assertEqual(s[8][0], 16)
        self.assertEqual(s[16][0], 8)
        self.assertEqual(formatos.clut_csm1(s), c)

    def test_pak_ordenado_alinhado_e_com_crc(self):
        pak = formatos.escrever_pak([("b.bin", b"xyz", "TEST"), ("a.bin", b"12345", "TEST")], 7)
        self.assertEqual(pak[:4], b"BPAK")
        ver, vc, n, off, total, crc = struct.unpack_from("<6I", pak, 4)
        self.assertEqual((ver, vc, n, total), (1, 7, 2, len(pak)))
        idx = pak[off:off + 64 * n]
        self.assertEqual(zlib.crc32(idx), crc)
        nomes = [idx[i * 64:i * 64 + 44].split(b"\0")[0] for i in range(n)]
        self.assertEqual(nomes, [b"a.bin", b"b.bin"])
        for i in range(n):
            o, t, c = struct.unpack_from("<III", idx, i * 64 + 44)
            self.assertEqual(o % 64, 0)
            self.assertEqual(zlib.crc32(pak[o:o + t]), c)

    def test_textura_t4_empacota_dois_pixels_por_byte(self):
        b = formatos.escrever_textura(4, 2, 0x14, [1, 2, 3, 4, 5, 6, 7, 8], [(0, 0, 0, 128)] * 9)
        _, larg, alt, psm, _, n, off_pix, off_clut, tam = struct.unpack_from("<IHHBBHIII", b, 4)
        self.assertEqual((larg, alt, psm, n, tam), (4, 2, 0x14, 16, 4))
        self.assertEqual(b[off_pix], 0x21)


class TesteTextura(unittest.TestCase):
    def test_blob_mais_escuro_no_centro(self):
        px = textura.sombra_blob(32)
        self.assertGreater(px[16 * 32 + 16][3], 100)
        self.assertEqual(px[0][3], 0)

    def test_quantizar_limita_cores(self):
        px = [(x, y, (x * y) % 256, 255) for y in range(32) for x in range(32)]
        pal, ind = textura.quantizar(px, 16)
        self.assertLessEqual(len(pal), 16)
        self.assertEqual(len(ind), len(px))


@unittest.skipUnless(os.path.isdir(VENTANIA), "Ventania não encontrado")
class TesteVentania(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pal = modelo.carregar_paleta(os.path.join(VENTANIA, "tools/assets/kit/paleta.json"))

    def glb(self, rel):
        return os.path.join(VENTANIA, "game/assets", rel)

    def test_personagem_rigido_com_cores_da_paleta(self):
        m = modelo.converter(self.glb("personagens/recruta_base_01.glb"), self.pal)
        self.assertTrue(m.esqueletico)
        self.assertEqual(m.triangulos(), 2790)
        self.assertIn(self.pal["nomes"].index("coral"), m.celulas_usadas)  # cachecol coral do Recruta
        self.assertTrue(all(p["no"] >= 0 for p in m.partes))
        clipes = modelo.animacoes(self.glb("personagens/recruta_base_01.glb"), m)
        self.assertEqual(len(clipes), 16)
        idle = next(c for c in clipes if c["nome"] == "idle")
        self.assertTrue(idle["loop"])
        for q, _ in idle["quadros"][10]:
            self.assertAlmostEqual(sum(x * x for x in q), 1.0, places=4)

    def test_bau_tampa_animada_marcador_e_colisao(self):
        m = modelo.converter(self.glb("baus_coletaveis/bau_ventania_01.glb"), self.pal)
        self.assertEqual(len(m.partes), 2)
        self.assertEqual([k["nome"] for k in m.marcadores], ["alca_tampa"])
        (mn, mx, tipo, _esp), = m.colisoes
        self.assertEqual(tipo, 0)  # o baú é caixa
        self.assertAlmostEqual(mx[0] - mn[0], 1.1, places=3)  # 1,1 m (guia de estilo)

    def test_picareta_marcadores(self):
        m = modelo.converter(self.glb("armas/picareta_01.glb"), self.pal)
        self.assertEqual(sorted(k["nome"] for k in m.marcadores), ["cabeca", "centro_cabeca", "grip_l", "grip_r"])

    def test_partes_alinhadas_a_4_vertices(self):
        m = modelo.converter(self.glb("personagens/capitao_mare_01.glb"), self.pal)
        b = formatos.escrever_modelo(m)
        n_partes, off_partes = struct.unpack_from("<I", b, 12)[0], struct.unpack_from("<I", b, 80)[0]
        for i in range(n_partes):
            no, ini, n = struct.unpack_from("<iII", b, off_partes + 32 * i)
            self.assertEqual(ini % 4, 0)
            self.assertEqual(n % 3, 0)


class TesteColisao(unittest.TestCase):
    """BMOD v2: caixas e rampas a partir dos convexos -colonly."""

    def caixa(self, mn, mx):
        return [(x, y, z) for x in (mn[0], mx[0]) for y in (mn[1], mx[1]) for z in (mn[2], mx[2])]

    def test_caixa(self):
        mn, mx, tipo, esp = modelo.classificar_colisao(self.caixa((-2, 0, -0.12), (2, 4, 0.12)))
        self.assertEqual((tipo, esp), (0, 0.0))
        self.assertEqual((mn, mx), ((-2, 0, -0.12), (2, 4, 0.12)))

    def test_rampa_nos_quatro_sentidos(self):
        # Laje de 0,15 m que sobe 4 m ao longo de 4 m.
        def laje(eixo, sobe_para_max):
            pts = []
            for a in (-2, 2):
                for b in (-2, 2):
                    alto = (b > 0) == sobe_para_max
                    base = 3.85 if alto else 0.0
                    for y in (base, base + 0.15):
                        pts.append((a, y, b) if eixo == 2 else (b, y, a))
            return pts
        for eixo, sobe, esperado in ((0, True, 1), (0, False, 2), (2, True, 3), (2, False, 4)):
            mn, mx, tipo, esp = modelo.classificar_colisao(laje(eixo, sobe))
            self.assertEqual(tipo, esperado)
            self.assertAlmostEqual(esp, 0.15, places=3)
            self.assertAlmostEqual(mx[1], 4.0, places=3)

    def test_cunha_vira_caixa(self):
        # Prisma triangular (telhado/escada em cunha): fica a AABB.
        pts = [(-2, 0, -2), (2, 0, -2), (-2, 0, 2), (2, 0, 2), (-2, 2, 0), (2, 2, 0)]
        mn, mx, tipo, esp = modelo.classificar_colisao(pts)
        self.assertEqual(tipo, 0)
        self.assertEqual((mn, mx), ((-2, 0, -2), (2, 2, 2)))

    def test_escada_em_u_vira_cunha_patamar_e_laje(self):
        # Lance 1 + patamar num prisma só (gerar_interiores.py do Ventania): sobe de x = 2 até -0,665.
        pts = []
        for z in (-0.05, 1.95):
            pts += [(2.0, 0.1, z), (-0.665, 2.1, z), (-1.95, 2.1, z), (-1.95, 0.1, z)]
        pedacos = modelo.decompor_colisao(pts)
        tipos = sorted(p[2] for p in pedacos)
        self.assertEqual(tipos, [0, 6])  # patamar (caixa) + cunha subindo para −x
        cunha = [p for p in pedacos if p[2] == 6][0]
        self.assertAlmostEqual(cunha[0][0], -0.665, places=3)
        self.assertAlmostEqual(cunha[1][0], 2.0, places=3)
        self.assertAlmostEqual(cunha[1][1], 2.1, places=3)
        # Lance 2: laje inclinada com aba plana no topo
        pts = []
        for z in (-1.95, -0.05):
            pts += [(-0.665, 1.85, z), (-0.665, 2.1, z), (2.0, 4.1, z), (2.3, 4.1, z), (2.3, 3.85, z)]
        pedacos = modelo.decompor_colisao(pts)
        self.assertEqual(sorted(p[2] for p in pedacos), [0, 1])  # laje subindo para +x + aba
        laje = [p for p in pedacos if p[2] == 1][0]
        self.assertAlmostEqual(laje[3], 0.25, places=3)
        self.assertAlmostEqual(laje[0][1] + laje[3], 2.1, places=3)  # topo na ponta baixa

    def test_rampa_do_ventania_no_pacote(self):
        caminho = os.path.join(RAIZ, "assets", "modelos", "rampa_pedra.bmod")
        d = open(caminho, "rb").read()
        self.assertEqual(d[:4], b"BMOD")
        versao, n_col = struct.unpack_from("<I", d, 4)[0], struct.unpack_from("<I", d, 24)[0]
        off_col = struct.unpack_from("<7I", d, 80)[3]
        self.assertEqual(versao, formatos.VERSAO_MODELO)
        self.assertEqual(n_col, 1)
        x0, y0, z0, tipo, x1, y1, z1, esp = struct.unpack_from("<3fI3ff", d, off_col)
        self.assertEqual(tipo, 4)  # sobe para −z, como no campo de treino
        self.assertAlmostEqual(esp, 0.141, places=2)
        self.assertAlmostEqual(y1, 4.1, places=1)

    def test_posicoes_int16_dentro_da_aabb(self):
        # v3: posições int16 × escala do cabeçalho; erro de no máximo meia unidade.
        for nome in ("rampa_pedra", "recruta_base_01", "pinheiro_01"):
            d = open(os.path.join(RAIZ, "assets", "modelos", nome + ".bmod"), "rb").read()
            nv = struct.unpack_from("<I", d, 8)[0]
            mn = struct.unpack_from("<3f", d, 48)
            escala = struct.unpack_from("<f", d, 60)[0]
            mx = struct.unpack_from("<3f", d, 64)
            off_pos = struct.unpack_from("<7I", d, 80)[4]
            self.assertGreater(escala, 0)
            self.assertLess(escala, 0.001)  # < 1 mm para modelos de até 32 m
            for k in range(nv):
                x, y, z, w = struct.unpack_from("<4h", d, off_pos + 8 * k)
                if (x, y, z) == (0, 0, 0):
                    continue  # folga entre partes
                for c, a, b in ((x, mn[0], mx[0]), (y, mn[1], mx[1]), (z, mn[2], mx[2])):
                    self.assertGreaterEqual(c * escala, a - escala)
                    self.assertLessEqual(c * escala, b + escala)
                self.assertEqual(w, 0)


class TesteEmpacotador(unittest.TestCase):
    def test_recusa_arquivo_sem_registro(self):
        extra = os.path.join(RAIZ, "assets/texturas/_teste_sem_registro.btex")
        open(extra, "wb").write(b"x")
        try:
            with tempfile.TemporaryDirectory() as d:
                r = subprocess.run([sys.executable, os.path.join(AQUI, "empacotar.py"), "--saida", os.path.join(d, "x.pak")],
                                   capture_output=True, text=True)
            self.assertNotEqual(r.returncode, 0)
            self.assertIn("PAK_RECUSADO", r.stdout)
        finally:
            os.remove(extra)


if __name__ == "__main__":
    unittest.main(verbosity=1)
