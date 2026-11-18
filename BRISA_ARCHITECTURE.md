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
- **Colisão do personagem:** cilindro vertical (raio 0,35 m, altura 1,8 m; agachado 1,2 m) contra o relevo, as caixas finas das peças (paredes de 0,24 m, pisos com topo a 0,10 m), planos inclinados (rampas), cilindros ou caixas dos props e a colisão simplificada das escadas.
- **Tiro:** raio por DDA na grade (como um voxel traversal) e, depois, contra as hitboxes (cabeça e corpo) dos personagens próximos ao raio.
- **Navegação dos bots:** o grafo estático (150×150 células de 4 m mais nós de andar, escadas e portas) sai **pronto do pipeline**. Em jogo, só se atualizam as arestas das células tocadas por construção ou destruição. A* com limite de expansões por tick.

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
| Malha | `src/render3d/malha.*` | Lista de triângulos em SoA no formato do VU1 (posição V4-32, normal V4-8 com sinal, cor V4-8), geradores procedurais com normal plana |
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
