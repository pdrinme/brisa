# Arquitetura — Brisa (PlayStation 2)

Versão 0.1 (Etapa 0). As decisões marcadas **[BR-ADR-n]** estão justificadas na
§14. Tudo o que diz "medir" ou "a medir" é hipótese até haver número de
emulador **e** de console.

## 1. O que o Brisa é, tecnicamente

- Um único **ELF** para o Emotion Engine, em C/C++, com PS2SDK e gsKit.
- Dados de conteúdo **fora do ELF** (pacotes `.PAK` versionados), para permitir patches sem recompilar (§11).
- **Offline e local**: um jogador e bots. Sem rede, sem servidor, sem meta-servidor.
- Um núcleo de jogo **independente de plataforma**, que compila também no PC (Linux) para testes automáticos sem PS2 (§12).

## 2. O hardware e o que ele impõe

| Recurso | PS2 | Consequência para o Brisa |
|---|---|---|
| CPU (EE, R5900) | ~295 MHz, FPU de precisão simples, cache de 16 KB (instruções) e 8 KB (dados), scratchpad de 16 KB | Código de dados contíguos, sem alocação no laço, pools fixos. IA e física com orçamento por tick |
| VU0 / VU1 | VU1: 16 KB de microprograma e 16 KB de dados, ligado ao GS pelo PATH1 | **Transformação, iluminação e recorte no VU1**; o EE só monta listas DMA |
| RAM principal | 32 MB | Orçamento na §6; tudo do mapa cabe sem streaming |
| GS | 4 MB de eDRAM (framebuffer + Z + texturas), sem T&L, preenchimento muito rápido, Gouraud, neblina por vértice, alfa, texturas CLUT de 4 e 8 bits | Visual por **cor de vértice** (a paleta do Ventania); poucas texturas, pequenas e CLUT |
| IOP | ~37 MHz, 2 MB | Controle, Memory Card, USB e áudio rodam nele (módulos IRX) |
| SPU2 | 2 MB, ADPCM, 48 vozes | Efeitos em ADPCM; música em streaming (§10) |
| Memory Card | 8 MB | Save pequeno (< 64 KB com o ícone) |
| Vídeo | NTSC 640×448 entrelaçado (60 campos/s); PAL 640×512 (50) | Meta de 30 FPS = 1 quadro a cada 2 campos em NTSC |
| Controle | DualShock 2 (dois analógicos, vibração) | Interface só por controle |

## 3. Visão geral dos módulos

```
              ┌──────────────────────────── núcleo (C++ portátil) ────────────────────────────┐
  data/*.PAK ─► conteudo/ (catálogos, armas, temporada, loja)                                 │
              │ mundo/ (setores, relevo, peças)  gameplay/  armas/  inventario/  construcao/  │
              │ bots/  partida/  progressao/  (lógica pura, tick fixo de 30 Hz)               │
              └───────────────┬──────────────────────────────────────────┬────────────────────┘
                              │ estado (somente leitura)                 │ comandos
              ┌───────────────▼───────────────┐          ┌───────────────┴─────────────┐
              │ apresentacao/: renderer/      │          │ entrada/ (DualShock 2)      │
              │ (VU1, GS), ui/, audio/, fx/   │          │ save/ (Memory Card)         │
              └───────────────┬───────────────┘          └───────────────┬─────────────┘
                              │                                          │
              ┌───────────────▼──────────────────────────────────────────▼─────────────┐
              │ plataforma/ps2 (gsKit, DMA, VIF, libpad, libmc, audsrv, RTC, arquivos) │
              │ plataforma/host (stubs para testes no PC)                              │
              └────────────────────────────────────────────────────────────────────────┘
```

**Regras:**
1. O núcleo **não inclui** cabeçalhos do PS2SDK. Ele vê a plataforma só por interfaces pequenas (`Arquivos`, `Relogio`, `Controle`, `Save`).
2. A apresentação lê o estado do núcleo e nunca o altera. Os bots e o jogador geram o mesmo `Comando` (como no Ventania, onde o bot é um jogador comum).
3. Sem alocação dinâmica depois do carregamento da partida: arenas por fase (`boot`, `menu`, `partida`) e pools de tamanho fixo.

## 4. Laço principal e tempo

- **Simulação em tick fixo de 30 Hz** (o mesmo tick do Ventania, o que mantém os números de movimento e armas válidos), com acumulador. No máximo 2 ticks por quadro; se ficar para trás, o jogo desacelera em vez de "espiral da morte".
- **Renderização a 30 FPS** com vsync (NTSC: a cada 2 campos). A interpolação visual entre ticks é opcional, decidida pela medição.
- Ordem do quadro: entrada → tick(s) do núcleo → montagem das listas de desenho (EE) → envio DMA (VIF1/GIF) enquanto o EE prepara o próximo quadro (buffer duplo de pacotes).

## 5. Renderização **[BR-ADR-1]**

- **Inicialização, modo de vídeo, VRAM e 2D: gsKit.** Sprites, fontes e telas de interface também usam gsKit.
- **3D: renderizador próprio no VU1.** O EE monta, por objeto visível, um pacote VIF (`packet2` do PS2SDK) com a matriz e as tiras de vértices; o microprograma do VU1 transforma, ilumina, recorta e manda GIFtags pelo PATH1.
  - **Etapa 2 compara dois caminhos medidos:** (A) transformação no EE com gsKit (a referência simples) e (B) VU1. O B só é adotado se render pelo menos 3× mais triângulos por quadro.
- **Formato de vértice** (por tira): posição 3×int16 (escala por malha), cor RGBA8 (da paleta), normal 3×int8, 16 bytes alinhados. **Sem UV na maioria das malhas.**
- **Estilo "toon" do Ventania no PS2:**
  - luz em 2 faixas calculada no VU1 (N·L com degrau suave) sobre a cor da paleta;
  - sombra puxada para o azul (a cor da sombra é um parâmetro, como `cor_sombra` no Ventania);
  - o brilho de borda (rim) é opcional (barato no VU1: 1 − N·V);
  - **sombras: blob** (um quad com textura radial de 32×32) sob personagens e baús. Sem shadow maps.
- **Neblina do GS** por vértice para a distância de visão, na cor do céu.
- **Culling:** setores de 32 m por frustum; objetos por distância (`visibilidade_m` do Ventania); interiores mobiliados só com a câmera dentro do edifício ou a menos de 25 m.
- **LOD:** personagens em 3 níveis; peças de construção e props com LOD0/LOD1 pelo pipeline.
- **Ordem de desenho:** opacos por setor (perto → longe), depois alfa (Tormenta, efeitos, vidro), depois HUD.

### 5.1 VRAM (4 MB), configuração inicial a validar na Etapa 1

| Uso | Formato | Tamanho |
|---|---|---|
| Framebuffer A, 640×448 | PSMCT32 | 1,09 MB |
| Framebuffer B, 640×448 | PSMCT32 | 1,09 MB |
| Z-buffer, 640×448 | PSMZ24 | 1,09 MB |
| Texturas residentes (fonte, HUD, blob, faixas da Tormenta, céu) | CLUT 4/8 bits | ≤ 0,5 MB |
| Livre / texturas enviadas por quadro (minimapa, cartões da loja) | — | ~0,2 MB |

Alternativas, se faltar espaço: framebuffer em PSMCT16S, ou renderização por
campo (640×224). A escolha é por medição e por comparação visual na TV.

**Etapa 1 (só 2D):** 2 framebuffers PSMCT24 de 640×448, sem Z-buffer (o Z entra
na Etapa 2), no modo de vídeo do console (NTSC ou PAL, detectado pelo gsKit).

### 5.2 Orçamento por quadro (hipótese inicial; Etapa 2 mede)

| Item | Meta |
|---|---|
| Triângulos 3D na tela (mundo + personagens) | 40–70 mil |
| Personagens animados visíveis | ≤ 8 em LOD0/LOD1, o resto em LOD2 |
| Tempo do EE (simulação + bots + montagem) | ≤ 20 ms de 33,3 ms |
| Tempo do GS/VU1 | ≤ 30 ms |

**Por que o mapa precisa de cuidado (medido na Etapa 0):** a Ilha Ventania tem
cerca de 236 mil triângulos de construções e props, mais 80 mil de relevo no
LOD0. O setor de 64 m mais denso (Vila Brisa) soma cerca de 25 mil, e a mediana
fica em cerca de 2 mil. A mobília (61 mil triângulos) só é vista por dentro.
Dados em `docs/auditoria/`.

## 6. Memória principal (32 MB), orçamento inicial

| Uso | Orçamento |
|---|---|
| ELF (código + dados estáticos) + IRX embutidos | 4 MB |
| Pilhas, heap do sistema, buffers do PS2SDK | 2 MB |
| Modelos (≈ 100 malhas; ≈ 70 mil vértices × 16 B no total original + LODs) | 3 MB |
| Animações (1 conjunto compartilhado + emotes) | 0,5 MB |
| Mapa: relevo 201×201, instâncias, colisão, navegação, minimapa | 4 MB |
| Partida: entidades, loot, peças (limite a medir), bots | 3 MB |
| Interface: texturas (cópia em RAM), cartões, fontes | 2 MB |
| Áudio: banco de efeitos já está no SPU2; buffers de streaming | 0,5 MB |
| Pacotes DMA (2 buffers) | 2 MB |
| **Reserva** | **≥ 11 MB** |

O ELF imprime no boot um mapa de memória, e a Etapa 2 em diante registra o pico
de uso em cada medição.

## 7. Mundo, colisão e navegação

- **Uma grade só para tudo:** a grade de 4 m da construção do Ventania também indexa colisão, tiro e navegação. Cada célula guarda as peças (até 5 slots: parede X, parede Z, piso, rampa, teto) e uma lista curta de props.
- **Relevo:** 201×201 amostras a cada 3 m (o mesmo do Ventania), com altura bilinear. Visual em blocos de 32 m com 2 LODs.
- **Colisão do personagem:** cilindro vertical (raio 0,35 m, altura 1,8 m; agachado 1,15 m, o valor do Ventania) contra o relevo, as caixas finas das peças (paredes de 0,24 m, pisos com topo a 0,10 m), planos inclinados (rampas), cilindros ou caixas dos props e a colisão simplificada das escadas.
- **Tiro:** raio por DDA na grade (como um voxel traversal) e, depois, contra as hitboxes (cabeça e corpo) dos personagens próximos ao raio.
- **Navegação dos bots** (Etapa 7, §20): o grafo estático (150×150 células de 4 m mais nós de andar, escadas e portas) sai **pronto do pipeline**. Em jogo, só se atualizam as arestas das células tocadas por construção ou destruição. A* com limite de expansões por tick.

## 8. Dados e formatos (todos little-endian, alinhados a 16 bytes)

| Arquivo | Conteúdo | Produzido por |
|---|---|---|
| `*.BMD` | Malha estática: cabeçalho, escala, tiras com cor e normal, AABB, LODs | `tools/conversor_modelos` |
| `*.BSK` | Malha esquelética: partes rígidas → índice do osso, LODs | idem |
| `*.BAN` | Animação: 20 ossos, 30 Hz, quaternions int16 + deslocamento do quadril | idem |
| `*.BTX` | Textura CLUT 4/8 bits + paleta (formato do GS) | `tools/conversor_texturas` |
| `*.BMP2` (mapa) | Relevo, setores, instâncias (modelo, posição, rotação, escala, material de impacto), peças do mapa, baús, loot, spawns, POIs, estradas pré-malhadas, grafo de navegação | `tools/conversor_mapa` |
| `*.BIN` (conteúdo) | Armas, raridades, consumíveis, construção, Tormenta, bots, progressão, missões, temporada, catálogo de cosméticos e loja, compilados de JSON | `tools/compilador_dados` |
| `*.ADP` | Efeitos ADPCM (SPU2) | `adpenc` do PS2SDK |
| `BRISA.PAK` / `BRISAPnn.PAK` | Pacote: índice (caminho → offset, tamanho, CRC32), versão de conteúdo | `tools/empacotador` |

**Regra:** o ELF valida a versão de cada formato no carregamento e recusa com
uma mensagem clara (não com uma tela preta). Os JSON de origem ficam em
`data/` e são legíveis por pessoas; o ELF nunca faz parse de JSON.

## 9. Salvamento (Memory Card)

- **Pasta** `BRISA` no cartão (slot 1, com o 2 como alternativa), com `icon.sys`, o ícone 3D (`brisa.ico`, convertido do baú do Ventania: azul-petróleo e amarelo) e dois arquivos de dados, `PERFIL.A` e `PERFIL.B`.
- **Cabeçalho:** `"BRSV"`, versão do formato, contador de geração (u32), tamanho, **CRC32** do conteúdo e versão de conteúdo.
- **Conteúdo** (TLV, < 16 KB): nome do jogador, nível, XP, Faíscas, ledger curto (32 entradas), desbloqueios (bitset por id de cosmético + origem), equipados (traje, picareta, mochila, planador, 6 emotes), temporada atual + XP e nível do passe + recompensas recebidas, progresso de missões (período + contadores), estatísticas de vida, configurações (volume, sensibilidade, inversão, vibração, posição da tela) e a data da última rotação vista da loja.
- **Escrita segura:** grava sempre no arquivo **mais antigo** (A/B alternados) e só depois o considera o atual. Uma queda de energia ou cartão removido no meio da escrita deixa o outro arquivo intacto.
- **Leitura e recuperação:** lê os dois, valida o CRC e a versão e usa o de maior geração válido. Se só um for válido, avisa e regrava o outro. Se nenhum for, oferece "Começar novo perfil" **sem apagar** os arquivos corrompidos até o jogador confirmar.
- **Migração:** cada versão do formato tem uma função que converte da anterior.
- **Casos tratados:** sem cartão (jogar sem salvar, com aviso), cartão cheio (espaço necessário em KB), cartão trocado durante o jogo, formatação pendente.
- **Quando salvar:** ao fim de cada partida, ao comprar, ao equipar e ao sair das configurações. Nunca durante a partida.

## 10. Áudio

- `audsrv` (PS2SDK) para efeitos ADPCM carregados no SPU2 e para streaming de música (quando houver; o Ventania não tem música).
- Os 46 efeitos originais do Ventania (cerca de 840 KB em WAV) viram ADPCM (≈ 3,5:1, cerca de 250 KB no SPU2).
- Barramentos de volume como no Ventania: Música, Efeitos e Interface.

## 11. Conteúdo e patches

- **Ordem de carregamento:** `BRISA.PAK` (base) e, depois, `BRISAP01.PAK`, `BRISAP02.PAK`…; um arquivo de patch **substitui** o de mesmo caminho.
- **Atualizável sem ELF novo:** armas e itens (valores), cosméticos (catálogo e novos modelos no formato atual), loja (agenda, preços, rotação), temporadas (nova temporada = novo arquivo de temporada + recompensas), missões, configurações de partida e Tormenta, e mapas no formato de mapa atual.
- **Exige ELF novo:** mudança de formato, sistema novo ou correção de código. Cada ELF declara a faixa de `versao_conteudo` que aceita.
- **Histórico:** `CHANGELOG.md` com a versão do ELF e a versão de conteúdo de cada entrega.

## 12. Testes

| Nível | Onde | O quê |
|---|---|---|
| Unitários do núcleo | PC (host, `make testes`) | Tormenta, inventário (conservação), grade e desabamento, armas, progressão, rotação da loja, save (CRC, A/B, migração, corrupção) |
| Simulação headless | PC | Partida completa só com bots: termina, tem vencedor, tempo preso < 1% (o mesmo critério do `test_bots` do Ventania) |
| Pipeline | PC | Conversores: orçamento, formato, ida e volta |
| Emulador | PCSX2 (BIOS do próprio dono, nunca distribuída) | Boot, telas, partida e medições |
| Console | PS2 real via USB (OPL ou uLaunchELF) ou rede (`ps2link` + `ps2client`) | **Fonte da verdade do desempenho** |

**Regra:** nenhuma etapa é marcada concluída só com emulador, se o critério
for de desempenho ou de compatibilidade.

## 13. Relação com o Ventania

- O Ventania é **referência somente leitura**. O Brisa nunca escreve no repositório do Ventania e não depende dele para compilar o ELF.
- **O pipeline lê o Ventania** (uma cópia local, em uma versão fixa): os GLB, a paleta, os JSON de dados e o layout da ilha, extraído rodando o gerador determinístico do Ventania na Godot headless **numa cópia temporária** do projeto.
- **Versão de referência atual:** commit `7864cd7` (v1.2.1). Cada sincronização registra o commit em `docs/SINCRONIA_VENTANIA.md`.

## 14. Registro de decisões

**BR-ADR-1: gsKit para o 2D e o vídeo, VU1 próprio para o 3D.** O gsKit é
maduro, está no toolchain e cobre modos de vídeo, VRAM, sprites e fontes, mas
transforma os vértices no EE: isso não chega ao volume de triângulos que a
Ilha exige. Um microprograma próprio no VU1 é o caminho de jogos comerciais da
época, e o estilo de cor chapada do Ventania o torna simples (sem UV, sem
textura). *Alternativas avaliadas:* Tyra (engine C++ para PS2 com VU1) e ps2gl
(API estilo OpenGL). Ambas trariam a arquitetura delas e dependências; as
licenças precisam ser conferidas antes de qualquer reaproveitamento. Ficam como
material de estudo, não como dependência. *Risco:* o microcódigo do VU1 é a
parte mais difícil; por isso a Etapa 2 começa pelo caminho A (EE + gsKit) e
mede antes de investir.

**BR-ADR-2: C++17 "enxuto".** O toolchain validado tem GCC 15.2. Usamos C++17
sem exceções e sem RTTI, sem contêineres da STL no laço (pools próprios),
`-O2 -G0`. C puro fica para o código de plataforma e os microprogramas. Validado
na Etapa 0 com uma sonda que liga libpad, libmc e audsrv.

**BR-ADR-3: núcleo portátil e testado no PC.** Sem rede, a simulação do
Ventania (servidor autoritativo) vira uma simulação local. Isolar o núcleo da
plataforma permite rodar no PC os mesmos testes de regras que o Ventania roda
(Tormenta, conservação do inventário, bots), sem depender de emulador.

**BR-ADR-4: mapa extraído, não redesenhado à mão.** O mapa do Ventania é gerado
por código determinístico (semente 20261116, `FastNoiseLite` da Godot).
Reescrever esse ruído fora da Godot geraria uma ilha diferente. A ferramenta do
Brisa roda o **próprio gerador do Ventania** na Godot headless, numa cópia, e
salva o layout em JSON. Testado na Etapa 0: 646 peças, 1.219 props, 37 baús,
102 pontos de loot, 6 POIs, 10 estradas, 7 escadas e 24 pontos de início.

**BR-ADR-5: cor de vértice no lugar do atlas de paleta.** No Ventania, cada
peça usa uma célula de 32 px de um atlas de 40 cores, escolhida pela UV. O
conversor do Brisa lê a UV, descobre a célula e grava a cor no vértice. O efeito
visual é idêntico, sem gastar VRAM, e mudar uma cor da paleta continua mudando o
jogo inteiro (basta reconverter).

**BR-ADR-6: participantes e tamanho do mapa definidos por medição.** Começa com
1 jogador + 11 bots (12). O alvo é 16, o mesmo número com que o Ventania
completa as partidas. O mapa começa com o layout da Ilha Ventania (para medir
com dados reais); a decisão entre manter os 600 m ou uma ilha compacta
derivada dele é tomada na Etapa 5, com os números (§5.2).

**BR-ADR-7: módulos do controle da ROM primeiro (Etapa 1).** O plano da Etapa 0
previa embutir `sio2man.irx` e `padman.irx` do PS2SDK no ELF. No emulador Play!,
o PADMAN de código aberto não sai de `PAD_STATE_EXECCMD`; os módulos da ROM
(`XSIO2MAN`/`XPADMAN`, ou `SIO2MAN`/`PADMAN` nos BIOS antigos) funcionam, e o
`libpad` atual aceita os dois protocolos. A ordem ficou ROM → IRX embutidos
(os embutidos continuam no ELF como alternativa; `-DBRISA_PAD_EMBUTIDO` inverte
a ordem). *A confirmar no console:* qual caminho é usado e se o controle
responde igual. Quando o Memory Card entrar (Etapa 12), o `MCMAN`/`MCSERV`
precisa ser da mesma família do `SIO2MAN` carregado.

**BR-ADR-8: tela 2D por lista de primitivas.** As telas montam uma lista
portátil (retângulos, gradientes, quads); o PS2 a envia pelo gsKit e o PC a
rasteriza em software. Isso permite testar telas e ver prévias sem console, e o
teste da Etapa 1 mostrou que a imagem do GS no emulador e a do PC são iguais
pixel a pixel nas áreas estáticas.

## 15. Motor gráfico (Etapa 2, implementado)

| Peça | Arquivo | Papel |
|---|---|---|
| Matemática | `src/nucleo/mat.*` | Vec3/Mat4 (coluna principal), perspectiva e câmera no estilo OpenGL, inversa rígida, frustum por planos |
| Malha | `src/render3d/malha.*` | Lista de triângulos em SoA no formato do VU1 (posição V4-16 com escala desde a Etapa 5 — era V4-32 —, normal V4-8 com sinal, cor V4-8), geradores procedurais com normal plana |
| Cena | `src/render3d/cena3d.h` | Câmera, luz toon (luz, sombra azulada, limiar, suavidade), neblina, objetos |
| Pipeline de referência | `src/render3d/pipeline_sw.*` | Transforma, ilumina, recorta (plano perto + faixa de guarda ±4w), descarta faces traseiras e projeta. É o **caminho A** no PS2 e a referência no PC |
| Caminho A (EE) | `src/plataforma/ps2/render_ee.*` | Triângulos → GIFtag PACKED (RGBAQ + XYZF2) em 2 buffers de 192 KB → DMA do GIF (PATH3) |
| Caminho B (VU1) | `src/plataforma/ps2/render_vu1.*`, `vu1/toon.vsm` | Cadeia de DMA para o VIF1 com referências às malhas (sem copiar vértices), lotes de até 96 vértices em buffer duplo (BASE 0, OFFSET 512), microprograma que transforma, ilumina, aplica Z/neblina e faz XGKICK (PATH1) |
| Quadro | `plataforma_ps2.cpp: desenhar_quadro` | Limpa (gsKit) → estado do 3D (TEST Z ≥, FOGCOL) → caminho escolhido → HUD 2D (gsKit, sem Z) → vsync |
| PC | `plataforma/host/rasterizador.*` | Rasterizador com Z "maior ou igual" e neblina por vértice como o GS: prévias e testes |

**Formato do Z e da neblina (os dois caminhos e o PC):** Z = (perto/w)·Zmax
(24 bits, perto = Z maior, teste GEQUAL), interpolado linearmente na tela;
neblina F = clamp((fim − w)·255/(fim − início)), com 255 = sem neblina.

**BR-ADR-9: o VU1 não recorta; objetos que cruzam o plano perto vão para o
EE.** O microprograma descarta triângulos com algum vértice fora do volume (Z
fora de ±w ou X/Y fora de ±4w), marcando ADC no XYZF2. O EE testa a esfera de
cada objeto: se ela cruza o plano perto (por exemplo, o chão sob a câmera),
o objeto inteiro vai pelo caminho A, que recorta. Assim o VU1 continua simples
e rápido, e nada some perto da câmera.

**Limitações conhecidas do caminho B (Etapa 2):**
- não descarta faces traseiras (o GS desenha e o Z rejeita): quando o GS for o gargalo, o descarte entra no microprograma (área com sinal na tela);
- o microprograma não está otimizado (um vértice por volta, com espera do DIV); há espaço para desenrolar o laço e intercalar instruções;
- um objeto com mais de 96 vértices é enviado em vários lotes, e cada lote reenvia o cabeçalho de 12 qw.

**Estado do BR-ADR-1:** os dois caminhos estão prontos e geram a mesma imagem
no emulador. A decisão **depende do benchmark no console**
(`docs/TESTE_NO_CONSOLE.md`, Etapa 2). Os tempos do emulador não valem.

## 16. Conteúdo e pipeline (Etapa 3, implementado)

| Peça | Arquivo | Papel |
|---|---|---|
| Conversor | `tools/conversor/construir.py` (+ `modelo.py`, `decimar.py`, `textura.py`, `formatos.py`, `mat.py`) | GLB do Ventania → BMOD/BANI/BTEX em `assets/`, com catálogo e relatório |
| Empacotador | `tools/conversor/empacotar.py` | `assets/` → `build/conteudo/BRISA.PAK`, recusando arquivo sem registro, CRC diferente ou licença fora da lista |
| Leitores | `src/conteudo/` (`pak`, `modelo`, `animacao`, `textura`) | Leem o pacote sem copiar: as malhas das partes apontam para os dados do pacote |
| Instância | `src/render3d/instancia.*` | Modelo + pose → uma entrada da cena por parte rígida |
| Sombras | `projetar_sombras` + plataforma | Quadrado com a textura blob no chão (gsKit, STQ, alfa, Z ligado) |
| Vitrine | `src/jogo/vitrine.*` | A vitrine do kit do Ventania com os assets convertidos |

**Formato de modelo único** (`docs/FORMATOS.md`): nós + partes rígidas +
marcadores + colisões. Cobre os três casos do Ventania sem código especial:
- personagem com pele rígida (uma parte por osso);
- objeto com nó animado (a tampa do baú);
- objeto estático (uma parte, nó −1).

**BR-ADR-10: o pacote vai embutido no ELF nesta fase.** Ler arquivos ao lado do
ELF exige os drivers do dispositivo de boot depois do reinício do IOP (USB:
`usbd`, `bdm`, `usbmass_bd`, `bdmfs_fatfs`; rede: os módulos do ps2link). O
formato e o carregador já são os definitivos (o jogo só vê um ponteiro para o
pacote); quando a leitura de arquivos entrar (até a Etapa 13, com os patches),
o mesmo `BRISA.PAK` passa a ser lido do dispositivo, e o embutido fica como
alternativa. O jogo continua com código e conteúdo separados no build: o
conteúdo pode ser reconstruído sem recompilar o código.

**BR-ADR-11: redução de polígonos por QEM no conversor.** Dos dois caminhos
previstos (PIPELINE_ASSETS §2), ficou a decimação por métricas quádricas,
parte rígida por parte rígida, com restrição nas bordas e nas fronteiras de
cor. Ela reduz os 11 trajes a ~1.000 e ~290 triângulos mantendo silhueta e
manchas de cor, em 13 s para todo o kit, sem precisar do Blender. A
reexecução dos geradores Blender continua possível, se algum LOD ficar ruim.

**BR-ADR-12: calibração da luz da vitrine.** A Godot ilumina em espaço linear
com luz ambiente, e o Brisa multiplica cor de vértice no espaço da tela. Na
imagem de referência, o lado do sol sai cerca de 1,3× a cor da paleta e o da
sombra 0,75–0,95× (azulado), e a vitrine usa esses fatores. Uma curva de tom
de verdade (por paleta, no conversor) fica como alternativa, se a TV mostrar
diferença.

**Memória medida na Etapa 3** (emulador; a contagem de bytes vale para o console):

| Item | Medido | Orçamento (§6) |
|---|---|---|
| ELF (com o pacote embutido) | 5,4 MB em disco; 9,0 MB na RAM (programa + bss) | 4 MB de código + o conteúdo à parte |
| Pacote inteiro | 5,0 MB (125 arquivos) | — |
| Modelos no pacote | 4,6 MB (os 33 modelos de personagem, com LODs, são 3,6 MB) | 3 MB |
| Modelos carregados pela vitrine | 50 modelos, 3,6 MB (sem cópia: apontam para o pacote) | — |
| Animações | 228 KB (2 bancos) | 0,5 MB |
| Texturas | 56 KB na RAM; < 80 KB na VRAM | — |
| Memória livre no boot | 18,3 MB | ≥ 11 MB de reserva |

**Achado de memória:** a lista de triângulos não indexada repete vértices e
gasta 24 B por vértice. Os modelos passam do orçamento (4,6 MB contra 3 MB). As
correções previstas, em ordem:
1. tiras de triângulos no conversor e no VU1 (cerca de 3× menos vértices);
2. posições em int16 com escala por parte (24 → 16 B por vértice);
3. carregar só os trajes usados na partida (a vitrine carrega todos).

Vale fazer antes do mapa (Etapa 5), quando o orçamento do mundo entrar.

## 17. Personagem, colisão e câmera (Etapa 4, implementado)

| Peça | Arquivo | Papel |
|---|---|---|
| Mundo de colisão | `src/mundo/colisao.*` | Caixas e rampas giradas em Y numa grade de 8 m (64 × 64 células, até 48 por célula, 2.048 colisores), mais o chão plano |
| Movimento | `src/personagem/movimento.*` | `passo_movimento`: os números do Ventania, a 30 Hz e portátil (o mesmo resultado no PC e no PS2) |
| Câmera | `src/personagem/camera3p.*` | Por cima do ombro, com braço de mola (raio contra o mundo) |
| Animador | `src/personagem/animador.*` | Misturas e one-shots do `pawn_visual.gd` sobre `amostrar_local` + `misturar_trs` + `compor_pose` |
| Campo de treino | `src/jogo/treino.*` | Cena, controles, HUD de depuração e o roteiro do autoteste |

**Colisão** (sem física de corpos rígidos, BRISA_GAME_DESIGN §7):
- **Formato:** o conversor classifica cada `-colonly` do Ventania como caixa ou **rampa** (laje inclinada, com sentido de subida e espessura; BMOD v2). Desde a Etapa 5 (BMOD v4), os prismas extrudados, como os lances das escadas em U de `gerar_interiores.py`, são cortados em caixas, rampas e **cunhas** (topo inclinado, base plana). Os telhados em pirâmide continuam como AABB (dá para andar em cima, como num bloco).
- **Consultas:** `chao` (o maior topo sob um círculo, até um limite), `empurrar` (tira o cilindro das paredes, e obstáculos com topo até pés + degrau não bloqueiam), `teto`, `livre` e `raio` (slab test no espaço do colisor; nas rampas, 12 amostras).
- **Movimento:** primeiro o horizontal, em subpassos de até 0,2 m (empurra, sobe degrau e rampa, mantém a velocidade ao longo do piso); depois o vertical (teto, chão e "grudar" a até 0,45 m ao descer). A velocidade guardada é a que as paredes deixaram andar, para a animação não "correr parada".
- **Custo:** ~545 KB de bss. Com a ilha (Etapa 5), a grade passa para a de 4 m do §7, com as peças por célula.

**BR-ADR-13: sem dano de queda.** O Ventania não tem dano de queda (o dano vem
só de armas, picareta e Tormenta, `combat_server.gd` e `partida_br.gd`), e o
Brisa mantém isso. A aterrissagem pesada é só visual. No Ventania ela toca ao
chegar ao chão depois da queda livre ou do planador. No Brisa, que ainda não
tem ônibus nem planador (Etapa 8), ela toca em qualquer queda de 6 m ou mais
(`mov::QUEDA_PESADA`). Quando o planador entrar, o pouso dele também vai
disparar a aterrissagem pesada.

## 18. Mapa (Etapa 5, implementado)

| Peça | Arquivo | Papel |
|---|---|---|
| Extração | `tools/auditoria/extrair_ilha.{sh,gd}` | O gerador do Ventania na Godot headless, numa cópia: layout, pesos do chão por amostra, costa e faixas de pedestres (`docs/auditoria/ilha_ventania_layout.json`) |
| Conversor | `tools/conversor/mapa.py` | Layout → `mapa/ilha.bmap`, paredes com porta/janela, estradas por setor, cadeia de LODs, minimapa |
| Leitor | `src/mundo/mapa.*` | BMAP sem cópia; regiões (ponto no contorno do POI) e costa |
| Relevo 3D | `src/mundo/terreno.*` | Malhas do relevo geradas na carga (LOD1) e sob demanda (LOD0), mar em ladrilhos |
| Colisão | `src/mundo/colisao.*` | Relevo bilinear (o `ground_height` do Ventania) + caixas e rampas com escala, grade compacta |
| Ilha | `src/jogo/ilha.*` | Carga, corte por setor/distância/LOD, HUD com região, minimapa e mapa completo |

**Conteúdo da ilha** (o mesmo layout do Ventania `7864cd7`): 646 peças da grade
de 4 m (com as máscaras de porta e janela), 1.219 props, 37 baús, 6 POIs com
contorno, 24 pontos de início e as estradas. Ao todo, 1.944 instâncias de 162
modelos e 266 mil triângulos, sem contar o relevo.

**Corte por quadro:**
1. **Setores de 30 m** (20 × 20): a esfera do setor (com as alturas) contra a distância de visão (240 m) e o frustum.
2. **Instâncias** do setor: distância máxima por categoria (peças 230 m, árvores 210, props grandes 190, baús 90, props pequenos 85, **móveis 24**: a mobília só é vista de perto, o que faz o papel do "só por dentro").
3. **Cadeia de LODs** na tabela de modelos: peças com LOD1 (30%) a partir de 45 m e LOD2 (12%) a partir de 110 m; árvores e props grandes com LOD1 a partir de 70 m; baús a partir de 40 m.
4. **Neblina** linear de 60 a 240 m na cor do Ventania (`cfe9ff`), com o céu `8fd3ff`.

**Medido nas 16 vistas do roteiro** (os números de triângulos e objetos valem
para o console; os iguais no PC e no emulador):

| Vista | Triângulos | Objetos na cena |
|---|---|---|
| Vila Brisa, chegando | **41.893** (pior caso) | 460 |
| Vila Brisa, do centro | 38.340 | 411 |
| Porto Ferrugem | 25.753 / 28.231 | 247 / 337 |
| Fazenda Girassol | 27.079 / 26.238 | 301 / 382 |
| Mirante do Vento (4 direções) | 14.478 a 25.915 | 306 a 581 |
| Pedreira Cinza | 11.463 / 16.136 | 177 / 310 |
| Acampamento Coral | 9.815 / 20.481 | 135 / 480 |

As peças de construção são a maior parte na Vila (27 a 40 mil triângulos antes
do corte por parte); o relevo fica entre 2,7 e 4,9 mil.

**BR-ADR-14: posições dos modelos em int16 com escala (BMOD v3).** Com posições
em 4 × f32, os modelos ocupavam 4,6 MB (orçamento: 3 MB). Agora cada vértice
guarda `x, y, z` em i16 × uma escala por modelo (erro abaixo de 0,1 mm num
personagem e de 1 mm numa peça de 4 m). O VU1 desempacota com UNPACK V4-16 com
sinal, converte com um ITOF0 e a escala entra nas colunas x, y, z da matriz
MVP; o EE e o PC usam `Malha::posicao`. O vértice cai de 24 para 16 bytes, e
os mesmos modelos foram de 4,6 para 3,1 MB. As tiras de triângulos ficam para
quando o VU1 for o gargalo medido no console.

**BR-ADR-15: o relevo é gerado na carga, não vem pronto no pacote.** A malha de
3 m da ilha inteira daria cerca de 2,3 MB. O BMAP leva só as alturas (80 KB) e
as cores (160 KB):
- o LOD1 (quadrados de 6 m) de todos os 281 blocos com terra é montado na carga (com o cache do LOD0, 888 KB);
- o LOD0 (3 m) é montado sob demanda, no máximo 4 blocos por quadro, num cache de 28 blocos em volta da câmera;
- as bordas do LOD0 usam só as amostras pares (as amostras ímpares da borda seguem a reta entre as pares), então os dois LODs se encontram sem frestas (teste `relevo_sem_frestas_entre_lods`);
- as posições usam a escala de 1/512 m: 30 m são 15.360 unidades exatas, e blocos vizinhos quantizam a borda comum do mesmo jeito.

A colisão usa o relevo bilinear (`ground_height`). A diferença para a malha
triangulada fica em poucos centímetros.

**BR-ADR-16: tamanho da ilha.** Decisão provisória: **(a) a Ilha Ventania
inteira**, com distância de visão de 240 m e neblina. Motivos medidos:
- **Memória:** o mapa inteiro cabe. São 316 KB do BMAP, cerca de 0,5 MB de modelos gerados (paredes com aberturas, estradas, LODs) e 65 KB de minimapa no pacote; em jogo, 1,2 MB de arena (relevo e biblioteca de modelos) e 341 KB de colisão. O orçamento do mapa é de 4 MB (§6).
- **Desenho:** a pior vista (chegando na Vila Brisa) envia 41,9 mil triângulos e as outras de 10 a 28 mil, dentro da meta de 40–70 mil do §5.2.
- **Identidade:** a ilha compacta (b) exigiria um layout novo, diferente do Ventania.

**Condições** (o aceite depende do console):
- se o benchmark do console (Etapa 2) mostrar que o VU1 não sustenta cerca de 42 mil triângulos a 30 FPS, a ordem de corte é: distância dos LODs e de visão (parâmetros prontos), depois o descarte de faces no microprograma, e só então a ilha compacta;
- a densidade de jogadores (12–16 em 600 m) é questão de partida, e entra na Etapa 8 (Tormenta e pontos de início), com medição do tempo até o primeiro encontro.

**Memória medida na Etapa 5** (contagem de bytes; vale para o console):

| Item | Medido | Orçamento (§6) |
|---|---|---|
| Pacote | 4,4 MB (231 arquivos): modelos 3,6 MB (trajes com LODs 2,5 MB; gerados para o mapa 0,5 MB), mapa 316 KB, animações 228 KB, texturas 121 KB | — |
| ELF na RAM | 11,5 MB (4,8 MB de código e pacote, 6,9 MB de bss, com a arena de 4 MB) | — |
| Livre no boot | 16,5 MB (eram 19,5 MB depois do int16 e 18,1 MB na Etapa 4) | Reserva ≥ 11 MB |
| Ilha em jogo | arena 1,2 MB (relevo 888 KB, biblioteca de 175 modelos ~300 KB) + colisão 341 KB | Mapa: 4 MB |

## 19. Combate e inventário (Etapa 6, implementado)

| Peça | Arquivo | Papel (no Ventania) |
|---|---|---|
| Dados | `data/itens.json` → `assets/dados/itens.bdat` (`tools/conversor/dados.py`) | `game/data/itens.json`, com os mesmos valores |
| Itens | `src/combate/itens.*` | `item_db.gd`: índices, nomes, raridades, sorteios por peso (o Mítico nunca sai) |
| Inventário e vitais | `src/combate/inventario.*` | `inventory.gd` e `vitals.gd`: picareta permanente, 5 espaços, reservas, nunca perde nem duplica |
| Tiro | `src/combate/tiro.*` | `combat.gd` e `hitbox.gd`: raio de mira pelo ombro, dispersão, projéteis determinísticos, cápsula + cabeça, tempos da picareta |
| Loot | `src/combate/loot.*` | `loot_server.gd`: pontos de loot, baús, munição automática, troca explícita |
| Combatente | `src/jogo/combatente.*` | `combat_server.gd` (por passo, 30 Hz): troca, tiro, recarga, consumível, golpe, baú e pegar |
| Portador | `src/personagem/portador.*` | `item_rig.gd`, `aim_pose_modifier.gd` e os `TwoBoneIK3D`: o item na mão e as mãos nos marcadores |
| HUD de combate | `src/jogo/hud_combate.*` | `combat_hud.gd`, `inventory_screen.gd`, `loot_visual.gd` |

**Regras iguais às do Ventania:**
- os tempos contam em passos de 30 Hz: cadência, recarga (com a raridade), uso, 6 passos para sacar, golpe de 19 passos com o impacto no passo 8, baú em 15 passos segurando ▲;
- o tiro passa primeiro pela colisão do mundo (`MundoColisao::raio`), e só acerta alvos mais perto que a parede;
- o dano soma os projéteis por alvo e vai primeiro para o escudo.

O roteiro de combate dá **o mesmo resultado no PC e no PS2**:
- o sorteio é um PCG32 com semente fixa;
- a direção de cada projétil sai da semente do tiro.

**Bonecos de treino:** os 3 do campo de treino (`DUMMIES` do Ventania), com 50 de
escudo. Quando zeram, voltam inteiros, e se recuperam 3 s depois do último
acerto. Ainda não colidem com o personagem: são só alvos.

**BR-ADR-17: armas na mão por IK sobre a pose rígida.** O Ventania não tem
clipes de tiro. A arma vai para a pose da categoria, e as mãos são puxadas até os
marcadores do modelo por dois `TwoBoneIK3D`. O Brisa faz o mesmo sobre as
matrizes da pose rígida, depois do animador:
1. mira: coluna, peito, pescoço e cabeça giram com os pesos do Ventania;
2. o item é posto com as mesmas chaves do `item_rig.gd` (quadril/mira, corrida, sacar, recuo, recarga por arma, golpes A/B da picareta, usos de cada consumível);
3. IK analítico de dois ossos com polo: a menor rotação leva o braço ao cotovelo calculado pela lei dos cossenos, e depois o antebraço ao alvo, com a mesma extensão de 6,5 cm na mão.

Os marcadores do item (`grip_r`, `grip_l`, `mag`, `muzzle`, `mao`, `cabeca`)
vêm do conversor da Etapa 3.

**BR-ADR-18: controles e atalhos.** O mapeamento é o do GAME_DESIGN §4: R2 atira
ou usa, L2 mira, ■ recarrega, ▲ interage, L1/R1 trocam de item, ▶ picareta, ◀
emote, ▲ no direcional mapa, SELECT inventário. Os atalhos de depuração passam
para combinações com L1 + R1 segurados, para não ocupar os botões do jogo:
- START: modo;
- SELECT: caminho de desenho VU1/EE;
- ●: traje.

START sozinho ainda mostra ou esconde o painel; ele vira pausa na Etapa 10.

**Tela de inventário (SELECT):**
- ◀▶ escolhe o espaço;
- ✕ marca e troca de lugar;
- ■ larga o espaço inteiro;
- ▲ divide a pilha ao meio;
- ● fecha.

Com ela aberta, o personagem anda, mas não atira nem interage.

**Medido** (emulador; o número de primitivas vale para o console):
- campo de treino com o combate: CPU de 1,7 a 2,3 ms por quadro (era 1,2 ms na Etapa 4), com loot, bonecos e HUD;
- o HUD usa de 1.500 a 2.600 primitivas 2D, porque a fonte 5×7 desenha cada traço como retângulo (a fonte rasterizada entra na Etapa 10);
- livres no boot: 16,4 MB (16,5 MB na Etapa 5);
- dados de itens: 1.856 bytes.


## 20. Bots (Etapa 7, implementado)

| Peça | Arquivo | Papel (no Ventania) |
|---|---|---|
| Dados | `data/bots.json` + `data/nomes_jogadores.txt` → `assets/dados/bots.bdat` | `game/data/bots.json` e a lista de 1.000 nomes, com os mesmos valores |
| Grafo | `tools/conversor/navegacao.py` → `assets/mapa/ilha.bnav` | a construção do `nav_grade.gd` (células, andares, portas, escadas, rampas), feita offline |
| Navegação | `src/mundo/navegacao.*` | o A* do `nav_grade.gd`: custo, heurística ×1,3, limite de 7.000 expansões, pontos "precisos" |
| Cérebro | `src/ia/cerebro.*` | `bot_cerebro.gd`: percepção, decisão por pontuação, execução, desvio e recuperação |
| Lutador | `src/jogo/lutador.*` | o jogador do servidor: personagem + animador + combatente + item na mão, comandado por entradas |
| Grupo | `src/jogo/grupo_bots.*` | o papel do `game_server.gd`: `add_bot`, orçamentos por passo, `bots_hear`, eliminações, reaparecer |

**Um bot é um jogador.** O cérebro só produz um `ComandoLutador` (movimento,
mira, gatilho, recarga, interação, espaço do inventário), como o `InputCmd` do
Ventania. Movimento, colisão, tiro, loot e inventário são os mesmos sistemas do
jogador; o jogador virou um `Lutador` com câmera. Ordem do passo (30 Hz), a do
servidor do Ventania: **todos os bots pensam** → o jogador anda e atira → os
eventos do jogador → **os bots andam e atiram** → eventos (dano avisa a vítima,
tiro é ouvido a até 70 m, eliminação larga o inventário e entra no feed).

**Decisão por pontuação** (a cada 1/`decisao_hz` s perto do jogador, 1 Hz longe
dele: o LOD de IA do Ventania): combater, cobertura, recuar, perseguir,
investigar, curar, baú, saquear e explorar, com os 21 parâmetros de cada
dificuldade (reação, erro de mira que assenta com o tempo de visada, alcance e
cone de visão, memória, chance de investigar e de buscar cobertura, limites de
vida para recuar e curar, rajadas, exploração de cômodos). A mistura do
`bots.json` reparte as dificuldades entre os 15 bots.

**Orçamentos por passo** (os do Ventania): 24 raios de visibilidade e 2
planejamentos de A*. Quem não ganha orçamento tenta no passo seguinte.

**Andar:** segue os pontos do caminho (precisos: porta, degrau e patamar, a
0,7 m; os outros a 1,7 m), com um raio de desvio local à frente. Sem progresso
por 45 passos: manobra lateral; com 90 ou mais: a aresta onde travou entra na
lista a evitar e o caminho é refeito; na 3ª recuperação, o objetivo é
abandonado (o baú ou item vai para uma lista a ignorar). "Preso" = sem
progresso, contado em `EstatisticasBot`.

**Ficam para as próximas etapas** (o cérebro já tem os parâmetros, que ficam
sem uso até lá): ônibus, queda e Tormenta (Etapa 8); coleta com a picareta e
construção ao levar tiro (Etapa 9). Fora da partida BR, os bots e o jogador
reaparecem em 4 s (`RESPAWN_TICKS` do treino do Ventania).

**BR-ADR-19: grafo de navegação calculado no pipeline.** O Ventania monta o
`NavGrade` ao carregar a ilha. No PS2 isso custaria segundos de CPU e memória
temporária a cada carga. O conversor faz a mesma construção em Python sobre o
layout extraído (os obstáculos e os cômodos passaram a sair do
`extrair_ilha.gd`), e o jogo só carrega 121 KB e roda o A*. Quando a construção
entrar (Etapa 9), as arestas das células tocadas serão atualizadas em jogo
(§7).

**BR-ADR-20: o personagem colide como uma cápsula no topo.** O personagem do
Ventania é uma cápsula. O cilindro de topo reto do Brisa batia a cabeça nas
bordas que só pegam os 20 cm de cima, como o patamar da escada empilhada acima
do 2º lance no prédio: os bots (e o jogador) travavam ao descer. A colisão
lateral agora ignora caixas cuja base esteja nos 20 cm de cima do corpo
(`MundoColisao::empurrar`). Os testes de colisão da Etapa 4 continuam passando.

**BR-ADR-21: no meio de uma escada, a rota começa pelo resto dela.** No
patamar do meio, o nó mais perto é a célula do chão 2 m abaixo, e a rota mandava
pular do patamar (o maior ponto de travamento nas simulações). O A* agora
procura o ponto de escada mais perto (a menos de 1,6 m e 0,8 m de altura) e
começa na ponta da escada que dá o menor custo até o destino, com os pontos
restantes na frente. Um bot cuja lista de arestas evitadas fecha a única saída
de uma sala esvazia a lista e replaneja.

**Determinismo:** o sorteio é PCG32 com semente por bot. No PC a partida é
idêntica a cada execução. O combate e o movimento do roteiro saem iguais no PC
e no PS2, mas **a partida dos bots não**: a FPU do EE não é IEEE (sem
denormais, arredondamento por truncamento), e as decisões em cadeia ampliam a
diferença (no PC vence um bot em 2,7 min; no PS2, outro em 4,8 min). Os dois
cumprem o aceite. Para um jogo offline isso não pesa; os testes conferem
propriedades (vencedor, tempo preso), não o resultado exato.

**Medido** (emulador Play!; os tempos de CPU são do emulador, não do console):
- 15 bots com o jogador, desenhando a ilha: IA de 0,69 a 0,77 ms por passo em média (pensar até 1,34 ms, simular até 1,40 ms);
- partida só de bots, sem desenhar: 0,33 ms por passo em média, pior 5,2 ms (um passo com dois A* longos); 359 caminhos;
- memória: livres no boot 15,7 MB (16,4 MB na Etapa 6); `bots.bdat` 24 KB, `ilha.bnav` 121 KB; estado estático do A* de ~430 KB (custos, origem e marcas de 23.524 nós, mais o heap de 16.384 abertos).

## 21. Partida Battle Royale (Etapa 8, implementado)

| Peça | Arquivo | Papel (no Ventania) |
|---|---|---|
| Dados | `data/partida.json` → `assets/dados/partida.bdat` | `game/data/partida.json`: contagem 10 s, fim 15 s, proteção 3 s, Tormenta de 7 fases |
| Tormenta | `src/partida/tormenta.*` | `tormenta.gd`: espera → fechamento → próxima; a próxima zona cabe inteira na atual, centro em terra e longe do mar |
| Ônibus | `src/partida/onibus.*` | `onibus.gd`: rota reta a 120 m, 20 m/s, passa a até 80 m do centro; portas sobre terra |
| Queda | `src/personagem/movimento.*` | `movement.gd _queda`: queda livre 26 m/s, mergulho 38 m/s (olhando para baixo, −45°), planador abre a 38 m do chão (6,5 m/s, 12 m/s na horizontal) |
| Visual da queda | `src/personagem/animador.*`, `src/jogo/lutador.*` | `pawn_visual.gd _entrada/air_tilt`: queda_livre, mergulho, planador_abrir → planador; de bruços, de cabeça para baixo, inclina nas curvas; a asa delta sobre a cabeça |
| Partida | `src/jogo/partida_br.*` | `partida_br.gd`: estados, assentos, salto/ejeção, dano da Tormenta, colocações, vitória, resultado |
| Bots | `src/ia/cerebro.*` | `bot_cerebro.gd _no_onibus/_caindo/_urgencia_tormenta`: salto perto de um POI ou baú, queda guiada, fuga da Tormenta |
| Tela | `src/jogo/ilha.cpp`, `src/render3d/pipeline_sw.*` | `match_hud.gd`, `storm_visual.gd`, `tela_resultado.gd`, o espectador do `game.gd` |

**Fluxo:** CONTAGEM (10 s, cada um num ponto de início) → ÔNIBUS (loot novo,
inventários só com a picareta, vida cheia; todos nos assentos; ✕ salta com as
portas abertas; quem não salta é ejetado quando elas fecham) → JOGO (a Tormenta
nasce quando o ônibus sai; 3 s de proteção) → FIM (vencedor; o resultado em
15 s). No ônibus, no ar e na proteção nenhum tiro acerta (`GrupoBots::alvos`
devolve zero alvos). O dano da Tormenta é de segundo em segundo, só na vida, do
menos para o mais vivo; se eliminaria todos os que restam, o último é o vencedor.

**Eliminação e espectador:** sem reaparecer. A câmera passa a seguir quem
eliminou o jogador (ou o próximo vivo; ✕ troca). A tela de resultado abre 2 s
depois da eliminação ou no fim: colocação (`#N DE 16` ou `VITÓRIA!`), eliminações,
tempo vivo, dano causado, baús abertos e curas usadas (as estatísticas de cada
`Lutador`). ✕ joga de novo (outra semente), ▲ volta a assistir. Materiais e
construções entram na Etapa 9; XP e passe, na 11.

**HUD:** contagem e ônibus no meio da tela; "SALTAR" com o símbolo ✕ e os
avisos da queda; a fase e o tempo da Tormenta abaixo do minimapa; a zona atual
(roxa) e a próxima (branca) no minimapa e a rota tracejada do ônibus; a tela
roxa e "VOCÊ ESTÁ NA TORMENTA! (-N/S)" fora da zona; "ASSISTINDO X".

**BR-ADR-22: 16 participantes (o jogador + 15 bots).** O BR-ADR-6 começava com
12 e mirava 16, a decidir por medição. Na Etapa 7, a IA dos 15 bots custou de
0,7 a 0,8 ms por passo no emulador, desenhando a ilha. A Etapa 8 confirma
com a partida inteira (§21, "Medido"). Fica 16, o mesmo número com que o Ventania
completa as partidas. O teste no console pode baixar esse número, se o custo
real passar do orçamento.

**BR-ADR-23: parede da Tormenta translúcida, sem textura e sem neblina.** O
`storm_visual.gd` usa um shader com faixas animadas. O Brisa desenha um
cilindro de 64 segmentos em três faixas, com alfa por vértice (o perfil do
shader: entra até 8% da altura, cheio até 75%, sai no topo; e um pouco mais
fraco de longe), recortado contra o plano perto e a banda de guarda, por
último e com teste Z (`projetar_parede`). São no máximo 384 triângulos com
alfa: no PS2, pelo caminho das sombras blob; no PC, pelo rasterizador. A
parede passa da neblina de propósito, para ser vista de longe.

**BR-ADR-24: a ilha abre numa partida.** Até o lobby (Etapa 10), o ELF começa
direto numa partida BR com 15 bots, e "Jogar de novo" começa outra com outra
semente. O treino livre da Etapa 7 (bots que reaparecem, sem Tormenta) continua
com `--treino-livre=1`. O roteiro do autoteste joga uma partida do começo ao fim
(`src/jogo/roteiro_partida.h`, o mesmo dos testes no PC). Como atalho, a
Tormenta elimina o jogador 3 s depois de nascer, para chegar ao espectador e ao
resultado sem esperar minutos.

**BR-ADR-25: A* recusa na hora um destino sem ligação, e o bot espera para
replanejar.** Os bots pousam de planador em telhados e andares sem escada (65
dos 111 andares do grafo estão fora da componente principal) e, às vezes, na
água rasa. No Ventania, cada tentativa de rota dali esgota as 7.000 expansões,
e o bot tenta de novo no passo seguinte. No emulador isso deu passos de 46 ms
(dois A* esgotados, ~23 ms cada). Mudanças:
1. A carga rotula as componentes conexas do grafo uma vez (busca em largura pelas mesmas arestas do A*). Uma rota cuja origem e destino estão em componentes diferentes falha sem expandir nada (`Navegacao::sem_ligacao`).
2. Depois de uma rota que falhou, o cérebro espera meio segundo antes do próximo A* e anda direto para o alvo, o que tira o bot do telhado ou da água.

Nas 12 sementes do PC, o pior passo caiu de 7.006 para 1.941 expansões. A
decisão dos bots é a mesma do Ventania; só o custo de uma falha mudou.

**Medido** (emulador Play!; os tempos de CPU são do emulador, não do console):
- partida com o jogador e 15 bots: CPU de 2,4 a 3,9 ms por quadro, 3D de 0,8 a 3,7 ms, 59,8 FPS;
- IA de 0,46 a 0,72 ms por passo em média (pior passo de pensar 1,53 ms, de simular 1,0 ms);
- partida só de bots, sem desenhar: 0,39 ms por passo, pior 4,9 ms;
- livres no boot: 15,6 MB; `partida.bdat`: 176 bytes; componentes do grafo: 46 KB estáticos.
