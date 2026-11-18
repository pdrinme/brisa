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

### Etapa 1 — Primeira execução no PS2 🟡 (tudo pronto e testado em emulador; ⏳ falta o teste no console)
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

### Etapa 2 — Motor gráfico ⬜
- Câmera 3D, matrizes, frustum, z-buffer.
- Caminho A (EE + gsKit) e caminho B (VU1 próprio); benchmark comparando os dois.
- Malha estática com cor de vértice, luz em 2 faixas, neblina e blob.
- **Aceite:** cena de benchmark com N mil triângulos a 30 FPS medidos no console; decisão do BR-ADR-1 registrada com os números.

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
