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

### Etapa 6 — Combate e inventário ⬜
As 5 armas, munição, dano, raridade, cura, escudo, inventário de 5 espaços, picareta, loot e baús.

### Etapa 7 — Bots ⬜
Navegação (grafo do pipeline), exploração, saque, coleta, combate, cobertura, Tormenta, dificuldades. Custo por bot medido no console. **Aceite:** partida só com bots termina e o tempo preso fica < 1%.

### Etapa 8 — Battle Royale ⬜
Fluxo da partida, Tormenta de 7 fases, zona segura, eliminações, espectador, vitória e resultados. Ônibus e planador se o desempenho permitir. Número de participantes definido por medição (12 → 16).

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
