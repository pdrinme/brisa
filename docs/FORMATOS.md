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
| 4 | u32 | versão (4) |
| 8 | u32 | n de vértices (com a folga entre partes) |
| 12 | u32 | n de partes (≤ 40) |
| 16 | u32 | n de nós (≤ 32) |
| 20 | u32 | n de marcadores |
| 24 | u32 | n de colisões |
| 28 | u32 | flags (bit 0: esquelético) |
| 32 | f32[4] | esfera envolvente (centro, raio) |
| 48 | f32[3], f32 | AABB mínimo; escala das posições (v3) |
| 64 | f32[3]+4 | AABB máximo |
| 80 | u32[7] | offsets: partes, nós, marcadores, colisões, posições, normais, cores |

Blocos:

| Bloco | Registro | Conteúdo |
|---|---|---|
| Partes | 32 B | `no i32` (−1 = estática), `inicio u32` (múltiplo de 4), `n u32` (múltiplo de 3), 4 B, `centro f32[3]`, `raio f32` (no espaço "bind" da parte) |
| Nós | 144 B | `nome[24]`, `pai i32`, `flags u32` (bit 0 animado, bit 1 osso), `t f32[3]+4`, `r f32[4]` (quaternion x,y,z,w), `s f32[3]+4`, `inv_bind f32[16]` (coluna principal). Pais vêm antes dos filhos |
| Marcadores | 48 B | `nome[24]`, `no i32` (pai), `pos f32[3]` (local ao pai), 8 B |
| Colisões | 32 B | `min f32[3]`, `tipo u32`, `max f32[3]`, `espessura f32`, no espaço do modelo. Tipo 0 = caixa; 1–4 = rampa (laje inclinada) que sobe para +x, −x, +z ou −z, com o topo indo de `min.y + espessura` na ponta baixa até `max.y` na alta e a base `espessura` abaixo do topo (v2; na v1 era só a AABB); 5–8 = cunha, com o mesmo topo da rampa nos mesmos sentidos e a base plana em `min.y` (v4: lances de escada) |
| Posições | 8 B/vértice | `x, y, z, 0` em i16 (× escala do cabeçalho) → UNPACK V4-16 com sinal; o VU1 converte com ITOF0 e a escala vai na matriz (v3; até a v2 eram 4 f32) |
| Normais | 4 B/vértice | `nx, ny, nz, 0` em i8 (×127) → UNPACK V4-8 com sinal |
| Cores | 4 B/vértice | `r, g, b, 255` → UNPACK V4-8 sem sinal |

Posições, normais e cores têm folga zerada até o múltiplo de 4 (+4). Posição final de um
vértice: `instância · mundo(nó) · inv_bind(nó) · (escala · p)` (parte com nó) ou
`instância · (escala · p)` (parte estática).

**Assinatura do esqueleto** (para casar modelo e animação): CRC32 de
`"índice:nome"` dos nós com flag de osso ou animado, unidos por `|`.

## BMAP — mapa (Etapa 5)

Escrito por `tools/conversor/mapa.py`; lido por `src/mundo/mapa.cpp` (sem cópia).
Cabeçalho de 128 bytes:

| Offset | Tipo | Campo |
|---|---|---|
| 0 | char[4] | `BMAP` |
| 4 | u32 | versão (2) |
| 8 | u32 | n de amostras do relevo por lado (201) |
| 12 | f32 | passo do relevo (3 m) |
| 16 | f32 | x e z da amostra (0, 0) (−300) |
| 20 | f32 | nível do mar (−1,2) |
| 24 | u32 | setores por lado (20) |
| 28 | f32 | lado do setor (30 m) |
| 32 | u32 ×6 | n de modelos, instâncias, POIs, pontos de início, pontos de contorno, amostras da costa |
| 56 | u32 ×9 | offsets: relevo, cores, costa, modelos, setores, instâncias, POIs, pontos, pontos de início |
| 92 | u32, u32 | n de pontos de loot e offset (v2) |

Blocos:

| Bloco | Registro | Conteúdo |
|---|---|---|
| Relevo | 2 B/amostra | altura em centímetros (i16), linha a linha (z), como `_alturas` do Ventania |
| Cores | 4 B/amostra | `r, g, b, 255`: o `terreno.gdshader` avaliado na amostra (pesos de areia, rocha e terra; manchas de grama) |
| Costa | 4 B × 360 | `coast_radius` do Ventania a cada 1° (`atan2(z, x)`) |
| Modelos | 48 B | `nome[32]` (`modelos/<nome>.bmod`), `lod i32` (próximo LOD na tabela, ou −1), `dist_lod f32` (a partir de quantos metros usar o próximo), `dist_max f32`, `categoria u32` |
| Setores | 16 B | `inicio u32`, `n u32` (instâncias do setor), `ymin f32`, `ymax f32` |
| Instâncias | 32 B | `modelo u16`, `categoria u16`, `x, y, z, rot (graus), escala, raio f32`, 4 B. Ordenadas por setor e, dentro dele, por modelo |
| POIs | 96 B | `id[24]`, `nome[40]` (UTF-8), `x, y, z, raio f32`, `cor u32` (0xRRGGBB), `inicio u32`, `n_pontos u32` (contorno), 4 B |
| Pontos | 8 B | `x, z f32` dos contornos das regiões |
| Pontos de início | 16 B | `x, y, z f32`, 4 B |
| Pontos de loot | 16 B | `x, y, z f32`, 4 B (v2: os 102 do Ventania) |

Categorias: 0 peça de construção, 1 árvore, 2 prop pequeno, 3 móvel, 4 prop
grande, 5 baú, 6 estrada. A malha do relevo **não** vai no arquivo: o jogo a
gera na carga (BRISA_ARCHITECTURE §18).

## BDAT — dados de jogo (Etapa 6)

Escrito por `tools/conversor/dados.py` a partir de `data/itens.json` (cópia
revisada do `game/data/itens.json` do Ventania); lido por `src/combate/itens.cpp`.
O compilador valida as referências antes de gravar (munição de cada arma, modelos
existentes, consumível com vida **ou** escudo, quantidade ≤ pilha). Cabeçalho de
64 bytes: `BDAT`, versão (1), n de raridades, munições, materiais, armas,
consumíveis e ferramentas (u32 ×6), e os offsets dos 8 blocos (u32 ×8).

| Bloco | Registro | Campos |
|---|---|---|
| Raridades | 52 B | `id[12]`, `nome[16]`, `cor u32`, `cor2 u32`, `dano`, `recarga`, `peso_bau`, `peso_chao` (f32) |
| Munições | 48 B | `id[12]`, `nome[24]`, `max u32`, `caixa u32`, 4 B |
| Materiais | 32 B | `id[12]`, `nome[16]`, `max u32` |
| Armas | 132 B | `id[12]`, `nome[16]`, `tipo[24]`, `modelo[24]`, `municao u32` (índice), `dano`, `cadencia` (f32), `automatica`, `pente` (u32), `recarga_s`, `alcance`, `dispersao` (f32), `projeteis u32`, `cabeca`, `peso`, `dispersao_mirando` (f32; < 0 = sem), 4 B, `pose u32` (1 rifle, 2 precisão, 3 pistola) |
| Consumíveis | 100 B | `id[12]`, `nome[32]`, `modelo[24]`, `vida`, `escudo`, `limite`, `tempo_s` (f32), `pilha`, `quantidade` (u32), `peso f32`, 4 B |
| Ferramentas | 76 B | `id[12]`, `nome[16]`, `modelo[24]`, `cadencia`, `impacto`, `alcance` (f32), `dano_jogador`, `dano_construcao` (u32), 4 B |
| Modificadores | 16 B | dispersão: `mirando`, `movendo`, `agachado`, `no_ar` (f32) |
| Baú | 16 B | `armas`, `consumiveis`, `caixas_municao` (u32), 4 B |

Os textos são UTF-8 com o zero no fim; o compilador recusa nomes que não cabem.
Índices de item no jogo: 0 = vazio, depois as armas, os consumíveis e as
ferramentas, na ordem do JSON (os mesmos do `ItemDB` do Ventania).

### BBOT — dados dos bots (Etapa 7)

Também é um arquivo `dados/` (`assets/dados/bots.bdat`, 24.432 bytes), escrito
por `tools/conversor/dados.py` a partir de `data/bots.json` (cópia revisada do
`game/data/bots.json` do Ventania, com os mesmos valores) e de
`data/nomes_jogadores.txt` (os 1.000 nomes do Ventania); lido por
`src/ia/dados_bots.h`. O compilador confere que a mistura só cita dificuldades
existentes, que cada dificuldade tem os 21 campos, que os nomes não se repetem e
cabem em 24 bytes, e que os trajes dos bots existem no pacote. Cabeçalho de 64
bytes: `BBOT`, versão (1), `preencher_ate`, n de dificuldades, da mistura, de
nomes e de trajes (u32 ×6), `decisao_hz`, `decisao_longe_hz`, `lod_distancia_m`
(f32), `raios_visao_por_tick` (u32), `audicao_tiro_m` (f32) e os offsets dos 4
blocos (u32 ×4).

| Bloco | Registro | Campos |
|---|---|---|
| Dificuldades | 100 B | `nome[16]` e 21 f32: `reacao_s`, `erro_mira_graus`, `alcance_visao_m`, `campo_visao_graus`, `engaja_m`, `cura_abaixo`, `constroi`, `coleta`, `salto_cedo`, `decisao_hz`, `mira_assenta_s`, `memoria_s`, `investiga`, `usa_cobertura`, `cobertura_vida`, `recua_vida`, `atira_andando`, `rajada_ticks`, `pausa_ticks`, `ritmo_lateral`, `explora_predios` |
| Mistura | 1 B | índice da dificuldade do bot *i* (cíclico) |
| Nomes | 24 B | UTF-8 com o zero no fim |
| Trajes | 24 B | id do traje (`recruta_base_01`, `recruta_brisa_01`) |

## BNAV — grafo de navegação dos bots (Etapa 7)

Escrito por `tools/conversor/navegacao.py` (chamado por `mapa.py`), que porta a
construção do `NavGrade` do Ventania (`game/gameplay/ai/nav_grade.gd`) e roda
**offline** sobre o layout extraído da ilha; lido por `src/mundo/navegacao.cpp`
(`mapa/ilha.bnav`, 120.928 bytes). Cabeçalho de 96 bytes: `BNAV`, versão (1), `N`
(150), tamanho da célula `T` (4 m, f32), `LIMITE` (300 m, f32), n de andares,
de ligações, de pontos e de cômodos (u32 ×4) e os offsets dos 8 blocos (u32 ×8).

| Bloco | Registro | Campos |
|---|---|---|
| Alturas | i16 × N² | altura do chão no centro de cada célula, em cm |
| Bloqueio | u8 × N² | 1 = célula fora do mapa (mar, costa < 3 m, desnível, obstáculo) |
| Arestas | u8 × N² | bits N, L, S, O (0–3): parede ou desnível > 3,4 m naquele lado |
| Portas | u8 × N² | bits N, L, S, O: a parede daquele lado tem porta (passa, ponto "preciso") |
| Andares | 36 B | `x, y, z` (f32), `i, j, nivel` (i16), 2 B, `viz[4]` (i32; −1 = sem vizinho) |
| Ligações | 16 B | `a, b` (i32, nós), `custo` (f32), `ponto` (u16, primeiro), `n_pontos` (u16) |
| Pontos | 12 B | `x, y, z` (f32) da rota da ligação (degraus e patamares da escada, rampa) |
| Cômodos | 16 B | centro `x, y, z` (f32), 4 B (exploração de prédios) |

Nós: `0..N²−1` são as células do chão (`j·N + i`), e `N² + k` é o andar *k*.
As ligações têm um sentido (cada escada ou rampa gera as duas). A ilha atual tem
111 andares, 30 ligações (15 pares, 7 deles de escada), 118 pontos, 149
cômodos e 11.776 células bloqueadas.

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
