# Pipeline de assets: Ventania → Brisa

Objetivo: aproveitar o máximo do trabalho artístico do Ventania **sem usar GLB
no PS2** e sem alterar o Ventania. Todas as ferramentas rodam no PC (Python 3
com a biblioteca padrão, como no Ventania; Godot headless só na extração do
mapa) e produzem arquivos binários no formato do Brisa
(`BRISA_ARCHITECTURE.md` §8).

```
 Ventania (cópia local, commit fixo, somente leitura)
 ├─ game/assets/**/*.glb ───────► auditar_ventania.py ─► docs/auditoria/ (Etapa 0 ✅)
 │                          └────► conversor_modelos ──► assets/**/*.BMD|BSK|BAN (Etapa 3)
 ├─ tools/assets/kit/paleta.json ► cores de vértice (no conversor)
 ├─ tools/assets/kit/blender/*.py ► (cópia no Brisa) geradores com perfil PS2 → LODs e assets novos (Etapa 3+)
 ├─ game/world/ilha/*.gd ──────► extrair_ilha.sh (Godot headless, cópia) ─► layout JSON (Etapa 0 ✅)
 │                                                        └► conversor/mapa.py ─► mapa/ilha.bmap + minimapa (Etapa 5 ✅)
 ├─ game/data/*.json, backend/config/** ─► data/*.json do Brisa (revisados) ─► compilador_dados ─► *.BIN
 ├─ game/assets/audio/**/*.wav ─► (reamostrar 22/32 kHz) ─► adpenc ─► *.ADP (Etapa 10)
 └─ ui/fontes/fredoka_*.woff2 ─► rasterizador ─► fonte bitmap CLUT 4 bits (Etapa 10)
                                   empacotador ─► BRISA.PAK (+ BRISAPnn.PAK)
```

## 0. Como rodar (Etapa 3)

```bash
make conteudo-ventania VENTANIA=../ventania
```

Esse alvo reconverte tudo do Ventania para `assets/` (cerca de 13 s) e monta
`build/conteudo/BRISA.PAK`. O build comum (`make`) só precisa de `assets/`
(versionado no Brisa); ele **não depende do Ventania**. Testes do pipeline:

```bash
python3 tools/conversor/testes.py
```

Implementado na Etapa 3:
- leitura do GLB;
- partes rígidas (o conversor recusa skinning com mais de um osso por vértice);
- cor pela célula da paleta (recusa UV em célula vazia);
- normais do GLB;
- colisões em AABB a partir dos nós `-colonly`;
- marcadores;
- animações amostradas a 30 Hz;
- partes alinhadas a 4 vértices (exigência do VIF);
- orçamento de LOD0 com redução automática;
- LOD1/LOD2 dos trajes por QEM;
- texturas PNG → PSMT8/PSMT4 com CLUT CSM1;
- sombra blob original;
- catálogo e pacote.

Ainda **não** feitos: tiras de triângulos, o compilador de dados de jogo
(`data/*.json` → BIN, que entra com o primeiro sistema que usa esses dados, na
Etapa 6). Desde a Etapa 5: posições em int16 (BMOD v3, BR-ADR-14), LODs dos
props usados no mapa e o conversor de mapa (§4).

## 1. Ferramentas da Etapa 0 (prontas e testadas)

| Ferramenta | O que faz | Resultado medido |
|---|---|---|
| `tools/auditoria/glb.py` | Leitor de GLB só com stdlib (accessors, malhas, pele, animações) | Lê os 90 GLB do Ventania |
| `tools/auditoria/auditar_ventania.py` | Triângulos visuais e de colisão, vértices, ossos, influências, animações, células da paleta, marcadores e estimativa de memória no PS2 | `docs/auditoria/assets_ventania.{json,md}`; os triângulos batem com o `validate_glb.py` do Ventania |
| `tools/auditoria/extrair_ilha.sh` + `extrair_ilha.gd` | Roda o gerador da Ilha Ventania na Godot 4.7.2 headless **numa cópia** e salva o layout | 646 peças, 1.219 props, 37 baús, 102 loot, 6 POIs, 10 estradas, 7 escadas, 24 spawns; duas execuções geram o mesmo arquivo, byte a byte |
| `tools/auditoria/densidade_mapa.py` | Triângulos por setor (64 m e 32 m) | Setor de 64 m mais denso: 25,2 mil; mediana: 2,0 mil |
| `tools/auditoria/lista_reuso.py` | Classifica cada GLB para reaproveitamento | `docs/auditoria/REUSO_GLB.md` |

## 2. Conversor de modelos (Etapa 3 — implementado em `tools/conversor/`)

**Entrada:** o GLB exportado pelo Ventania (não o `.blend`; assim não é preciso
ter o Blender para converter).

1. **Leitura:** `glb.py`. Descarta os nós `-colonly`/`-convcolonly` (eles viram colisão).
2. **Cor:** para cada vértice, a UV → célula da paleta (atlas de 256 px, células de 32 px, 8 por linha) → cor RGB da `paleta.json`. O atlas tem a origem da UV no topo: isso será **conferido com um asset de cor conhecida** (o cachecol coral do Recruta) antes de converter em lote. Se houver UV dentro de uma célula vazia (magenta), a conversão falha.
3. **Normais** quantizadas (int8) para a luz em 2 faixas do VU1.
4. **Posição** em int16 com escala por malha (precisão abaixo de 1 mm em 4 m).
5. **Tiras:** stripificação gulosa. Corte por lote de vértices que cabe no buffer duplo do VU1 (o tamanho exato é definido pelo microprograma da Etapa 2).
6. **Skinning rígido** (personagens): agrupa os triângulos por osso (o osso de peso 1). Cada grupo vira um lote com uma matriz. A conversão **falha** se aparecer vértice com mais de uma influência.
7. **Animações:** amostragem a 30 Hz dos canais (rotação de cada osso + translação do quadril), quaternions normalizados em int16, ciclo marcado pelo sufixo `-loop` (convenção do Ventania).
8. **Marcadores** (`grip_r`, `grip_l`, `muzzle`, `mag`, `alca_tampa`, `cabeca`…) viram pontos nomeados no arquivo.
9. **LOD:** dois caminhos a comparar na Etapa 3:
   - (a) decimação por quádricas no conversor;
   - (b) **reexecutar os geradores Blender do Ventania (copiados para o Brisa) com menos segmentos** (esferas, chanfros, cilindros). O (b) preserva melhor a silhueta e a identidade e é o preferido se o `bpy` 5.3 (o mesmo do Ventania) rodar no ambiente.
10. **Validação:** orçamento por categoria (tabela abaixo), checagem de formato e uma pré-visualização em PNG renderizada pelo próprio conversor (rasterizador em software simples), para revisão sem console.

| Categoria | LOD0 (máx. tris) | LOD1 | LOD2 |
|---|---|---|---|
| Personagem | 3.500 | ~1.000 | ~300 |
| Arma / picareta | 600 | 150 | — |
| Peça de construção | 200 | 60 | — |
| Prop de cenário | 600 | 150 | billboard ou nada |
| Baú / coletável | 500 | 120 | — |
| Mochila / acessório | 400 | 120 | — |

## 3. Conversor de texturas (Etapa 3 — implementado em `tools/conversor/textura.py`)

Poucas texturas: fonte, ícones do HUD, sombra blob, faixas da Tormenta, céu,
minimapa e cartões da loja. Entrada PNG → quantização para 16 ou 256 cores →
`*.BTX` (CLUT no layout do GS, com o swizzle de CSM1 tratado pelo conversor),
largura e altura em potência de 2, alfa de 0 a 128 (escala do GS).

## 4. Conversor de mapa (Etapa 5 — implementado em `tools/conversor/mapa.py`)

Roda dentro de `make conteudo-ventania` (cerca de 3,5 s). Entrada:
`docs/auditoria/ilha_ventania_layout.json`, gerado por
`tools/auditoria/extrair_ilha.sh`. Essa extração roda o gerador do Ventania
numa cópia, e duas execuções dão o mesmo arquivo byte a byte. O conversor também
lê os GLB das peças de construção do Ventania.

1. **Relevo** 201×201 (centímetros, i16) e **cores do chão** por amostra. É o `terreno.gdshader` do Ventania avaliado no vértice: pesos de areia, rocha e terra extraídos do gerador, manchas de grama de ~30 m. Os ruídos finos (abaixo de 3 m) não cabem na grade e entram pelo valor médio. A malha do relevo é gerada no jogo (BR-ADR-15).
2. **Peças da grade:** a `Grade.transform` do Ventania. As paredes com porta (máscara 493), janela (495) e vitrine/portão (448, 455) viram modelos próprios, como no `grade.gd` do Ventania:
   - os ladrilhos 3×3 são recortados da parede do kit, e as aberturas ganham moldura escura da paleta;
   - a QEM volta a malha para ~1,7× a parede inteira;
   - a colisão é uma caixa por ladrilho presente.
3. **Props, baús e pontos de início** com posição, rotação e escala do layout.
4. **Estradas pré-malhadas** (`_malhas_estradas` + `estrada.gdshader`), agrupadas em modelos por setor de 30 m:
   - asfalto com bordas brancas e faixa central amarela tracejada; ruas de bairro com bordas tracejadas;
   - calçadas de concreto com meio-fio nos trechos urbanos e faixas de pedestres;
   - os caminhos de terra são só cor do chão (como no Ventania).
5. **Setores de 30 m**, com as instâncias ordenadas por modelo e as alturas mínima e máxima; categorias com distância máxima e **cadeia de LODs por QEM** (peças: LOD1 30% e LOD2 12%; árvores, props grandes e baús: LOD1 30%).
6. **POIs** com contorno (para a região no HUD) e a costa (para o limite de movimento do Ventania, a 9 m da praia).
7. **Minimapa** 256×256 PSMT8 (2,34 m por pixel): chão com sombreado, mar, estradas, plantas das casas, árvores e contornos das regiões.

8. **Grafo de navegação dos bots** (Etapa 7, `tools/conversor/navegacao.py`): a construção do `NavGrade` do Ventania (`nav_grade.gd`) portada e rodada offline, gravada em `mapa/ilha.bnav` (formato BNAV):
   - 150×150 células de 4 m, bloqueadas no mar, a menos de 3 m da costa e nos obstáculos do layout (`nav_obstacles()`: círculos e retângulos girados);
   - arestas fechadas por desnível > 3,4 m e pelas paredes das peças, com as portas marcadas (ponto "preciso" no meio da porta);
   - andares acima de 2,5 m (pisos das casas) com vizinhos, rampas e as escadas (a `ROTA_ESCADA` do Ventania: degraus e patamares como pontos da ligação);
   - os centros dos cômodos (`rooms()`), para a exploração de prédios.

   A extração (`extrair_ilha.gd`) passou a gravar `comodos` e `obstaculos_nav`; duas execuções seguem dando o mesmo arquivo. Resultado: 111 andares, 30 ligações (15 pares, 7 de escada), 11.776 células bloqueadas, nenhuma escada sem os dois pisos.

Os pontos de loot entraram na Etapa 6 (BMAP v2).

## 5. Dados de jogo (Etapa 6 — `tools/conversor/dados.py`)

`data/*.json` do Brisa (legíveis, começam como cópia revisada dos valores do
Ventania) → `tools/conversor/dados.py` → `assets/dados/*.bdat` (formato BDAT em
`FORMATOS.md`), registrado no catálogo. `make dados` recompila só os dados,
sem o Ventania; o `make conteudo-ventania` também os inclui. Implementado na
Etapa 6: `data/itens.json` (armas, consumíveis, munição, materiais, picareta,
raridades, baú, dispersão), com a validação das referências. Etapa 7:
`data/bots.json` (as 3 dificuldades com os 21 parâmetros do Ventania, a
mistura, os orçamentos de raios e de decisão, a audição) e
`data/nomes_jogadores.txt` (os 1.000 nomes) → `dados/bots.bdat` (BBOT), com
os trajes que os bots podem usar (`trajes_dos_bots`). Os outros arquivos
entram com os sistemas que os usam. Validador com as regras do Ventania:
- temporada: níveis 1..N com uma recompensa cada, itens existentes, estimativa de horas;
- loja: itens com visual, preços por raridade;
- cosméticos: todo id com modelo.

## 6. Áudio (Etapa 10)

WAV do Ventania → reamostragem (22.050 ou 32.000 Hz, mono) → `adpenc` (PS2SDK)
→ `*.ADP`. Banco de efeitos carregado inteiro no SPU2 (cerca de 250 KB).

## 7. Regras

- O pipeline aceita só a **versão do Ventania registrada** em `docs/SINCRONIA_VENTANIA.md`. Com outra versão, ele avisa e exige confirmação.
- Toda saída é **reproduzível**: o mesmo commit do Ventania e a mesma versão das ferramentas geram os mesmos bytes (o extrator do mapa já foi verificado assim).
- Nenhuma ferramenta escreve no repositório do Ventania.
