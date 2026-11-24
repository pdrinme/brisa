# Roadmap — Brisa (PlayStation 2)

Legenda: ✅ concluído e testado · 🟡 em andamento · ⬜ não iniciado · ⏳ aguarda
teste em console.
**Nada é marcado ✅ sem teste executado. Desempenho só conta medido em console.**
Uma etapa por vez; cada uma termina com o relatório-padrão (o que foi feito,
arquivos criados e modificados, assets reaproveitados, testes, desempenho,
problemas e próxima etapa).

## Marcos

- **B1 — "Roda no PS2"** (Etapas 1–3): ELF que inicia, desenha 3D no VU1 e mostra assets do Ventania convertidos.
- **B2 — "Uma partida"** (Etapas 4–9): personagem, ilha, combate, bots, Battle Royale e construção.
- **B3 — "Um jogo"** (Etapas 10–12): menus, lobby, progressão, passe, loja e Memory Card.
- **B4 — "Pronto para distribuir"** (Etapa 13).

## Etapas

### Etapa 0 — Estudo do Ventania e planejamento ✅
- ✅ Ventania clonado (somente leitura, commit `7864cd7`, v1.2.1). Lidos os 11 documentos obrigatórios, os dados (`game/data`, `backend/config`), os geradores de assets, os 90 GLB, o código dos sistemas e as imagens de `docs/art/`.
- ✅ Estudo sistema a sistema, com os 6 pontos de cada um e o estado real do código: `docs/estudo/SISTEMAS_VENTANIA.md`. Inclui 9 divergências entre documentos e código no Ventania.
- ✅ Auditoria automática dos assets (`tools/auditoria/auditar_ventania.py`): 90 GLB, 52.420 triângulos visuais, todos "Original"; os 11 personagens com skinning rígido de 20 ossos e 16 animações compartilhadas.
- ✅ Lista de reaproveitamento arquivo a arquivo: 71 diretos, 11 com LODs, 7 com simplificação e 1 descartado (`docs/auditoria/REUSO_GLB.md`).
- ✅ Extração do layout real da Ilha Ventania (Godot headless numa cópia): 646 peças, 1.219 props, 37 baús, 102 loot. A extração é reproduzível byte a byte, e o Ventania continuou intacto (`git status` limpo).
- ✅ Densidade do mapa por setor (dimensiona LOD, culling e distância de visão).
- ✅ Toolchain PS2DEV validado (GCC 15.2, PS2SDK, gsKit, IRX): `verificar_toolchain.sh` → `TOOLCHAIN_OK`; ELFs de teste compilados.
- ✅ Documentos: README, BRISA_ARCHITECTURE, BRISA_GAME_DESIGN, BRISA_ROADMAP, ASSET_LICENSES, PIPELINE_ASSETS, FERRAMENTAS_PS2DEV, ESTILO_VISUAL_PS2, SINCRONIA_VENTANIA.
- ⬜ Pendente com o dono: (1) autorização formal de redistribuir os assets do Ventania no Brisa (ASSET_LICENSES §2); (2) as imagens de referência citadas no pedido; (3) um repositório remoto para o Brisa (o contêiner é efêmero).
- Correção posterior (Etapa 1): o emulador Play! roda ELFs sem BIOS; a Etapa 1 passou a usá-lo.
- Não executado: nenhum ELF rodou (não há emulador com BIOS nem console aqui).

### Etapa 1 — Primeira execução no PS2 🟡 (pronta e testada no emulador; ⏳ falta o teste no console)
**Feito:**
- `Makefile` próprio (`make`, `make autoteste`, `make testes`, `make previa`, `make emulador`, `make emulador-completo`) com dependências de cabeçalho (`-MMD`).
- Estrutura `src/`: `nucleo/` (portátil, sem PS2SDK), `jogo/` (a tela), `plataforma/ps2` e `plataforma/host`, `main.cpp`.
- **Núcleo:**
  - tipos;
  - arena (alocador linear);
  - passo fixo de 30 Hz (máximo 2 passos por quadro, descarte do excesso);
  - controle independente de plataforma (bordas entregues por passo: um toque nunca se perde nem conta duas vezes);
  - estatísticas de quadro;
  - formatação com vírgula decimal;
  - lista de desenho 2D;
  - **fonte bitmap própria** 5×7, com os acentos do português e o estilo "adesivo" (contorno por retângulos dilatados).
- **Plataforma PS2:**
  - reinício do IOP e patches SBV;
  - módulos do controle da ROM (XSIO2MAN/XPADMAN → SIO2MAN/PADMAN), com os IRX do PS2SDK embutidos no ELF como alternativa;
  - gsKit no modo do console (NTSC 640×448 ou PAL 640×512, entrelaçado, buffer duplo, sem Z);
  - DualShock 2 em modo analógico travado;
  - relógio pelo timer do barramento;
  - medição da memória livre (EndOfHeap − sbrk, confirmada por malloc).
- **Plataforma PC:** tempo simulado, controle por roteiro, rasterizador de software que imita o GS (alfa 0..128, Gouraud, quads em tira) e PNG.
- **Tela da Etapa 1**, na linguagem visual do Ventania:
  - fundo azul-petróleo → índigo com rajadas;
  - BRISA em adesivo;
  - quadrado girando (✕ cor, L1/R1 tamanho, analógico direito velocidade, START pausa);
  - estado completo do DualShock 2;
  - rodapé com quadro, CPU, FPS, memória, programa e vídeo.
- **Ferramentas de emulador** (Play!, sem BIOS): AutoTest e o capturador próprio `tools/emulador/captura_play` (GS OpenGL em Mesa/Xvfb, controle simulado pela SIO2), mais `instalar_emulador.sh`.
- `docs/TESTE_NO_CONSOLE.md`: o roteiro do teste no PS2 real e o que reportar.

**Testes executados:**
- `make testes`: 16 testes, ~5 mil verificações, 0 falhas. Cobrem arena, passo fixo, toques, zona morta, bits iguais aos do libpad, estatísticas, formatação, fonte completa, limite da lista, e a tela (giro de 90°/s, pausa, cor, tamanho, área segura, cores do tema rasterizadas).
- `make emulador`: o ELF real no Play! com vídeo e controle simulado → `EMULADOR_OK`. Os módulos da ROM carregam, o controle chega ao estado estável e é lido em modo analógico, e todos os botões e eixos do roteiro aparecem na imagem. Capturas em `docs/etapa1/`.
- **Fidelidade do desenho:** a imagem do GS no emulador e a prévia do PC são idênticas no cartão do controle (0 de 66.216 pixels diferentes).
- `make emulador-completo`: 600 quadros até o fim do programa → `EMULADOR_OK`.
- **Estabilidade no emulador:** 36.000 quadros (10 min de tempo emulado a 59,8 FPS) até `BRISA_FIM_OK`, sem travar. O tempo de quadro ficou constante (16.718–16.719 µs), a CPU teve pior caso de 1.444 µs, e nenhuma primitiva foi descartada.

**Medido (no emulador Play!; não vale como medição de console):**
- quadro de 16,7 ms (59,8 FPS, NTSC);
- CPU média de 1,44 ms por quadro (pior 1,44);
- ~2.450–2.470 primitivas por quadro, 0 descartadas;
- 28,0 MB livres no boot (heap de 0x3d3000 a 0x1fdf000);
- programa de 572 KB (texto + dados + bss);
- ELF de 240 KB.

**Problemas encontrados e resolvidos:**
- os tipos `u32/u64` conflitavam com os do PS2SDK (o núcleo agora usa `tamtypes.h` no EE);
- objetos velhos sem dependência de cabeçalho (`-MMD`);
- a primeira medição de memória era impossível (32,7 MB) e foi trocada por EndOfHeap − sbrk;
- o PADMAN embutido fica preso em `PAD_STATE_EXECCMD` no Play! (a ordem passou para ROM primeiro);
- `padPortClose`/`padEnd` não retornam no Play! com o PADMAN embutido (a saída não chama RPC do controle);
- contorno do texto com 18 cópias estourava a lista (agora são 3, com retângulos dilatados);
- círculo com faixas por sobreposição de partes translúcidas.

**⏳ Falta para o aceite:**
- rodar no **console real** (pendrive ou ps2link) e conferir o roteiro de `docs/TESTE_NO_CONSOLE.md`, inclusive 10 min sem travar;
- conferir no console qual caminho de módulos é usado e se o modo PAL funciona.

**Não feito (fora do escopo da etapa):** PCSX2. Ele exige o dump da BIOS do console do dono, que não existe neste ambiente; o Play! foi usado no lugar, com a BIOS em alto nível.

### Etapa 2 — Motor gráfico 🟡 (pronto e testado no emulador; ⏳ falta o benchmark no console)
**Feito:**
- **Núcleo 3D portátil:**
  - `Vec3`/`Mat4`, perspectiva, câmera, inversa rígida e frustum por planos;
  - malhas em SoA no formato do VU1, com geradores procedurais (caixa, cilindro, cone, pirâmide, esfera geodésica, grade de chão);
  - cena com câmera, **luz toon de 2 faixas com sombra azulada** (a fórmula do `toon_paleta.gdshader` do Ventania) e neblina;
  - pipeline de referência com recorte no plano perto e na faixa de guarda do GS e descarte de faces traseiras.
- **Caminho A (EE):** triângulos → GIFtag PACKED RGBAQ+XYZF2 → DMA do GIF em buffer duplo.
- **Caminho B (VU1):**
  - microprograma próprio `vu1/toon.vsm` (transformação, luz toon, Z, neblina, recorte por CLIP/ADC, XGKICK);
  - cadeia de DMA do VIF1 que só referencia as malhas;
  - lotes de 96 vértices em buffer duplo;
  - objetos que cruzam o plano perto vão pelo EE (BR-ADR-9).
- **Z-buffer** de 24 bits (GEQUAL), **neblina do GS** por vértice, material = cor da paleta por vértice.
- **Cena de teste** na paleta do Ventania:
  - objetos: chão com relevo, casas de tábuas, torres de pedra, galpões de metal, pinheiros, árvores redondas, baús azul-petróleo/amarelo e pedras;
  - 7 níveis de densidade (24 a 1.536 objetos) e 5 de "carga" (esferas de 1.280 triângulos);
  - câmera pelos analógicos, SELECT troca o caminho, ▲ roda o **benchmark automático** (12 configurações × 2 caminhos, com linhas `BRISA_BENCH` no log e uma tabela na tela).
- Rasterizador do PC com Z e neblina iguais aos do GS, para as prévias e os testes.

**Testes executados:**
- `make testes`: 25 testes, 0 falhas. Os 9 novos cobrem perspectiva, câmera e inversa, frustum, normais para fora em todos os geradores, normal empacotada, luz toon, faces traseiras (6 de 12 num canto do cubo e 0 por dentro), recorte no plano perto, Z "maior ou igual" e neblina no rasterizador, e os níveis e o benchmark da cena.
- `make emulador` (Play!, GS real em OpenGL): `EMULADOR_OK` nas duas execuções, a da cena (VU1 e EE) e a do controle.
- **Os dois caminhos geram a mesma imagem:** na mesma cena parada (11.238 triângulos), as capturas do VU1 e do EE diferem em **8 de 170.240 pixels** (bordas). O EE do emulador comparado com a referência do PC difere em 0,28% dos pixels (regras de borda). Capturas em `docs/etapa2/`.
- `make emulador-completo` (AutoTest, benchmark reduzido a 3+6 quadros por configuração): `EMULADOR_OK`. Roteiro VU1 e EE, as **24 medições** e o fim do programa sem travar.

  | Configuração | Triângulos | 3D pelo EE (emulador) | 3D pelo VU1 (emulador) |
  |---|---|---|---|
  | Cena nível 1 | ~3.300 | 23 ms | 12 ms |
  | Cena nível 4 | ~11.000 | 87 ms | 13 ms |
  | Cena nível 7 | ~76.000–79.000 | 646 ms | 20 ms |
  | Carga nível 5 | 34.048 | 295 ms | 14 ms |

  No emulador, o VU1 quase não varia com a carga: o Play! não cronometra o VU1 nem o GS de forma realista. **Estes números não servem para decidir nada**; servem só para mostrar que o benchmark mede e termina.

**Medido no emulador (NÃO vale como desempenho de console):** na cena de
11.238 triângulos (121 objetos visíveis, 361 lotes), o 3D levou 13,1 ms pelo
VU1 e 93,1 ms pelo EE. A diferença aponta na direção esperada (o VU1 alivia o
EE), mas a proporção real só o console dá.

**Problemas encontrados e resolvidos:**
- no caminho do VU1, o Z ia 16× menor que o do EE (escala errada para o XYZF2) e os objetos sumiam atrás do chão;
- no caminho do EE, triângulos perto da câmera podiam sair da faixa de coordenadas do GS (agora há recorte na faixa de guarda, igual ao critério do VU1);
- arredondamento no rasterizador do PC;
- o benchmark do autoteste foi reduzido (3+6 quadros por configuração), porque o Play! é muito lento no caminho do EE.

**⏳ Falta para o aceite:**
- o **benchmark no console** (▲ na cena; `docs/TESTE_NO_CONSOLE.md`, Etapa 2);
- com os números, registrar a decisão do BR-ADR-1 e o orçamento de triângulos por quadro a 30 FPS.

**Também continua pendente:** o teste da Etapa 1 no console.

**Não incluído (próximas etapas):** texturas (a sombra blob e as texturas CLUT entram com o conversor de texturas, na Etapa 3), descarte de faces traseiras no VU1 e otimização do microprograma (quando o console mostrar onde está o gargalo).

### Etapa 3 — Pipeline Ventania → Brisa 🟡 (pronta e testada no emulador; ⏳ falta ver no console)
**Feito:**
- **Conversor** (`tools/conversor/`, só com a biblioteca padrão do Python): GLB do Ventania → formatos próprios (`docs/FORMATOS.md`):
  - **BMOD**: nós, partes rígidas, marcadores, colisões, posições, normais e cores da paleta;
  - **BANI**: clipes a 30 Hz, quaternions int16;
  - **BTEX**: PSMT8/PSMT4 com CLUT no arranjo do GS;
  - **BPAK**: pacote com índice e CRC32.
- **Importados do Ventania (`7864cd7`):**
  - os 89 modelos reaproveitáveis (14 peças de construção, 51 de cenário e mobília, 7 armas, 7 coletáveis, 11 trajes, 2 acessórios);
  - o banco de 16 animações do esqueleto padrão e a animação do baú;
  - os 11 ícones do inventário.
  - Ficou de fora: `caixote_exemplo_01`.
- **Redução de polígonos** (QEM por parte rígida, preservando bordas e cores):
  - 7 modelos acima do orçamento caíram para o LOD0 permitido;
  - os 11 trajes ganharam LOD1 (~1.000 triângulos) e LOD2 (~290).
- **Validações:** skinning rígido, UV dentro da paleta, esqueleto igual em todos os trajes, partes alinhadas para o VIF, orçamentos.
- **Catálogo do Brisa** (`assets/catalogo_brisa.json`, 125 arquivos com origem, autor, licença, commit e CRC); o empacotador **recusa** arquivo sem registro.
- **No jogo:** leitores sem cópia, pose e animação (interpolação, laço), modelo → cena por parte, sombras blob texturizadas, sprites com textura no 2D, pacote embutido no ELF (BR-ADR-10).
- **Vitrine do kit** reproduzindo `vitrine_kit.gd` do Ventania (posições, rotações, câmera, sol, faixa de luz). Controles:
  - L1/R1 troca entre os 11 trajes e R2 troca o LOD;
  - ○ percorre as 16 animações e SELECT troca entre VU1 e EE;
  - START mostra e esconde o HUD, e os analógicos giram a câmera.
  - A cena de benchmark da Etapa 2 continua em L3 + R3.

**Testes executados:**
- `python3 tools/conversor/testes.py`: 15 testes OK (19 a partir da Etapa 4). Cobrem matemática, QEM (alvo, sem virar faces, cores preservadas), CSM1, pacote (ordem, alinhamento, CRC), PSMT4, sombra, quantização, Recruta (rígido, 2.790 triângulos, coral do cachecol, 16 clipes normalizados), baú (2 partes, marcador, colisão de 1,1 m), marcadores da picareta, alinhamento das partes e a recusa do empacotador.
- `make testes`: 32 testes, 0 falhas. Os 7 novos cobrem CRC32, pacote íntegro e ordenado, pacote corrompido recusado, partes alinhadas a 16 bytes, animações servindo a 4 trajes, a mão subindo no aceno, o laço contínuo, a tampa do baú abrindo e as texturas.
- `make emulador`: `EMULADOR_OK` (cena e controle). `make emulador-completo`: `EMULADOR_OK`, com VU1 e EE, **os 11 trajes × 3 LODs e os 16 clipes** desenhados e o fim sem travar.
- **Imagens:**
  - VU1 × EE no emulador: 3 de 152.320 pixels diferentes;
  - emulador × PC: 0,88% (bordas finas e filtragem bilinear da sombra);
  - comparação lado a lado com a vitrine do Ventania em `docs/etapa3/ventania_x_brisa_emulador.png`.

**Medido** (bytes valem para o console; tempos são do emulador):
- pacote de 5,0 MB (125 arquivos): modelos 4,6 MB, animações 228 KB, texturas 56 KB;
- vitrine com 50 modelos carregados (3,6 MB, sem cópia) e 8.950 triângulos na cena;
- memória livre no boot: 18,3 MB (era 28 MB na Etapa 1);
- ELF de 5,4 MB;
- conversão completa em 13 s.

**Problemas encontrados e resolvidos:**
- **alinhamento do VIF:** as partes precisam começar em múltiplo de 4 vértices, e o UNPACK V4-8 precisa consumir qwords inteiros (sem isso, bytes de dados seriam lidos como comandos do VIF);
- a assinatura do esqueleto incluía o nome da malha de cada traje;
- luz mais escura que a da Godot (calibrada, BR-ADR-12);
- um teste supunha errado o movimento do marcador da tampa.

**Problema aberto:** os modelos passam do orçamento de memória (4,6 MB contra 3 MB) por causa da lista de triângulos não indexada. As correções estão em BRISA_ARCHITECTURE §16 (tiras, int16, carregar só o necessário), antes da Etapa 5.

**Não incluído:** compilador de dados de jogo (entra na Etapa 6, com o primeiro sistema que lê esses dados), leitura do pacote a partir do USB (BR-ADR-10), transparência (vidro e água do Capitão Maré estão opacos) e sombras projetadas de verdade (o Brisa usa blob, por projeto).

**⏳ Falta para o aceite:** rodar a vitrine no console e comparar na TV (`docs/TESTE_NO_CONSOLE.md`, Etapa 3).

### Etapa 4 — Personagem 🟡 (pronta e testada no PC e no emulador; ⏳ falta jogar no console)
**Feito:**
- **Campo de treino do Ventania** (`world/arena/campo_treino.gd`) como cena inicial do ELF (`src/jogo/treino.*`):
  - as 19 peças de construção, os 7 baús e os 40 props nas posições do Ventania (os props saem de `tools/auditoria/extrair_campo_treino.gd`, com a mesma semente 42 do RNG da Godot);
  - as paredes invisíveis da borda (±110 m) e o ponto inicial (4, 0, 10).
- **Personagem jogável** com os 11 trajes do Ventania (SELECT troca), sombra blob e os números de `pawn_body.gd`/`player_movement.gd` (`src/personagem/movimento.*`):
  - andar 4,5 m/s, correr 6,5, agachado 2,5;
  - gravidade 22 m/s², salto 7,4 m/s (1,37 m), aceleração 45 no chão e 9 no ar, queda máxima 45 m/s;
  - cilindro de raio 0,35 m e altura 1,8 m (agachado 1,15), degrau de 0,35 m e "grudar" no chão ao descer;
  - velocidade constante ao longo do piso nas rampas;
  - sem levantar do agachado se não houver espaço.
- **Colisão** (`src/mundo/colisao.*`, §17): caixas e **rampas** (o conversor classifica as colisões do Ventania, BMOD v2), giradas pela instância, numa grade de 8 m. Faz chão, paredes, teto, "cabe aqui?" e raio.
- **Câmera em 3ª pessoa** de `player_camera.gd` (`src/personagem/camera3p.*`):
  - ombro de 0,55 m, 3,2 m de distância (1,7 m mirando com L2);
  - FOV de 70°/55°, pitch de −80° a 70°, 135°/s no analógico direito;
  - braço de mola que encolhe na parede e estica devagar.
- **Animador** (`src/personagem/animador.*`), a árvore do `pawn_visual.gd`:
  - idle/andar/correr misturados pela velocidade, com a fase sincronizada;
  - agachado e agachado andando;
  - pulo sobre a queda, aterrissagem, aterrissagem pesada (queda ≥ 6 m) e aceno (◀ no direcional).
- **Dano de queda: não tem** (BR-ADR-13), como no Ventania.
- **Roteiro do autoteste** (`src/jogo/roteiro_treino.h`): o mesmo no PC e no PS2. Sobe a torre pelas duas rampas, cai 8 m, pula, gira a câmera, anda agachado, corre, troca VU1 → EE e acena. Cada marco vai para o log (`BRISA_MARCO …`).
- **Modos:** L1 + R1 + START alterna treino → vitrine (Etapa 3) → benchmark (Etapa 2).

**Testes executados:**
- `make testes`: 43 testes, 5.387 verificações, 0 falhas. Os 11 novos (`tests/teste_personagem.cpp`) cobrem:
  - as velocidades, a aceleração e a direção pelo yaw da câmera;
  - o salto (1,2–1,45 m, ~0,67 s) e a queda presa a 45 m/s;
  - parede, degrau de 0,3 m (sobe) e obstáculo de 0,5 m (bloqueia), além do deslizar numa parede girada 45°;
  - teto baixo (agachado passa e não levanta embaixo; o pulo para no teto) e o raio contra caixa girada e o chão;
  - a câmera: posição do ombro, braço de mola, mira, agachado, velocidade e limites;
  - o campo carregado (3 rampas, pisos a 4,1 m e 8,1 m) e a passagem agachada sob os pisos de madeira a 1,5 m;
  - os estados do animador e a pose finita;
  - o **roteiro completo** com as mesmas expectativas do emulador.
- `python3 tools/conversor/testes.py`: 19 testes OK. Os 4 novos cobrem caixa, rampa nos 4 sentidos, cunha → caixa e a rampa de pedra do Ventania no pacote (tipo −z, laje de 0,141 m). Foi corrigido o teste do baú, que ainda lia a colisão do formato v1.
- O teste da câmera achou um erro: a margem de 0,2 m era descontada mesmo sem nada no caminho. Corrigido.
- `make emulador`: `EMULADOR_OK` (cena e controle). `make emulador-completo`: `EMULADOR_OK` (16 verificações).
- **PC × PS2 (Play!):** os 10 marcos do roteiro saem **iguais** (posição com 2 casas, estado, animação, yaw da câmera). Por exemplo:
  - topo da torre em (−0,15; 8,10; −25,90);
  - aterrissagem com queda de 8,05 m;
  - fim em (5,19; 0,00; −5,42).
- **Imagens** (`docs/etapa4/`): a rampa, o topo da torre pelo VU1, o aceno pelo EE e a tira da prévia no PC.

**Medido** (bytes valem para o console; **tempos são do emulador**, não do console):
- campo com 62 colisores e 91 objetos na cena (25 a 59 visíveis), com 3.670 a 9.224 triângulos enviados;
- caminho VU1: 3D de 1,3 a 2,6 ms e quadro de 16,7 ms (59,8 FPS); CPU (lógica + montar a cena) de 1,23 ms;
- caminho EE: 3D de 43 ms com 5.170 triângulos (35 FPS no emulador);
- ELF de 5,4 MB (9,9 MB na RAM com o bss) e 18,1 MB livres no boot;
- mundo de colisão: ~545 KB de bss (2.048 colisores e a grade de 64 × 64 × 48).

**Problemas encontrados e resolvidos:**
- no alto da primeira rampa, o piso seguinte (topo a 4,1 m) virava parede. Agora, sobre rampa, o degrau conta a partir do chão na frente do cilindro;
- faltava o campo `y` no `.inc` dos props;
- a margem do braço de mola (acima);
- a linha de ajuda do HUD passava da borda da tela;
- a captura do emulador encerra o programa no último quadro salvo. O fim do roteiro é conferido no `emulador-completo`.

**Limitações conhecidas (vão para as próximas etapas):**
- as escadas e os telhados em cunha ainda colidem como caixa (AABB). As escadas da ilha entram na Etapa 5, com o mapa;
- o campo é plano: o relevo da ilha entra na Etapa 5;
- o caminho EE é lento demais para o jogo (43 ms só de 3D no emulador). Ele fica como alternativa e diagnóstico, e o VU1 é o caminho do jogo (a confirmar no console, BR-ADR-1);
- o problema de memória dos modelos (Etapa 3) continua aberto e é a primeira tarefa antes do mapa.

**⏳ Falta para o aceite:** jogar o campo de treino no console (`docs/TESTE_NO_CONSOLE.md`, Etapa 4).

### Etapa 5 — Mapa 🟡 (pronta e testada no PC e no emulador; ⏳ falta jogar no console)
**Feito:**
- **Memória dos modelos (pendência da Etapa 3):** posições em int16 com escala por modelo (BMOD v3, BR-ADR-14). O VU1 lê V4-16 e converte com ITOF0. Os mesmos modelos foram de 4,6 para 3,1 MB, e o vértice caiu de 24 para 16 bytes.
- **A Ilha Ventania inteira** (o layout do gerador do Ventania `7864cd7`, extraído sem alterar o Ventania):
  - relevo 201×201 com cores do chão do `terreno.gdshader` e mar;
  - 646 peças da grade (paredes com porta, janela e vitrine recortadas por ladrilho, como no `grade.gd`), 1.219 props (vegetação, pedras, mobília, contêineres, escadas), 37 baús;
  - estradas pré-malhadas com faixas, calçadas com meio-fio e faixas de pedestres;
  - 6 regiões com nome no HUD, 24 pontos de início.
- **Desenho** (§18):
  - relevo com LOD1 gerado na carga e LOD0 em cache em volta da câmera, sem frestas entre os dois (BR-ADR-15);
  - corte por setor de 30 m, distância máxima por categoria (móveis só até 24 m) e cadeia de LODs por QEM (peças: LOD1 a 45 m e LOD2 a 110 m);
  - neblina de 60 a 240 m.
- **Colisão:**
  - o relevo (altura bilinear, igual à do Ventania) e as 2.727 caixas, rampas e **cunhas** dos modelos, com escala;
  - o limite da costa do Ventania (160 caixas a 9 m da praia);
  - grade compacta montada uma vez (4.461 referências);
  - as **escadas em U sobem** até o andar de cima (BMOD v4).
- **HUD:** região atual, minimapa em volta do jogador (textura 256×256 recortada por UV) e mapa completo com os nomes dos POIs (▲ no direcional).
- **Decisão do tamanho da ilha (BR-ADR-16):** a ilha inteira, com as condições do console (ver BRISA_GAME_DESIGN §6).
- **Roteiro do autoteste da ilha:** 16 vistas medidas (cada POI chegando e de dentro, e o Mirante nas 4 direções) e uma caminhada correndo sobre o relevo. O ELF abre na ilha; L1 + R1 + START alterna ilha → treino → vitrine → benchmark.
- **Corrigido no caminho:** o Makefile não remontava o microprograma do VU1 quando o `.vsm` mudava (a regra não tinha dependência). O `make emulador` passava mesmo com o VU1 sem desenhar nada, porque só contava lotes; agora ele **compara a imagem** do emulador com a do PC no mesmo passo.

**Testes executados:**
- `make testes`: 53 testes, 18.603 verificações, 0 falhas. Os 9 novos (`tests/teste_mapa.cpp`, mais um em `teste_3d.cpp`) cobrem:
  - o mapa com os números do Ventania, regiões e costa;
  - setores e cadeia de LODs;
  - o relevo sem frestas entre LODs;
  - o jogador no relevo;
  - a **porta passa e a parede inteira bloqueia**;
  - a costa segura quem corre para o mar;
  - o corte com orçamento (pior vista abaixo de 50 mil triângulos) e móveis só de perto;
  - **subir a escada em U** em 3 prédios;
  - o minimapa e a quantização int16.
- `python3 tools/conversor/testes.py`: 21 testes OK (novos: posições int16 dentro da AABB e a decomposição da escada em cunha, patamar e laje).
- Extração da ilha: duas execuções idênticas byte a byte. Conversão reprodutível (o mesmo pacote byte a byte), em 12,4 s.
- `make emulador`: `EMULADOR_OK`, com comparação de imagem PS2 × PC: treino 0,15%, Vila Brisa 0,87%, Mirante 0,61% dos pixels.
- `make emulador-completo`: `EMULADOR_OK`, com as **16 vistas da ilha** e a caminhada (no chão, 0,00 m acima do relevo). Os números de cada vista (triângulos, objetos, instâncias, LODs) saem **iguais** no PC e no PS2.
- Imagens em `docs/etapa5/`.

**Medido** (contagens e bytes valem para o console; **tempos são do emulador**):
- **Triângulos por vista:**
  - pior caso: chegando na Vila Brisa, 41.893 (460 objetos);
  - Vila do centro: 38.340;
  - outros POIs: 9.815 a 28.231;
  - Mirante (4 direções): 14.478 a 25.915;
  - o relevo é 2,7 a 4,9 mil disso.
- O corte por quadro olha de 98 a 964 instâncias e põe 81 a 523 na cena; leva 0,5 a 1,3 ms do EE emulado.
- **Memória:**
  - pacote de 4,4 MB (231 arquivos);
  - ELF de 4,9 MB (11,5 MB na RAM com a arena de 4 MB);
  - **16,5 MB livres no boot**;
  - ilha em jogo: arena de 1,2 MB (relevo 888 KB) e colisão de 341 KB.
- Carga da ilha: 72 ms no emulador.

**Problemas encontrados e resolvidos:**
- o microprograma não era remontado (Makefile), e por isso o VU1 desenhava inteiros como se fossem floats (nada na tela). A verificação que deveria pegar isso só contava lotes; agora compara imagens;
- a extensão padrão da quantização (64 m) estourava no chão de 242 m do treino;
- as paredes com aberturas ficaram com 3–4× os triângulos depois do recorte; a QEM as voltou para ~1,7×;
- a primeira vista "chegando" caía dentro de uma casa;
- as peças de construção dominavam a Vila (50 mil triângulos), o que levou ao LOD2 e aos móveis até 24 m;
- os lances das escadas viravam caixas de 2 m (os andares de cima ficavam inacessíveis), o que levou às cunhas do BMOD v4;
- um teste supunha que 495 era porta (é janela; a porta é 493).

**Limitações conhecidas:**
- os **tempos** de desenho e de corte só valem medidos no console; a decisão do tamanho da ilha fica condicionada a eles (BR-ADR-16);
- sem culling de oclusão (paredes internas da Vila são desenhadas);
- o VU1 ainda não descarta faces traseiras;
- telhados em pirâmide colidem como bloco;
- o relevo é sempre andável (sem limite de inclinação);
- ainda não há loot nos pontos de loot (Etapa 6), nem o grafo de navegação (Etapa 7).

**⏳ Falta para o aceite:** jogar a ilha no console e mandar as medições (`docs/TESTE_NO_CONSOLE.md`, Etapa 5).

### Etapa 6 — Combate e inventário 🟡 (pronta e testada no PC e no emulador; ⏳ falta jogar no console)
**Feito** (detalhes em BRISA_ARCHITECTURE §19):
- **Compilador de dados de jogo** (`tools/conversor/dados.py`): `data/itens.json`, uma cópia revisada do Ventania com os mesmos valores, vira `dados/itens.bdat`, com validação. `make dados` recompila sem o Ventania.
- **As regras do Ventania** portadas e testadas:
  - 5 armas e 6 raridades (o Mítico nunca é sorteado);
  - 4 tipos de munição, 4 consumíveis e a picareta permanente;
  - inventário de 5 espaços com pilhas, reservas e troca só explícita (nada some nem duplica);
  - vida 100 + escudo, com o escudo primeiro;
  - tiro pelo ombro com dispersão por estado, projéteis determinísticos e hitboxes de corpo e cabeça;
  - cadência, recarga e uso em passos;
  - picareta com um impacto por golpe e combo A/B;
  - baú segurando ▲ (arma + a munição dela + consumível + munição);
  - munição automática a 1,3 m e itens largados.
- **Item na mão** com IK de dois ossos (BR-ADR-17): as 3 poses de arma, o consumível e a picareta, mais a mira, o recuo, o sacar, as recargas (carregador, cartuchos da espingarda, pistola), os golpes e o uso de cada consumível.
- **Mundo:**
  - loot no chão girando sobre o anel da raridade, saltando do baú ao abrir;
  - a tampa do baú animada pelo clipe "abrir";
  - no treino, os 17 pontos de loot, 7 baús que reabastecem em 45 s e os 3 bonecos de treino;
  - na ilha, os 102 pontos de loot e os 37 baús da partida (BMAP v2).
- **HUD:**
  - vida e escudo, barra de itens com os ícones do Ventania e a cor da raridade, nome completo e munição;
  - barra de ação (recarga, uso, baú) e aviso de interação com o símbolo do botão;
  - mira que abre com o recuo, marcador de acerto e números de dano;
  - traçantes e mensagens ("inventário cheio…").
- **Tela de inventário** (SELECT): mover, largar e dividir, com as reservas de munição e materiais.
- **Controles do GAME_DESIGN §4**, com os atalhos de depuração em L1 + R1 + … (BR-ADR-18).
- **Roteiro de combate** do autoteste, depois do de movimento, igual no PC e no PS2: abre o baú 4 e pega uma Rajada rara e células de escudo; dá 10 tiros no boneco 2 (330 de dano); recarrega; usa a célula (escudo 25); dá 3 golpes de picareta; larga a Rajada pelo inventário.

**Testes executados:**
- `make testes`: 66 testes, 19.234 verificações, 0 falhas. Os 13 novos cobrem:
  - os itens com os valores do Ventania e o sorteio (o Mítico nunca sai, e a mesma semente dá o mesmo resultado);
  - o inventário: picareta permanente, ciclo da barra, pilhas, cheio com coleta parcial, troca explícita, dividir, largar, munição e recarga;
  - vida e escudo;
  - mira pelo ombro, hitbox de cabeça e corpo, dano com raridade e cabeça, dispersão e projéteis no cone;
  - o loot: pontos, baú, pegar e trocar sem perder, munição automática, bloqueio de quem largou, inventário ao ser eliminado;
  - a picareta: um impacto por golpe no passo certo, combo, cadência, alcance;
  - o combatente: saque de 0,2 s, pente, recarga automática, semi-automática, a parede bloqueando o tiro, consumível com limite, baú e pegar;
  - o **roteiro de combate inteiro**;
  - o loot da ilha (37 baús, itens sobre o chão).
- **Poses do item na mão** conferidas em imagens laterais (Rajada, Gavião mirando, Faísca, golpe da picareta, curativo, célula).
- `make emulador`: `EMULADOR_OK`, com **5 comparações de imagem PS2 × PC**: treino 0,41%, Rajada na mão 0,51%, tela de inventário 0,36%, Vila 1,01%, Mirante 0,62%.
- `make emulador-completo`: `EMULADOR_OK`. O log de combate sai **igual linha a linha** no PC e no PS2.

**Medido** (bytes e primitivas valem para o console; tempos são do emulador):
- CPU do treino com o combate de 1,7 a 2,3 ms (1,2 ms na Etapa 4);
- 1.500 a 2.600 primitivas 2D no HUD;
- 16,4 MB livres no boot;
- pacote de 4,4 MB.

**Problemas encontrados e resolvidos:**
- três testes meus estavam errados: o tiro na cabeça matava o boneco, os itens do baú não pousavam no cenário de teste e a conta da recarga estava errada;
- o tiro registrava o acerto com o tick 0;
- a lista de munição passava da borda da tela de inventário;
- o número da vida encostava na barra de itens.

**Limitações conhecidas:**
- os bonecos não colidem com o personagem;
- sem dano nas construções e sem coleta de materiais (Etapa 9);
- sem som (Etapa 10);
- sem clarão do cano (o 3D não tem transparência aditiva ainda);
- o personagem ainda não recebe dano: não há inimigos até os bots (Etapa 7);
- os traçantes e números são 2D, desenhados por cima.

**⏳ Falta para o aceite:** jogar o combate no console (`docs/TESTE_NO_CONSOLE.md`, Etapa 6).

### Etapa 7 — Bots 🟡 (pronta e testada no PC e no emulador; ⏳ falta jogar no console)
**Feito** (detalhes em BRISA_ARCHITECTURE §20):
- **Dados dos bots** (`data/bots.json` e os 1.000 nomes do Ventania → `dados/bots.bdat`): as 3 dificuldades com os 21 parâmetros do Ventania, a mistura, os orçamentos e a audição, validados pelo compilador.
- **Grafo de navegação calculado no pipeline** (`tools/conversor/navegacao.py` → `mapa/ilha.bnav`, BR-ADR-19): a construção do `NavGrade` do Ventania sobre o layout da ilha. São 150×150 células de 4 m, paredes e portas, 111 andares e 30 ligações (7 escadas). O extrator da ilha agora grava os obstáculos e os 149 cômodos.
- **A\*** no jogo (`src/mundo/navegacao.*`) com o custo, a heurística ×1,3 e o limite de 7.000 expansões do Ventania, pontos precisos em portas e degraus, e a saída do meio de uma escada (BR-ADR-21).
- **Cérebro do bot** (`src/ia/cerebro.*`), o `bot_cerebro.gd` portado:
  - percepção: cone de visão com raio de visibilidade, memória da última posição, audição de tiros a 70 m, reação ao levar dano;
  - decisão por pontuação: explorar (pontos e cômodos), saquear, abrir baú, combater, cobertura, recuar, perseguir, investigar e curar;
  - execução: mira com o erro da dificuldade que assenta com o tempo de visada, rajadas, escolha da arma pela distância, recarga, consumível útil, troca de espaço;
  - andar com desvio local, manobra quando trava, arestas evitadas, replanejamento e abandono do objetivo.
- **Lutador** (`src/jogo/lutador.*`): o jogador e os bots são o mesmo tipo de personagem, com o mesmo movimento, colisão, combate e inventário. O jogador passa a **levar dano**, é eliminado e reaparece em 4 s.
- **Grupo de bots** (`src/jogo/grupo_bots.*`), o papel do servidor: 15 bots nos pontos de início, com nomes sem repetir, os trajes dos bots e a mistura de dificuldades; orçamento de 24 raios e 2 A* por passo; eliminações largam o inventário; feed de eliminações.
- **Na tela:** os bots desenhados com LOD por distância (18 m / 55 m, até 150 m) e sombra perto; nomes acima dos bots a até 30 m; "N VIVOS"; feed "A > B"; "ELIMINADO POR X · VOLTA EM N S".
- **Colisão em cápsula no topo** (BR-ADR-20): o personagem não bate mais a cabeça em bordas que pegam só os 20 cm de cima (o patamar da escada empilhada travava quem descia).
- **Roteiro do autoteste**, depois da caminhada na ilha: 15 s com 15 bots e o jogador parado (desenhando) e, depois, **a partida só de bots no próprio console**, sem desenhar, até sobrar um. O log traz o tempo preso, os estados e a CPU.
- No jogo normal, a ilha abre com 15 bots que reaparecem (`--sem-bots=1` desliga).

**Testes executados:**
- **Aceite (GAME_DESIGN §8)**, partida só de bots, 15 bots, sem Tormenta:
  - no PC, 12 sementes: todas com vencedor (de 2,0 a 10,8 min), tempo preso de 0,09% a 0,31% do tempo vivo, nenhuma falha de rota;
  - no emulador: vencedor em 4,8 min, preso 0,00%.
- `make testes`: 72 testes, 19.317 verificações, 0 falhas. Os 6 novos cobrem:
  - o grafo: andares, ligações e cômodos; chão × andar; A* pela escada nos dois sentidos; a saída do patamar; sem rota quando as saídas estão evitadas;
  - um bot com destino fixo subindo e descendo a escada empilhada do prédio e a do galpão, sem travar;
  - nomes sem repetir, trajes dos bots, mistura e valores das dificuldades;
  - dois bots Difíceis frente a frente: entram em combate e um elimina o outro;
  - o jogador leva dano de um bot, é eliminado (o feed e o "eliminado por" certos) e reaparece com vida cheia em 4 s;
  - a partida só de bots com 3 sementes (vencedor, 14 eliminações, preso < 1%, houve combate).
- `make emulador-completo`: `EMULADOR_OK` (fase dos bots e a partida no log). O combate e os marcos continuam **iguais linha a linha** no PC e no PS2.
- `make emulador`: `EMULADOR_OK`, com 6 comparações de imagem PS2 × PC; a nova (o jogador atrás de um bot, com o nome e "16 VIVOS") difere 1,16%, e as 5 anteriores seguem iguais (0,36% a 1,01%).

**Medido** (tempos do emulador, não do console):
- IA de 15 bots com o jogador, desenhando a ilha: 0,69 a 0,77 ms por passo em média; pior passo de pensar 1,34 ms e de simular 1,40 ms;
- partida só de bots: 0,33 ms por passo, pior 5,2 ms (dois A* longos no mesmo passo);
- 15,7 MB livres no boot (16,4 MB na Etapa 6); pacote de 4,5 MB (234 arquivos);
- no PC: 8 µs por passo para os 15 bots; 250 a 330 A* por partida, de 15 a 30 expansões em média cada.

**Problemas encontrados e resolvidos:**
- o cabeçalho do BNAV tinha 68 bytes, não 64 (passou a 96);
- os testes do treino quebravam porque a biblioteca de modelos lotava com os LODs dos trajes (capacidade aumentada; o teste agora para se a carga falhar);
- tempo preso de 2% na primeira simulação: a cabeça batia no patamar da escada do prédio (BR-ADR-20);
- bots presos no patamar do meio da escada do galpão (BR-ADR-21);
- um bot ficou numa sala porque as arestas evitadas fechavam a única porta (agora esvazia a lista e replaneja);
- as contas de eliminações não zeravam ao repovoar.

**Limitações conhecidas:**
- sem ônibus, queda e Tormenta (Etapa 8), por isso as partidas de bots levam de 2 a 11 min;
- os bots não coletam materiais nem constroem (Etapa 9);
- a partida dos bots não sai igual no PC e no PS2 (a FPU do EE não é IEEE); os testes conferem propriedades;
- os personagens não colidem entre si (como os bonecos);
- sem som de tiro (Etapa 10); os bots "ouvem" pela regra do servidor;
- o pior passo da IA (5 ms no emulador) precisa ser medido no console.

**⏳ Falta para o aceite:** jogar contra os bots no console e medir a CPU da IA (`docs/TESTE_NO_CONSOLE.md`, Etapa 7).

### Etapa 8 — Battle Royale 🟡 (pronta e testada no PC e no emulador; ⏳ falta jogar no console)
**Feito** (detalhes em BRISA_ARCHITECTURE §21):
- **Dados da partida** (`data/partida.json` → `dados/partida.bdat`, BPAR): os tempos e as 7 fases da Tormenta do Ventania, validados pelo compilador.
- **Tormenta** (`tormenta.gd` portado): espera → fechamento → próxima, zonas aninhadas com centro em terra e longe do mar, dano por segundo só na vida (o escudo não protege).
- **Ônibus dos Ventos** (`onibus.gd`): rota sorteada a 120 m, entrando e saindo sobre o mar; portas sobre terra; ✕ salta; quem fica é ejetado. Entram também o ônibus 3D e a câmera atrás dele.
- **Queda livre, mergulho e planador** (`movement.gd _queda`), com as animações `queda_livre`, `mergulho`, `planador_abrir` e `planador`; o corpo de bruços, de cabeça para baixo, inclinando nas curvas; e a asa delta sobre a cabeça.
- **Partida** (`partida_br.gd`): contagem de 10 s → ônibus (loot novo, só a picareta) → jogo (3 s de proteção) → fim. Sem reaparecer; colocações, eliminações, vencedor; nenhum tiro acerta no ônibus, no ar ou na proteção.
- **Espectador** (segue quem eliminou, ✕ troca) e **tela de resultado** (`#N DE 16` ou `VITÓRIA!`, eliminações, tempo vivo, dano, baús e curas; ✕ jogar de novo, ▲ assistir).
- **Bots:** saltam perto de uma região ou de um baú, guiam a queda até lá, fogem da Tormenta (urgência do Ventania) e exploram e recuam dentro da zona segura.
- **Tela:**
  - a parede da Tormenta roxa e translúcida, com faixas animadas (BR-ADR-23), no PS2 e no PC;
  - a tela roxa e o aviso dentro da Tormenta;
  - zona atual e próxima no minimapa, a rota tracejada do ônibus;
  - contagem, fase e tempo da Tormenta;
  - "SALTAR" com o símbolo ✕ e os avisos da queda.
- **16 participantes** (BR-ADR-22), medidos. O ELF abre numa partida (BR-ADR-24); `--treino-livre=1` volta ao treino da Etapa 7.
- **Custo de rota no EE** (BR-ADR-25): componentes conexas do grafo e espera depois de uma rota que falhou. Eram passos de 46 ms no emulador.
- **Roteiro do autoteste** (`src/jogo/roteiro_partida.h`, o mesmo dos testes no PC): joga uma partida do começo ao fim. Depois, a partida BR só de bots roda sem desenhar.

**Testes executados:**
- **Aceite**, partida BR só de bots (15) com ônibus e Tormenta:
  - no PC, 12 sementes: todas terminam com vencedor (de 2,3 a 6,4 min), preso de 0,00% a 0,15%;
  - no emulador: vencedor em 3,7 min, preso 0,00%, os 15 saltaram.
- `make testes`: 82 testes, 19.573 verificações, 0 falhas. Os 10 novos cobrem:
  - os dados da partida com os valores do Ventania;
  - a Tormenta: fases, zonas aninhadas, determinismo, fechamento interpolado e dano só na vida (porte do `test_tormenta.gd`);
  - a rota do ônibus em 20 sementes;
  - queda livre, mergulho sem tranco, planador abrindo perto do chão e pouso (porte do `test_queda.gd`);
  - as poses e a inclinação no ar;
  - **a partida inteira jogada pelo roteiro**: proteção, colocação 16 de 16, resultado e "jogar de novo";
  - o dano da Tormenta fora da zona;
  - a partida BR só de bots em 3 sementes (vencedor, colocações de 1 a 15, preso < 1%);
  - as componentes do grafo.
- `make emulador`: `EMULADOR_OK`, com 8 comparações de imagem PS2 × PC. As 3 novas: no ônibus 1,36%, de frente para a parede da Tormenta 0,75%, cartão do resultado 0,44%.
- `make emulador-completo`: `EMULADOR_OK`. Os marcos da partida do jogador (posições, alturas da queda, colocação) saem iguais no PC e no PS2.

**Medido** (tempos do emulador, não do console):
- partida com o jogador: CPU de 2,4 a 3,9 ms por quadro e 3D de 0,8 a 3,7 ms, a 59,8 FPS no emulador; IA de 0,46 a 0,72 ms por passo em média, pior passo de pensar 1,53 ms e de simular 1,0 ms;
- partida só de bots: 0,39 ms por passo em média, pior 4,9 ms (eram 46 ms antes do BR-ADR-25), nenhum passo acima de 10 ms;
- parede da Tormenta: até 384 triângulos com alfa;
- 15,6 MB livres no boot (15,7 MB na Etapa 7); pacote com 235 arquivos (4,5 MB).

**Problemas encontrados e resolvidos:**
- passos de 46 ms no emulador: bots pousados em telhados sem escada ou na água repetiam A* que esgotavam as 7.000 expansões (BR-ADR-25). Um teste com a partida repetida mostrou que não era o JIT do Play!;
- a parede da Tormenta quase não aparecia de longe (o perfil de alfa estava errado) e virava um tom uniforme no céu. Ficou com o perfil do Ventania e as faixas animadas;
- os nomes dos bots dentro do ônibus apareciam no ar;
- textos do HUD encostados (título no minimapa, fase sobre o feed, botões fora do cartão);
- a fonte não tinha o "#".

**Limitações conhecidas:**
- as partidas de bots terminam antes de a Tormenta fechar (de 2 a 7 min): os bots pousam juntos nas regiões e lutam cedo, como no Ventania. Ainda não há dados de quanto dura uma partida com uma pessoa;
- a parede da Tormenta não tem neblina; um planador só (os cosméticos vêm na Etapa 12);
- sem construção, coleta e materiais no resultado (Etapa 9); sem XP e passe (Etapa 11); sem lobby e sem "voltar ao lobby" (Etapa 10);
- a partida dos bots diverge entre PC e PS2 (FPU do EE), como na Etapa 7; o roteiro do jogador continua igual.

**⏳ Falta para o aceite:** jogar uma partida inteira no console e medir a CPU (`docs/TESTE_NO_CONSOLE.md`, Etapa 8).

### Etapa 9 — Construção ⬜
Parede, piso, rampa, teto (se possível), materiais, coleta, destruição e desabamento. Limite de peças medido. Edição de ladrilhos como opcional.

### Etapa 10 — Interface e lobby ⬜
Telas iniciais, menu, lobby 3D, Armário, configurações, HUD final, fonte, áudio ADPCM.

### Etapa 11 — Progressão ⬜
XP, níveis, Faíscas com ledger, passe gratuito, recompensas, missões, validador de temporada.

### Etapa 12 — Loja e cosméticos ⬜
Catálogo, loja rotativa (RTC), compra local, os 11 trajes, picaretas, planadores, primeiras mochilas e emotes originais, e o save completo no Memory Card (A/B, CRC, recuperação).

### Etapa 13 — Polimento e distribuição ⬜
Testes, otimização, correções, ELF final, pacotes de conteúdo, sistema de patches, documentação para jogar.

## Riscos conhecidos

| Risco | Impacto | Mitigação |
|---|---|---|
| Microcódigo do VU1 é difícil e pouco documentado | Atraso na Etapa 2 | Começar pelo caminho A (EE + gsKit); a comparação medida decide |
| Densidade da Vila Brisa (25 mil triângulos num setor de 64 m) | FPS cai dentro da cidade | LOD, culling de interiores, neblina, ilha compacta (Etapa 5) |
| Sem console nem BIOS no ambiente de desenvolvimento | Desempenho real desconhecido | O dono testa cada etapa no console; ps2link para medir |
| Emulador ≠ console | Falsos positivos | Aceites de desempenho e compatibilidade só no console |
| 32 MB de RAM e 4 MB de VRAM | Cortes tardios | Orçamento desde a Etapa 0, mapa de memória impresso no boot |
| Contêiner efêmero | Perda do trabalho | Repositório remoto antes da Etapa 1 |
| Escopo grande | Sistemas pela metade | Uma etapa por vez, aceite antes de avançar |
