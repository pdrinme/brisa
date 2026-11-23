# Licenças de Assets — Brisa

## 1. Política

1. **Todo asset distribuído** com o Brisa (modelo, textura, animação, som, música, fonte, ícone, dado de mapa) precisa ser:
   - **original** do Brisa;
   - **original do Ventania** com autorização do titular (o mesmo dono dos dois projetos);
   - **CC0 ou domínio público**;
   - ou de uma licença que permita uso e **redistribuição** em jogo: CC-BY com atribuição, OFL para fontes, MIT, Apache-2.0.
2. **Nada da Epic Games, do Fortnite ou de qualquer outro jogo comercial**, nem mesmo como "referência dentro do projeto". Isso vale para arquivos extraídos, "vazados", de dumps públicos ou de ferramentas de porting. Estar disponível publicamente **não é licença**. Também não se recria de forma reconhecível nenhum personagem, item, mapa, som, logotipo ou interface de outro jogo. (A mesma regra do Ventania, em `ASSET_LICENSES.md` e `docs/assets/REFERENCIAS.md` de lá.)
3. **Nada é reaproveitado automaticamente.** Cada arquivo do Ventania passa por cinco verificações (origem, autoria, licença, redistribuição e compatibilidade técnica) antes de entrar no catálogo do Brisa.
4. **A BIOS do PS2 nunca entra** no repositório nem nos pacotes. Quem testa em emulador usa o dump do próprio console.
5. **Dependências de código** (PS2SDK, gsKit e outros) ficam registradas na §5, com a licença de cada uma.
6. O catálogo do Brisa (`assets/catalogo_brisa.json`, a partir da Etapa 3) é a fonte da verdade, e o build recusa asset sem registro (a mesma regra do Ventania).

## 2. Autorização do titular do Ventania

Todos os assets do Ventania estão registrados lá como **"Original — Projeto
Ventania"** (90 de 90 GLB no `assets_catalog.json`, conferidos pela auditoria).
Por serem originais do mesmo dono, o uso no Brisa depende só da autorização
dele.

> **Situação (2026-11-18):** o titular pediu para priorizar os assets originais
> do Ventania ("priorize os assets originais criados especificamente para o
> Ventania") e autorizou a Etapa 3, cujo escopo é importá-los. Com base nisso,
> a Etapa 3 converteu os modelos, as animações e os ícones listados no
> catálogo. **Falta uma confirmação explícita por escrito**, que deve ser
> registrada aqui com data antes da primeira distribuição pública (Etapa 13).

## 3. Lista de reaproveitamento (Etapa 0)

### 3.1 Modelos (GLB)

A lista **arquivo por arquivo** (90 GLB) fica em
[`docs/auditoria/REUSO_GLB.md`](docs/auditoria/REUSO_GLB.md), gerada pela
auditoria. Resumo:

| Decisão | Quantidade | Exemplos |
|---|---|---|
| Reaproveitar (conversão direta) | 71 | paredes, pisos, rampas, tetos, baú, armas, consumíveis, árvores, pedras, props, mobília |
| Reaproveitar com LODs | 11 | os 11 trajes (2.128–4.072 triângulos, esqueleto de 20 ossos, skinning rígido) |
| Reaproveitar com simplificação | 7 | 3 escadas em U (1.212–1.428 triângulos), gôndola (984), Ônibus dos Ventos (908), Âncora de Coleta (772), planador Arraia (668) |
| Não usar | 1 | `caixote_exemplo_01` (asset de teste do pipeline do Ventania) |

Verificações feitas em cada GLB:
- **Origem e autoria:** `assets_catalog.json` do Ventania (todos `original`, autor "Projeto Ventania") e o gerador correspondente em `tools/assets/kit/blender/`.
- **Licença:** `Original` (nenhum de terceiros).
- **Redistribuição:** depende da §2.
- **Técnica:** triângulos, materiais, ossos, influências, animações e colisão, medidos por `tools/auditoria/auditar_ventania.py`.

### 3.2 Outros arquivos do Ventania

| Arquivo | Origem / licença | Decisão para o Brisa |
|---|---|---|
| `tools/assets/kit/paleta.json` (40 cores) | Original | **Reaproveitar** como fonte das cores de vértice. A ordem das cores é preservada |
| `tools/assets/kit/blender/*.py` (geradores dos modelos) | Original | **Reaproveitar, copiando para o repositório do Brisa** (com nota de origem), para gerar LODs e os assets novos (mochilas, emotes). A cópia nunca altera o Ventania. Depende da §2 |
| `game/assets/audio/jogo/*.wav` (37) e `audio/ui/*.wav` (9) | Original (sintetizados por `tools/audio/*.py`) | **Reaproveitar** convertendo para ADPCM. Os geradores de áudio também podem ser copiados |
| `game/assets/ui/fontes/fredoka_*.woff2` | OFL-1.1, The Fredoka Project Authors | **Reaproveitar** rasterizada em bitmap, com o texto da OFL nos créditos e no pacote. A declaração de copyright não tem Reserved Font Name |
| `game/assets/ui/icones/*.png` (11 ícones de itens) | Original (renderizados dos modelos) | Reaproveitar reduzidos ou **renderizar de novo** pelo pipeline do Brisa (mais coerente com o visual PS2) |
| `game/assets/ui/loja/*.png` (16 imagens) | Original (renderizadas dos modelos) | Idem, com cartões de 128×160 em CLUT 8 bits |
| `game/assets/ui/icone.png` | Original | Referência para o ícone do save e a tela inicial |
| `game/data/nomes_jogadores.txt` (1.000 nomes) | "Lista fornecida pelo dono do projeto" | **Reaproveitar** (depende da §2) |
| Layout da Ilha Ventania (saída do gerador determinístico) | Original (código do Ventania) | **Reaproveitar** (`docs/auditoria/ilha_ventania_layout.json`) |
| Valores de jogo (`game/data/*.json`, `backend/config/**`) | Original | **Reaproveitar** como valores iniciais, reescritos no formato do Brisa |
| Shaders Godot (`*.gdshader`), cenas `.tscn`, materiais `.tres` | Original | **Não aproveitáveis** no PS2; servem só de especificação visual (2 faixas de luz, sombra azulada, rachaduras) |
| Código GDScript e Python | Original | **Só referência.** Nenhuma linha roda no PS2; o comportamento é reimplementado em C++ |
| `references/` | Vazia no repositório (fora do git por política) | Nada a reaproveitar |

## 4. Registro de assets do Brisa

Ainda vazio: nenhum asset do Ventania foi convertido (Etapa 3). A Etapa 1 só
tem conteúdo original do Brisa escrito em código: a fonte bitmap 5×7
(`src/nucleo/fonte.cpp`, desenho próprio) e as cores do tema do Ventania
(valores hexadecimais de `ventania_theme.gd`, do mesmo dono). A partir da Etapa 3, cada
asset convertido ganha uma entrada no formato:

```json
{
  "id": "parede_madeira",
  "arquivo": "assets/construcoes/parede_madeira.bmd",
  "origem": "ventania:game/assets/construcoes/parede_madeira.glb@7864cd7",
  "autor": "Projeto Ventania",
  "licenca": "Original",
  "autorizacao": "titular (ver §2)",
  "modificado": "conversão para BMD, cor de vértice, colisão em caixa",
  "estado": "convertido"
}
```

(O formato real gerado tem `id`, `arquivo`, `tipo`, `bytes`, `crc32`, `origem`,
`autor`, `licenca`, `autorizacao`, `categoria` e as medidas de cada tipo.)

**Etapa 5 (mapa):** os arquivos gerados a partir do Ventania também estão no
catálogo, com a origem exata:
- `mapa/ilha.bmap`: layout extraído do gerador `ilha_ventania.gd`;
- as paredes com porta e janela: GLB da parede + regras de `grade.gd`;
- as estradas: layout + cores de `estrada.gdshader`;
- os LODs do mapa: derivados por QEM dos modelos convertidos;
- o minimapa: gerado pelo Brisa a partir do layout.

Todos têm a licença do titular (§2), e nenhum asset de terceiros entrou.

## 5. Dependências de código e ferramentas

| Componente | Uso | Licença |
|---|---|---|
| PS2SDK (ps2dev) | Bibliotecas do EE/IOP, IRX | Academic Free License 2.0 (conferido no `LICENSE` do pacote, 2026-11-18) |
| gsKit (ps2dev) | Vídeo e 2D | Academic Free License 2.0 (conferido no `LICENSE` do repositório gsKit) |
| Toolchain GCC/binutils (ps2dev) | Compilação | GPL (ferramenta; não é distribuída com o jogo) |
| Godot 4.7.2 (só no pipeline, para extrair o mapa) | Ferramenta | MIT (não é distribuída com o jogo) |
| PCSX2 | Emulador de teste | GPL (ferramenta; não é distribuída) |
| Play! (Jean-Philip Desjardins) | Emulador de teste da Etapa 1 (AutoTest e o capturador `tools/emulador/captura_play`, que liga com ele) | BSD de 2 cláusulas (conferida no `License.txt`); ferramenta, não é distribuída com o jogo |
| Mesa, Xvfb | Renderização em software para as capturas do emulador | MIT/X11; ferramentas |

## 6. Atribuições exigidas

- **Fredoka:** Copyright 2016 The Fredoka Project Authors. Licenciada sob a SIL Open Font License 1.1. Aparece nos Créditos.
- PS2SDK e gsKit: atribuição nos Créditos, conforme a licença deles.
