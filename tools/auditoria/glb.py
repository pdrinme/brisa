"""Leitor mínimo de GLB (glTF 2.0 binário) — só a biblioteca padrão do Python.

Usado pela auditoria da Etapa 0 e, a partir da Etapa 3, pelo conversor de
modelos do Brisa. Lê os GLB já exportados pelo Ventania (somente leitura).
"""
import json
import struct

_COMP = {5120: ("b", 1), 5121: ("B", 1), 5122: ("h", 2), 5123: ("H", 2), 5125: ("I", 4), 5126: ("f", 4)}
_NCOMP = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


class Glb:
    def __init__(self, caminho):
        with open(caminho, "rb") as f:
            dados = f.read()
        magic, versao, _ = struct.unpack_from("<4sII", dados, 0)
        if magic != b"glTF" or versao != 2:
            raise ValueError(f"{caminho}: não é GLB 2.0")
        off = 12
        self.json = None
        self.bin = b""
        while off < len(dados):
            tam, tipo = struct.unpack_from("<I4s", dados, off)
            corpo = dados[off + 8: off + 8 + tam]
            if tipo == b"JSON":
                self.json = json.loads(corpo.decode("utf-8"))
            elif tipo == b"BIN\x00":
                self.bin = corpo
            off += 8 + tam
        self.caminho = caminho

    def accessor(self, idx):
        """Devolve a lista de elementos (tuplas) do accessor."""
        a = self.json["accessors"][idx]
        bv = self.json["bufferViews"][a["bufferView"]]
        fmt, tam = _COMP[a["componentType"]]
        n = _NCOMP[a["type"]]
        passo = bv.get("byteStride") or tam * n
        base = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
        out = []
        for i in range(a["count"]):
            out.append(struct.unpack_from("<" + fmt * n, self.bin, base + i * passo))
        if a.get("normalized") and fmt in ("B", "H"):
            m = 255.0 if fmt == "B" else 65535.0
            out = [tuple(v / m for v in e) for e in out]
        return out

    def primitivas(self):
        for mi, malha in enumerate(self.json.get("meshes", [])):
            for p in malha["primitives"]:
                yield mi, malha.get("name", ""), p
