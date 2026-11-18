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
 │                                                        └► conversor_mapa ─► ILHA.BMP2 + minimapa (Etapa 5)
 ├─ game/data/*.json, backend/config/** ─► data/*.json do Brisa (revisados) ─► compilador_dados ─► *.BIN
 ├─ game/assets/audio/**/*.wav ─► (reamostrar 22/32 kHz) ─► adpenc ─► *.ADP (Etapa 10)
 └─ ui/fontes/fredoka_*.woff2 ─► rasterizador ─► fonte bitmap CLUT 4 bits (Etapa 10)
                                   empacotador ─► BRISA.PAK (+ BRISAPnn.PAK)
```

## 1. Ferramentas da Etapa 0 (prontas e testadas)

| Ferramenta | O que faz | Resultado medido |
|---|---|---|
| `tools/auditoria/glb.py` | Leitor de GLB só com stdlib (accessors, malhas, pele, animações) | Lê os 90 GLB do Ventania |
| `tools/auditoria/auditar_ventania.py` | Triângulos visuais e de colisão, vértices, ossos, influências, animações, células da paleta, marcadores e estimativa de memória no PS2 | `docs/auditoria/assets_ventania.{json,md}`; os triângulos batem com o `validate_glb.py` do Ventania |
| `tools/auditoria/extrair_ilha.sh` + `extrair_ilha.gd` | Roda o gerador da Ilha Ventania na Godot 4.7.2 headless **numa cópia** e salva o layout | 646 peças, 1.219 props, 37 baús, 102 loot, 6 POIs, 10 estradas, 7 escadas, 24 spawns; duas execuções geram o mesmo arquivo, byte a byte |
| `tools/auditoria/densidade_mapa.py` | Triângulos por setor (64 m e 32 m) | Setor de 64 m mais denso: 25,2 mil; mediana: 2,0 mil |
| `tools/auditoria/lista_reuso.py` | Classifica cada GLB para reaproveitamento | `docs/auditoria/REUSO_GLB.md` |

## 2. Conversor de modelos (Etapa 3)

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

## 3. Conversor de texturas (Etapa 3)

Poucas texturas: fonte, ícones do HUD, sombra blob, faixas da Tormenta, céu,
minimapa e cartões da loja. Entrada PNG → quantização para 16 ou 256 cores →
`*.BTX` (CLUT no layout do GS, com o swizzle de CSM1 tratado pelo conversor),
largura e altura em potência de 2, alfa de 0 a 128 (escala do GS).

## 4. Conversor de mapa (Etapa 5)

Entrada: `ilha_ventania_layout.json` (ou o layout da ilha compacta).
1. **Relevo** 201×201 → blocos de 32 m com 2 LODs. Cores do terreno por regra (grama em dois tons, areia, rocha, terra), como o shader do Ventania, mas assadas por vértice.
2. **Estradas:** as faixas de asfalto, ruas, calçadas e terra são **pré-malhadas** pelo conversor (no Ventania são geradas em tempo de execução).
3. **Instâncias** (peças, props, baús, loot): agrupadas por setor de 32 m, ordenadas por modelo (menos troca de estado), com distância de visibilidade por tipo.
4. **Interiores:** a mobília é marcada com o edifício a que pertence, para o culling "só por dentro".
5. **Colisão** por célula de 4 m e **navegação** (o grafo do `NavGrade` do Ventania calculado offline).
6. **Minimapa e mapa completo:** imagem vista de cima (relevo + estradas + construções + cores das regiões) em 128² e 256², CLUT 8 bits.

## 5. Dados de jogo (Etapa 3 em diante)

`data/*.json` do Brisa (legíveis, começam como cópia revisada dos valores do
Ventania) → `compilador_dados` → `*.BIN`. Validador com as regras do Ventania:
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
