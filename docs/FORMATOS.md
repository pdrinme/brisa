# Formatos binários do Brisa

Todos em little-endian (o EE e o PC também são). Os blocos ficam alinhados a 16
bytes, e os arquivos dentro do pacote começam alinhados a 64. Escritores:
`tools/conversor/formatos.py`. Leitores: `src/conteudo/*.cpp`. **Mudou um, mude
o outro e suba a versão**: o jogo recusa versões que não conhece, com mensagem.

## BPAK — pacote de conteúdo (`BRISA.PAK`)

| Offset | Tipo | Campo |
|---|---|---|
| 0 | char[4] | `BPAK` |
| 4 | u32 | versão do formato (1) |
| 8 | u32 | versão de conteúdo (patches; ver BRISA_ARCHITECTURE §11) |
| 12 | u32 | n de arquivos |
| 16 | u32 | offset do índice (64) |
| 20 | u32 | tamanho total |
| 24 | u32 | CRC32 do índice |
| 64 | 64 B × n | índice, **ordenado por nome** (busca binária): `nome[44]`, `offset u32`, `tamanho u32`, `crc32 u32`, `tipo[4]` (`BMOD`/`BANI`/`BTEX`), 4 B livres |
| … | | arquivos, cada um alinhado a 64 |

## BMOD — modelo

Cabeçalho de 128 bytes:

| Offset | Tipo | Campo |
|---|---|---|
| 0 | char[4] | `BMOD` |
| 4 | u32 | versão (2) |
| 8 | u32 | n de vértices (com a folga entre partes) |
| 12 | u32 | n de partes (≤ 40) |
| 16 | u32 | n de nós (≤ 32) |
| 20 | u32 | n de marcadores |
| 24 | u32 | n de colisões |
| 28 | u32 | flags (bit 0: esquelético) |
| 32 | f32[4] | esfera envolvente (centro, raio) |
| 48 | f32[3]+4 | AABB mínimo |
| 64 | f32[3]+4 | AABB máximo |
| 80 | u32[7] | offsets: partes, nós, marcadores, colisões, posições, normais, cores |

Blocos:

| Bloco | Registro | Conteúdo |
|---|---|---|
| Partes | 32 B | `no i32` (−1 = estática), `inicio u32` (múltiplo de 4), `n u32` (múltiplo de 3), 4 B, `centro f32[3]`, `raio f32` (no espaço "bind" da parte) |
| Nós | 144 B | `nome[24]`, `pai i32`, `flags u32` (bit 0 animado, bit 1 osso), `t f32[3]+4`, `r f32[4]` (quaternion x,y,z,w), `s f32[3]+4`, `inv_bind f32[16]` (coluna principal). Pais vêm antes dos filhos |
| Marcadores | 48 B | `nome[24]`, `no i32` (pai), `pos f32[3]` (local ao pai), 8 B |
| Colisões | 32 B | `min f32[3]`, `tipo u32`, `max f32[3]`, `espessura f32`, no espaço do modelo. Tipo 0 = caixa; 1–4 = rampa (laje inclinada) que sobe para +x, −x, +z ou −z, com o topo indo de `min.y + espessura` na ponta baixa até `max.y` na alta (v2; na v1 era só a AABB) |
| Posições | 16 B/vértice | `x, y, z, 1` (f32) → UNPACK V4-32 |
| Normais | 4 B/vértice | `nx, ny, nz, 0` em i8 (×127) → UNPACK V4-8 com sinal |
| Cores | 4 B/vértice | `r, g, b, 255` → UNPACK V4-8 sem sinal |

Normais e cores têm folga zerada até o múltiplo de 4 (+4). Posição final de um
vértice: `instância · mundo(nó) · inv_bind(nó) · p` (parte com nó) ou
`instância · p` (parte estática).

**Assinatura do esqueleto** (para casar modelo e animação): CRC32 de
`"índice:nome"` dos nós com flag de osso ou animado, unidos por `|`.

## BANI — animações

| Offset | Tipo | Campo |
|---|---|---|
| 0 | char[4] | `BANI` |
| 4 | u32 | versão (1) |
| 8 | u32 | n de nós (igual ao do modelo) |
| 12 | u32 | n de clipes |
| 16 | u32 | quadros por segundo (30) |
| 20 | u32 | assinatura do esqueleto |
| 32 | 48 B × n | clipes: `nome[24]`, `n_quadros u32`, `loop u32`, `offset u32`, `n_nos u32`, 8 B |
| … | 16 B por nó por quadro | `q i16[4]` (×32767), `t i16[3]` (milímetros), 2 B |

No laço, o último quadro é igual ao primeiro (o período é n − 1 quadros). A
escala vem do repouso do modelo. O nome do clipe perde o sufixo `-loop` do
Ventania, e o flag `loop` guarda essa informação.

## BTEX — textura

| Offset | Tipo | Campo |
|---|---|---|
| 0 | char[4] | `BTEX` |
| 4 | u32 | versão (1) |
| 8 | u16, u16 | largura, altura (potência de 2) |
| 12 | u8 | PSM: `0x13` PSMT8, `0x14` PSMT4 |
| 14 | u16 | n de cores (256 / 16) |
| 16 | u32 | offset dos pixels (64) |
| 20 | u32 | offset da CLUT |
| 24 | u32 | bytes de pixels |

Pixels: 1 B por pixel (PSMT8), ou 4 bits com o pixel par no nibble baixo
(PSMT4). CLUT: RGBA de 8 bits por canal, com **alfa na escala do GS (0..128)**.
Na PSMT8, a CLUT já vem no arranjo CSM1 (em cada bloco de 32 entradas, a faixa
8–15 troca de lugar com a 16–23), pronta para subir para a VRAM.
