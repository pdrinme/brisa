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

### Etapa 3 — Pipeline Ventania → Brisa ⬜
- Conversores de modelo (BMD/BSK/BAN), textura (BTX), dados (BIN) e empacotador (PAK).
- LODs (comparação decimação × geradores).
- Catálogo do Brisa com a licença e a origem de cada asset.
- Importar o kit de construção, o baú, as armas e o Recruta.
- **Aceite:** a vitrine do kit (a mesma composição da imagem de referência) renderizada no PS2 e lado a lado com a do Ventania; memória medida; conversores com testes.

### Etapa 4 — Personagem ⬜
O Recruta convertido, com movimento, câmera, animações, colisão (relevo + caixas) e a decisão sobre o dano de queda.

### Etapa 5 — Mapa ⬜
Relevo, vegetação, construções do mapa, baús, POIs, estradas, setores, LOD e culling, minimapa. **Decisão do tamanho da ilha** com medições (BRISA_GAME_DESIGN §6).

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
