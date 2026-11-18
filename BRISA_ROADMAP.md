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

### Etapa 0 — Estudo do Ventania e planejamento ✅ (aguardando autorização para a Etapa 1)
- ✅ Ventania clonado (somente leitura, commit `7864cd7`, v1.2.1). Lidos os 11 documentos obrigatórios, os dados (`game/data`, `backend/config`), os geradores de assets, os 90 GLB, o código dos sistemas e as imagens de `docs/art/`.
- ✅ Estudo sistema a sistema, com os 6 pontos de cada um e o estado real do código: `docs/estudo/SISTEMAS_VENTANIA.md`. Inclui 9 divergências entre documentos e código no Ventania.
- ✅ Auditoria automática dos assets (`tools/auditoria/auditar_ventania.py`): 90 GLB, 52.420 triângulos visuais, todos "Original"; os 11 personagens com skinning rígido de 20 ossos e 16 animações compartilhadas.
- ✅ Lista de reaproveitamento arquivo a arquivo: 71 diretos, 11 com LODs, 7 com simplificação e 1 descartado (`docs/auditoria/REUSO_GLB.md`).
- ✅ Extração do layout real da Ilha Ventania (Godot headless numa cópia): 646 peças, 1.219 props, 37 baús, 102 loot. A extração é reproduzível byte a byte, e o Ventania continuou intacto (`git status` limpo).
- ✅ Densidade do mapa por setor (dimensiona LOD, culling e distância de visão).
- ✅ Toolchain PS2DEV validado (GCC 15.2, PS2SDK, gsKit, IRX): `verificar_toolchain.sh` → `TOOLCHAIN_OK`; ELFs de teste compilados.
- ✅ Documentos: README, BRISA_ARCHITECTURE, BRISA_GAME_DESIGN, BRISA_ROADMAP, ASSET_LICENSES, PIPELINE_ASSETS, FERRAMENTAS_PS2DEV, ESTILO_VISUAL_PS2, SINCRONIA_VENTANIA.
- ⬜ Pendente com o dono: (1) autorização formal de redistribuir os assets do Ventania no Brisa (ASSET_LICENSES §2); (2) as imagens de referência citadas no pedido; (3) um repositório remoto para o Brisa (o contêiner é efêmero).
- Não executado: nenhum ELF rodou (não há emulador com BIOS nem console aqui).

### Etapa 1 — Primeira execução no PS2 ⬜
- `Makefile` próprio, estrutura `src/`, núcleo e plataforma separados, alvo `make` (ELF) e `make testes` (PC).
- Inicializar SIF/IOP, carregar os IRX embutidos (sio2man, padman), modo de vídeo NTSC 640×448 (PAL opcional) com gsKit e buffer duplo.
- Desenhar uma tela (fundo azul-petróleo → índigo, o logotipo BRISA em sprites, um quadrado girando) e ler o DualShock 2 (botões e analógicos na tela).
- Medir: tempo de quadro e memória livre no boot.
- **Aceite:** o ELF inicia no PCSX2 **e no console** (USB ou ps2link), mostra a tela, reage ao controle e roda 10 min sem travar. Os testes do PC passam.

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
