"""Matemática mínima (stdlib) para o conversor: matrizes 4×4 em listas de 16
floats, coluna principal (m[c*4 + l]), igual a src/nucleo/mat.h do Brisa."""
import math


def identidade():
    return [1.0 if i % 5 == 0 else 0.0 for i in range(16)]


def mul(a, b):
    r = [0.0] * 16
    for c in range(4):
        for l in range(4):
            r[c * 4 + l] = sum(a[k * 4 + l] * b[c * 4 + k] for k in range(4))
    return r


def trs(t=(0, 0, 0), q=(0, 0, 0, 1), s=(1, 1, 1)):
    x, y, z, w = q
    r = [1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w), 0,
         2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w), 0,
         2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y), 0,
         0, 0, 0, 1]
    for c in range(3):
        for l in range(3):
            r[c * 4 + l] *= s[c]
    r[12], r[13], r[14] = t
    return r


def ponto(m, p):
    x, y, z = p
    return (m[0] * x + m[4] * y + m[8] * z + m[12],
            m[1] * x + m[5] * y + m[9] * z + m[13],
            m[2] * x + m[6] * y + m[10] * z + m[14])


def direcao(m, d):
    x, y, z = d
    return (m[0] * x + m[4] * y + m[8] * z,
            m[1] * x + m[5] * y + m[9] * z,
            m[2] * x + m[6] * y + m[10] * z)


def inversa(m):
    """Inversa geral 4×4 (eliminação de Gauss-Jordan)."""
    a = [[m[c * 4 + l] for c in range(4)] + [1.0 if l == c else 0.0 for c in range(4)] for l in range(4)]
    for col in range(4):
        piv = max(range(col, 4), key=lambda r: abs(a[r][col]))
        if abs(a[piv][col]) < 1e-12:
            raise ValueError("matriz singular")
        a[col], a[piv] = a[piv], a[col]
        p = a[col][col]
        a[col] = [v / p for v in a[col]]
        for r in range(4):
            if r != col:
                f = a[r][col]
                a[r] = [a[r][k] - f * a[col][k] for k in range(8)]
    return [a[l][4 + c] for c in range(4) for l in range(4)]


def normalizar(v):
    n = math.sqrt(sum(c * c for c in v))
    return tuple(c / n for c in v) if n > 1e-12 else tuple(0.0 for _ in v)


def vetorial(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def slerp(a, b, t):
    d = sum(x * y for x, y in zip(a, b))
    if d < 0:
        b = tuple(-x for x in b)
        d = -d
    if d > 0.9995:
        r = tuple(x + (y - x) * t for x, y in zip(a, b))
        return normalizar(r)
    th = math.acos(min(1.0, d))
    s = math.sin(th)
    wa, wb = math.sin((1 - t) * th) / s, math.sin(t * th) / s
    return tuple(wa * x + wb * y for x, y in zip(a, b))
