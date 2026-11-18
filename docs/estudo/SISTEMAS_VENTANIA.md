# Estudo do Ventania, sistema por sistema

Fonte: repositório `pdrinme/ventania`, commit **`7864cd7` (v1.2.1)**, lido em
2026-11-18, **somente leitura**. Cada afirmação abaixo foi conferida no código,
nos arquivos de dados ou nas ferramentas, e não apenas nos documentos. Onde o
documento e o código divergem, vale o código, e a divergência fica registrada
(seção final).

Legenda do estado no Ventania:
- **IMPL**: implementado e testado no Ventania.
- **PARCIAL**: existe, com lacunas.
- **DOC**: só está documentado ou planejado; não existe no código.

Para cada sistema: **(1)** como funciona no Ventania · **(2)** regras ·
**(3)** elementos visuais · **(4)** adaptação ao PS2 · **(5)** o que simplificar
· **(6)** o que refazer do zero.

O Ventania é um projeto Godot 4.7.2 (GDScript, cerca de 19,6 mil linhas fora
dos testes), com servidor de partida autoritativo e meta-servidor Python/SQLite.
**Nenhum código do Ventania roda no PS2.** Todo o código do Brisa é novo (C/C++);
o Ventania entra como especificação, dados e assets.

---

## 1. Personagens — IMPL

1. 11 trajes originais gerados por script Blender (`tools/assets/kit/blender/gerar_personagem.py` e `gerar_skins.py`): Recruta, Recruta Brisa, Capitão Maré, Entregador, Escafandro, Faz-Tudo, Espantalho, Explorador das Nuvens, Botânico, Astrônomo e Guardião da Tempestade.
2. Um **esqueleto único de 20 ossos** (Root, Hips, Spine, Chest, Neck, Head, e Shoulder/UpperArm/LowerArm/Hand/UpperLeg/LowerLeg/Foot nos dois lados). Mesma hitbox para todos os trajes. Cosmético nunca muda gameplay.
3. Altura de cerca de 1,88 m, cabeça com cerca de 1/4,5 da altura, mãos e pés grandes. **Skinning rígido**: cada peça pertence 100% a um osso, e as juntas são cobertas por esferas. Confirmado pela auditoria: as 11 malhas têm no máximo 1 influência por vértice. De 2.128 a 4.072 triângulos e 1 material (o Capitão Maré tem 3: paleta, vidro e água).
4. **Excelente encaixe com o PS2.** O skinning rígido dispensa mistura de pesos: cada peça é transformada por uma única matriz no VU1. As cores da paleta viram cor de vértice, sem textura.
5. LODs obrigatórios (perto ≈ original, médio ≈ 1.000, longe ≈ 300 triângulos). O aquário do Capitão Maré vira cúpula translúcida simples fora do close-up, e os peixes só aparecem no lobby e a menos de 10 m.
6. Formato binário de malha esquelética do Brisa, matriz por osso no VU1 e geração de LOD (decimação ou reexecução dos geradores com menos segmentos; decisão na Etapa 3).

## 2. Movimentação em terceira pessoa — IMPL

1. `gameplay/character/movement.gd`: uma simulação compartilhada por servidor e cliente (predição), a 30 Hz.
2. Andar 4,5 m/s, correr 6,5 m/s (só para a frente), agachado 2,5 m/s, gravidade 22 m/s², salto de cerca de 1,4 m (v = 7,4 m/s), aceleração de 45 m/s² no chão e 9 m/s² no ar, queda máxima de 45 m/s, "grudar no chão" a 0,45 m. As rampas sobem na velocidade normal (`floor_constant_speed`, medido de 96% a 99%). Escadas em U com degraus de 20 cm.
3. Blend de locomoção (idle, andar, correr) e agachado.
4. Os mesmos números, num laço fixo de 30 Hz em C++, sem rede. A colisão própria troca o `CharacterBody3D`: cilindro vertical contra relevo, peças da grade e props.
5. Sem predição e sem reconciliação (não há rede). Degraus tratados como rampa invisível de colisão (a escada visual continua com degraus).
6. Colisão e resposta do personagem (deslizar em paredes, subir degrau, snap ao chão).

## 3. Câmera — IMPL

1. `player_camera.gd`: câmera por cima do ombro, com troca de ombro e mira com zoom.
2. Altura dos olhos 1,55 m (agachado 1,05), ombro 0,55 m, distância 3,2 m (mirando 1,7), FOV 70° (mirando 55°), pitch de −80° a +70°, sensibilidade ×0,5 ao mirar. Distância extra no ônibus e na queda.
3. Mira no centro da tela.
4. Igual, com o analógico direito do DualShock 2. Colisão da câmera com as peças por um raio contra a grade de 4 m (barato).
5. Aceleração e curva de resposta do analógico próprias para controle (o Ventania foi calibrado para mouse primeiro).
6. Tudo (matemática simples).

## 4. Animações — IMPL

1. 16 ações por personagem, geradas proceduralmente no Blender: `idle-loop`, `andar-loop`, `correr-loop`, `agachado-loop`, `agachado_andar-loop`, `pular`, `queda-loop`, `aterrissar`, `preparar-loop`, `saltar`, `queda_livre-loop`, `mergulho-loop`, `planador_abrir`, `planador-loop`, `aterrissar_pesado` e `acenar`. Medido: 60 canais e cerca de 7,6 mil chaves no total por personagem.
2. Na partida, `AnimationTree` (BlendSpace1D), `AimPoseModifier` (coluna segue a mira), `ItemRig` (pose do item mais ações procedurais) e `TwoBoneIK3D` nos braços. "O item comanda as mãos." Marcadores nos modelos: `grip_r`, `grip_l`, `muzzle`, `mag`, `mao`, `alca_tampa`.
3. Animações curtas e expressivas, com quadril balançando.
4. As animações são **compartilhadas por todos os trajes** (um conjunto só na RAM). Amostragem a 30 Hz, quaternions em int16: cerca de 60 KB.
5. Mira por rotação aditiva em Spine/Chest/Head (sem modificador genérico). IK analítico de 2 ossos só no jogador local e nos personagens a menos de 15 m; os demais usam pose de braço pronta por categoria de item.
6. Player de animação (amostragem, mistura de 2 a 3 camadas) e poses de empunhadura por categoria (fuzil, pistola, escopeta, picareta, consumível, construção), geradas no pipeline.

## 5. Armas — IMPL

1. `game/data/itens.json`. Tiro hitscan validado no servidor; traçantes visíveis; dispersão com modificadores.
2. Cinco armas originais:

| Arma | Tipo | Munição | Dano | Cadência (/s) | Pente | Recarga (s) | Alcance (m) | Dispersão (°) | Projéteis | Cabeça |
|---|---|---|---|---|---|---|---|---|---|---|
| Rajada | Rifle de Assalto | média | 30 | 5,5 (auto) | 30 | 2,3 | 150 | 1,6 | 1 | ×1,5 |
| Trovão | Espingarda | cartucho | 9 | 1,0 | 5 | 4,2 | 35 | 6,0 | 10 | ×1,5 |
| Zumbido | Submetralhadora | leve | 17 | 11,0 (auto) | 30 | 2,1 | 70 | 2,8 | 1 | ×1,5 |
| Faísca | Pistola | leve | 24 | 5,0 | 16 | 1,4 | 80 | 1,2 | 1 | ×1,75 |
| Gavião | Rifle de Precisão | pesada | 100 | 0,35 | 1 | 2,6 | 300 | 4,0 (0 mirando) | 1 | ×2,5 |

   Modificadores de dispersão: mirando ×0,5, movendo ×1,6, agachado ×0,75, no ar ×2,2. Recarga por pente, cancelada ao trocar de arma. **Explosivos: DOC** (não existem).
3. Modelos de 128 a 300 triângulos (`rajada_01`, `trovao_01`, `zumbido_01`, `faisca_01`, `gaviao_01`), em paleta. Nome exibido = tipo + nome próprio.
4. Os mesmos dados no formato do Brisa. Hitscan pelo DDA da grade de 4 m mais cilindros de hitbox.
5. Traçantes como linhas ou quads aditivos curtos. Efeitos de impacto por material num pool fixo de partículas.
6. Lógica de tiro, dispersão e recarga em C++.

## 6. Munição — IMPL

1. Reserva fora dos 5 espaços.
2. Leve (máx. 360, caixa 30), média (360/30), pesada (60/6) e cartucho (80/10).
3. Caixa de munição `municao_01` (100 triângulos).
4. Igual.
5. Nada.
6. Lógica (simples).

## 7. Raridade — IMPL

1. `itens.json` → `raridades`. Multiplica dano e tempo de recarga e define pesos de sorteio.
2. Comum ×1,00/×1,00 (baú 34, chão 45), Incomum ×1,05/×0,95 (30/30), Raro ×1,10/×0,90 (21/16), Épico ×1,16/×0,85 (11/7), Lendário ×1,22/×0,80 (4/2) e Mítico ×1,30/×0,75 (0/0, só eventos; não é usado por nenhum item hoje).
3. Cores `#b5b5b5`, `#5fd35a`, `#3fa2ff`, `#b45cf2`, `#ffb52e` e o degradê animado `#ff4fd8`→`#2ee6ff`. Brilho do item no chão, cartões de loja e inventário na cor da raridade.
4. Igual. O degradê animado do Mítico vira ciclo de cor por vértice.
5. Nada.
6. Lógica.

## 8. Inventário — IMPL

1. `gameplay/items/inventory.gd` (autoritativo no servidor) e `ui/hud/inventory_screen.gd`.
2. Picareta fixa e fora dos espaços, mais 5 espaços (armas e consumíveis), munição e materiais em reserva. Operações: trocar, dividir pilha, largar com quantidade e pegar escolhendo o espaço. **Nunca troca em silêncio**: com o inventário cheio, Interagir troca pelo item da mão ("Trocar X por Y"). O item largado cai 1,6 m à frente, e quem largou não o recoleta por 2 s. Testes de conservação (sem perda e sem duplicação).
3. Barra com 6 espaços (picareta + 5), ícones renderizados dos modelos e tela de inventário.
4. Igual, com controle: L1/R1 troca de espaço, → seleciona a picareta, SELECT abre a tela.
5. Na tela de inventário, "dividir" e "largar quantidade" usam um seletor numérico com o direcional, no lugar do arrastar.
6. Lógica e telas.

## 9. Picareta permanente — IMPL

1. `ferramentas.picareta` em `itens.json`, com o golpe decidido no servidor.
2. Cadência 1,6/s, golpe de cerca de 0,6 s (19 passos) com impacto no passo 8, alcance de 2,4 m, 20 de dano em jogador e 50 em construção. Um impacto por golpe, alternando o lado. Não cai, não é largada, não é trocada.
3. `picareta_01` (532 triângulos) e a variante cosmética `ancora_01` (772), com os mesmos marcadores.
4. Igual.
5. Nada.
6. Lógica.

## 10. Coleta de materiais — IMPL

1. Golpe da picareta em props e peças do mapa (`construcao.json` → `coleta`).
2. Árvore/arbusto = 7 de madeira, rocha = 6 de pedra, peça do mapa = 4 do material da peça por golpe, e +10 ao derrubá-la. Construções de jogadores não dão material. Máximo de 999 de cada.
3. Sons e efeitos de impacto por material (madeira, pedra, metal, terra).
4. Igual. Cada prop coletável guarda o material no formato do mapa.
5. As árvores não são destruídas (como no Ventania).
6. Lógica.

## 11. Construção — IMPL

1. `gameplay/build/` (`grade.gd`, `construcoes.gd`, `build_server.gd`) e `scenes/game/build_mode.gd`.
2. Grade de 4 m alinhada ao mundo. Peças: parede (2 planos), piso, rampa (4 direções) e teto em pirâmide. Custo de 10 do material. Madeira 150 de vida em 1 s, pedra 300 em 1,6 s, metal 500 em 2,4 s; nasce com 15% da vida. Alcance de 9 m, até 15 peças/s em sequência, limite de 1.200 peças de jogadores (medido). **Edição** por ladrilhos (parede 3×3, piso 2×2) só pelo dono. Escolha do slot pelo olhar.
3. Prévia azul ou vermelha com o motivo. Paredes de 108 a 192 triângulos, teto de 30. Tom "em obra" e rachaduras por shader (Voronoi).
4. **Encaixa no PS2**: a grade de 4 m também serve de grade espacial para colisão, tiro e navegação. As peças são instâncias de 12 malhas.
5. Limite de peças de jogadores a medir (começa em 300). O tom de obra vira cor de vértice. As rachaduras viram 2 ou 3 estágios de malha/decal, sem shader. **Edição de ladrilhos fica para depois do básico** (Etapa 9, se o desempenho permitir); paredes com porta e janela do mapa vêm prontas do pipeline.
6. Tudo em C++ (grade, apoio, desabamento, prévia).

## 12. Destruição de estruturas — IMPL

1. Dano de tiros (`dano_armas` = 1,0) e da picareta (50). Em zero, a peça some. Peças sem caminho até o chão desabam (BFS no servidor, em lotes de 40 por tick).
2. As **646 peças das casas do mapa** (contadas pelo extrator, ids 1..N determinísticos) são do mesmo sistema: destrutíveis e coletáveis, restauradas a cada partida.
3. Fragmentos e som por material, rachaduras progressivas.
4. Igual, com BFS em lotes e orçamento por quadro.
5. Fragmentos num pool pequeno (cerca de 32 ativos).
6. Lógica.

## 13. Baús — IMPL

1. `bau_ventania_01` (476 triângulos, tampa articulada, animação `abrir`). Abrir segurando Interagir.
2. Cada baú solta 1 arma, 1 consumível e 2 caixas de munição (`itens.json` → `bau`), pela tabela de raridade. Não reabastecem no BR.
3. **Azul-petróleo com amarelo**, a cor de função do loot (guia de estilo, princípio 4).
4. Igual, com o mesmo modelo convertido.
5. Brilho do baú (partículas) como sprite aditivo pulsando.
6. Lógica.

## 14. Loot — IMPL

1. `gameplay/items/loot_server.gd`. Pontos de loot no chão sorteados de novo a cada partida.
2. 37 baús e 102 pontos de loot na Ilha (conferido pelo extrator), com cotas para dentro das casas, área externa dos POIs e natureza. Pesos por raridade (chão e baú).
3. Item no chão com brilho da raridade e placa de material.
4. Igual.
5. Itens no chão desenhados só a até 40 m.
6. Lógica.

## 15. Tormenta (zona) — IMPL

1. `gameplay/match/tormenta.gd`, dados em `game/data/partida.json`.
2. Raio inicial de 380 m e 7 fases. Cada fase espera, depois fecha:

| Fase | Espera (s) | Fechamento (s) | Raio (m) | Dano/s |
|---|---|---|---|---|
| 1 | 60 | 45 | 190 | 1 |
| 2 | 45 | 40 | 120 | 1 |
| 3 | 40 | 35 | 75 | 2 |
| 4 | 35 | 30 | 45 | 5 |
| 5 | 30 | 25 | 25 | 8 |
| 6 | 25 | 20 | 10 | 10 |
| 7 | 20 | 20 | 0 | 15 |

   O dano é 1×/s, só na vida (ignora o escudo). A zona seguinte cabe inteira na atual e fica em terra. Se todos morreriam juntos, o de mais vida vence. Soma de cerca de 470 s, ou 9–10 min com espera e ônibus.
3. Parede roxa animada (`azul_tormenta` #4A3FB0), tela roxa dentro dela, avisos e som em cada mudança.
4. Igual. A parede é um cilindro de 32–48 segmentos com cor de vértice translúcida e rolagem de UV numa textura pequena de faixas. A tela roxa é um quad em tela cheia com alfa do GS.
5. Os raios podem ser escalados se o mapa do Brisa for menor (fator único em `partida`).
6. Lógica e visual.

## 16. Pontos de interesse — IMPL

1. `IlhaVentania.POIS`, a fonte única (id, nome, posição, raio, cor), com `region_bounds()` (contorno derivado das construções) e `region_at()`.
2. Vila Brisa (−150, 55; r 48; menta `5fd6a4`), Pedreira Cinza (40, −170; r 40; pedra `b4b7c2`), Porto Ferrugem (185, 45; r 44; ferrugem `e0703f`), Fazenda Girassol (−40, 170; r 44; amarelo `f5c542`), Mirante do Vento (10, 5; r 26; céu `7cc8ff`) e Acampamento Coral (−175, −150; r 34; coral `ff7f7a`), mais cabanas espalhadas.
3. Cada POI tem cor e silhueta próprias (madeira, pedra, metal, celeiro, torre, cabanas). Estradas de asfalto, ruas com calçada e caminhos de terra.
4. Os mesmos POIs, pelos dados extraídos (ver `docs/auditoria/`).
5. Interiores mobiliados desenhados só quando a câmera está dentro ou a menos de 25 m. Mobília: 61 mil triângulos no mapa inteiro.
6. Formato de mapa por setores e o desenho de estradas como malha pré-calculada pelo pipeline.

## 17. Minimapa — IMPL

1. Gerado do relevo, com o nome das regiões (prioridade para a atual), a seta do jogador, a zona e a rota do ônibus. Chip da região atual abaixo do minimapa.
3. Canto superior direito, com contorno e cor da região.
4. **Textura pré-renderizada pelo pipeline** (128×128 ou 256×256 em CLUT 8 bits: o formato nativo do GS), com a zona e os ícones desenhados por cima em sprites.
5. Os nomes no minimapa só aparecem para a região atual e as vizinhas (a TV tem menos resolução).
6. Gerador offline da imagem a partir do layout extraído.

## 18. Mapa completo — IMPL

1. Tecla M / ▲. Estradas e construções vistas de cima, regiões com etiqueta colorida, zona atual e próxima, rota do ônibus.
4. A mesma textura em 256×256 (CLUT 8), com zoom e movimento pelo analógico.
5. Sem zoom contínuo: 2 níveis.
6. Tela 2D.

## 19. Eliminações — IMPL

1. `partida_br.gd`: eliminação definitiva em Solo, com colocação = quantos estavam vivos. Desconectar conta como eliminação (não se aplica ao Brisa).
2. **Estado "caído" com revive: DOC** (só faria sentido em Dupla ou Esquadrão, que não existem na partida).
3. Feed de eliminações e aviso na tela.
4. Igual, sem a parte de rede. Ao morrer, o inventário cai no chão (`drop_all`).
5. Nada.
6. Lógica.

## 20. Espectador — IMPL

1. Ao ser eliminado, segue quem o eliminou. Atirar troca de jogador, e é possível voltar ao lobby a qualquer momento.
4. Igual: a câmera segue qualquer entidade.
5. Nada.
6. Câmera e HUD.

## 21. Tela de resultados — IMPL

1. `ui/hud/tela_resultado.gd`: VITÓRIA! com o personagem acenando e confete, ou a colocação e o vencedor. Estatísticas: eliminações, tempo vivo, dano, baús, curas, materiais e construções. XP detalhado e "PASSE NÍVEL N". Botões Jogar de novo, Voltar ao lobby e Assistir.
4. Igual, com o personagem em 3D no lobby simplificado.
5. Confete como sprites.
6. Tela.

## 22. Bots — IMPL

1. `gameplay/ai/bot_cerebro.gd` (978 linhas) e `nav_grade.gd` (507). O bot é um jogador como os outros: gera `InputCmd` e passa pelas mesmas regras, sem tocar no mundo diretamente.
2. Utility scoring com inércia. Estados: explorar, saquear, abrir baú, coletar, ir para a zona, combater, cobertura, recuar, perseguir, investigar e curar. Percepção por cone e raio, com orçamento de 24 raios por tick; memória da última posição **vista**; tiro ouvido a até 70 m vira ponto de investigação; dano revela só a direção. Navegação em grade de 150×150 células de 4 m mais nós de andar, escadas e rampas, com A* limitado a 6.000 expansões e 2 caminhos por tick. Recuperação de travamento em 3 níveis. Dificuldades (`data/bots.json`):

| | Fácil | Médio | Difícil |
|---|---|---|---|
| Decisões/s | 2,5 | 5 | 7 |
| Reação | 0,9 s | 0,55 s | 0,3 s |
| Erro de mira | 6° | 3,2° | 1,6° |
| Visão | 55 m / 100° | 75 m / 120° | 95 m / 140° |
| Memória | 3 s | 6 s | 10 s |
| Investiga | 30% | 70% | 100% |
| Cobertura | 0% | 50% | 90% |
| Constrói ao levar tiro | não | não | sim |
| Coleta | não | sim | sim |

   Partida completada até 16 (mistura Fácil:Médio:Médio:Difícil). Nomes sorteados de uma lista de 1.000 (`data/nomes_jogadores.txt`).
3. Usam os trajes Recruta e Recruta Brisa (`trajes_dos_bots`).
4. **O mesmo desenho de IA em C++.** O grafo de navegação estático sai pronto do pipeline (offline); só as arestas tocadas por construções são atualizadas em jogo.
5. A* com menos expansões (para começar, 2.000) e 1 caminho por tick. Decisões a 1 Hz para bots longe da câmera. Menos raios de percepção por tick (a medir). Bots podem usar mais trajes (só aparência).
6. Tudo em C++. Cerca de 0,04 ms por bot por tick num PC moderno com GDScript; no EE, em C++, precisa ser medido (Etapa 7).

## 23. Lobby — IMPL

1. `scenes/menu/main_menu.gd` (1.735 linhas): personagem equipado com idle, nome, nível, barra de XP, Faíscas, JOGAR com seletor de modo, abas Loja, Passe, Missões, Armário (Perfil → Coleção), Amigos e Configurações.
3. Interface "adesivo ao vento": fundo noturno azul-petróleo → índigo com rajadas de vento animadas, contorno grosso escuro e sombra dura deslocada, botões levemente inclinados com o texto em caixa alta, **amarelo só para a ação principal**, cartões escuros translúcidos e foco sempre visível (contorno amarelo). Fonte Fredoka (OFL).
4. Cena 3D simples (personagem, chão, céu) com a interface 2D em sprites. Navegação só por controle.
5. Sem a aba Amigos. As rajadas de vento viram 6–10 sprites animados.
6. Toda a interface (sistema próprio de widgets no GS).

## 24. Grupos — IMPL no Ventania (só referência)

1. Grupos de até 4 no meta-servidor, líder, convites, presença e lobby lado a lado.
2. **Os modos Dupla e Esquadrão são DOC**: o meta-servidor só aceita os modos `br` e `treino` (`social.py`: `MODOS = ("br", "treino")`), e a partida não tem times.
4. **Fora do escopo do Brisa** (não há online). Fica a referência visual: até 4 personagens lado a lado no lobby, que pode servir para um futuro modo em tela dividida.
6. Nada agora.

## 25. Cosméticos — PARCIAL

1. `game/data/cosmeticos.json` (id → cena), catálogos em `backend/config/catalog/`. Categorias previstas: traje, picareta, mochila, emote, acessório, trilha e planador (`PlayerProfile.COSMETIC_SLOTS`; até 6 emotes).
2. **Existem de fato:** 11 trajes, 2 picaretas (padrão e Âncora de Coleta), 3 planadores (padrão, Arraia e Folha ao Vento) e 1 emote (Aceno, que usa a animação `acenar`). **Não existem:** mochilas, outros emotes, acessórios e trilhas (o slot existe, mas não há item nem modelo). **Emote na partida: não implementado** (há a ação de entrada "emote", mas não há mensagem no protocolo).
3. Imagem de cada item renderizada do modelo real, com fundo na cor da raridade. Visualizador 3D (girar; Corpo, Rosto e Costas).
4. Os mesmos itens. Mochilas e emotes novos serão **criados originais para o Brisa** (pedido explícito do escopo), sobre o mesmo esqueleto e a mesma paleta.
5. Imagens da loja em 128×160 com CLUT 8 bits. O visualizador gira o modelo real em tempo real (o PS2 aguenta um personagem em close).
6. Mochilas (novo osso `Chest` + malha presa), emotes (novas ações no gerador) e um 3º slot visual na partida.

## 26. Inventário de cosméticos (Armário) — IMPL

1. Perfil → Coleção: equipar por espaço, só o que se possui. Desbloqueios com origem e data.
4. Tela Armário com categorias, grade de cartões e visualizador 3D.
5. Nada.
6. Tela e lógica local.

## 27. Moeda virtual (Faíscas) — IMPL

1. `progressao.py`: ledger com saldo e motivo, chave de idempotência e saldo nunca negativo.
2. +50 por nível de conta, bônus de 100 no nível 5 e 200 no 10, missões, desafios e passe. **Sem dinheiro real.**
4. Igual, num ledger local curto (últimas N entradas) dentro do save.
5. O extrato completo vira as últimas 32 entradas.
6. Lógica local.

## 28. Passe de batalha gratuito — IMPL

1. `backend/config/seasons/t1.json`: "Temporada 1: Primeiros Ventos", de 17/11/2026 a 05/01/2027, **50 níveis de 1.450 XP**, 100% gratuito. Cada nível entrega a recompensa na hora.
2. Recompensas: Faíscas na maioria dos níveis (30, 60 ou 150), traje Explorador no nível 8, Botânico no 18, planador Folha no 25, Astrônomo no 30 e Guardião no 50. 8 desafios cumulativos (50 partidas, 100 eliminações, 150 baús, 5 vitórias, 25 top 10, 1.000 construções, 10.000 de dano, 10.000 de coleta). Meta de cerca de 27,5 h até o nível máximo (calculada pela ferramenta `validar_temporada.py`).
3. Trilha horizontal com a cor da raridade, OBTIDO/ATUAL e rolagem até o nível atual.
4. Igual, num arquivo de conteúdo do Brisa.
5. Nada nas regras. A interface mostra 5 níveis por vez.
6. Lógica local, validador próprio no pipeline e a tela.

## 29. Missões — IMPL

1. `backend/config/missoes.json`: 3 diárias por dia e 5 semanais por semana ISO, sorteadas com semente pela data (iguais para todos).
2. Dez tipos de contador: partidas, eliminações, baús, minutos vivo, top 10, vitórias, coleta, construções, dano e curas. Diárias valem de 150 a 200 XP e 50 Faíscas; semanais de 500 a 1.000 XP e 150 a 200 Faíscas.
4. Igual, pelo relógio do console (RTC).
5. Nada.
6. Lógica local.

## 30. Loja rotativa — IMPL

1. `backend/meta_server/loja.py`. A rotação é calculada, não guardada: Destaques pela segunda-feira e Diária por uma corrente determinística dos últimos 30 dias (não repete ontem nem os Destaques da semana). Uma agenda explícita (`agenda.json`) tem precedência.
2. **Destaques: 2 itens por semana. Diária: 4 itens por dia** (o GAME_DESIGN diz 2–4 e 6; vale a configuração). Preços: comum 200, incomum 300, raro 600, épico 900, lendário 1.500. Compra atômica (confere preço, posse e saldo) com chave de idempotência. Itens do passe não voltam (`itens_do_passe_voltam: false`). 7 itens vendáveis hoje.
3. Cartões na cor da raridade, temporizador, "JÁ É SEU", confirmação com o saldo depois e "Equipar agora".
4. **O mesmo algoritmo determinístico**, rodando no console com a data do RTC. Sem RTC válido, a rotação avança por partidas jogadas (configurável).
5. Nada nas regras.
6. Lógica local e tela.

## 31. Temporadas — IMPL

1. Uma temporada por arquivo, com datas, níveis, recompensas e desafios. Validada ao subir o servidor (temporada inválida não sobe). Os itens ganhos ficam para sempre.
4. Para um jogo offline, a temporada **é um pacote de conteúdo**: a ativa é a última instalada (base ou patch), e as datas são opcionais. Assim, quem joga sem atualizar não fica com o passe "vencido".
5. Sem temporadas sobrepostas.
6. Lógica local.

## 32. Salvamento — IMPL (servidor) → refazer (Memory Card)

1. No Ventania, tudo o que persiste fica no meta-servidor (SQLite WAL, transações, ledger). O cliente guarda só configurações (`user://settings.json`, escrita atômica pelo `SafeFile`), token de sessão e um cache do perfil.
4. No Brisa, **tudo vai para o Memory Card**: perfil, nível, XP, Faíscas, desbloqueios, equipados, passe, missões e configurações.
6. Do zero: formato binário com versão, CRC32 e contador de geração, dois arquivos A/B com escrita alternada, ícone 3D do save e tratamento de cartão ausente, cheio ou removido (ver `BRISA_ARCHITECTURE.md` §9).

## 33. Outros sistemas encontrados no código

| Sistema | Estado | Uso no Brisa |
|---|---|---|
| **Cura e escudo**: Curativo (+15 até 75, 2,5 s), Kit de cura (+100, 7 s), Célula P (+25 de escudo até 50, 2 s), Célula G (+50 até 100, 4 s); vida 100 e escudo 100; o dano passa primeiro pelo escudo | IMPL | Igual (Etapa 6) |
| **Ônibus dos Ventos e planador**: rota sorteada a 120 m de altitude e 20 m/s; queda livre (26 m/s, mergulho até 38), planador automático abaixo de 38 m | IMPL | Etapa 8, depois do aparecimento direto no chão (o mesmo caminho que o Ventania seguiu na v0.4) |
| **Campo de treino** com bonecos e kit de 300 materiais | IMPL | Modo Treino offline (útil para os testes) |
| **Configurações**: 3 perfis gráficos, áudio por barramento, remapeamento | IMPL | Volume, sensibilidade, inverter eixo, vibração, posição da tela e 480p (se suportado) |
| **Áudio**: 37 efeitos de jogo e 9 de interface, sintetizados por script (`tools/audio/`), sem música | IMPL | Converter para ADPCM do SPU2; música original fica a criar |
| **Dano de queda** ("acima de uma altura limite") | **DOC** (não há código) | Decidir na Etapa 4 |
| **Explosivos** | DOC | Fora do escopo inicial |
| **Dupla, Esquadrão, revive** | DOC | Fora do escopo inicial (talvez tela dividida no futuro) |
| **Amigos, contas, matchmaking, sala privada, servidor dedicado, reconexão** | IMPL | Fora do escopo (offline). "Sala privada" vira **Partida personalizada** local: número de bots, dificuldade e Tormenta |

## 34. Divergências encontradas entre documentos e código do Ventania

| Onde | Documento diz | Código/dados dizem |
|---|---|---|
| GAME_DESIGN §7 | "Não há ônibus nem paraquedas"; "construções ainda não são destrutíveis" | Ônibus, queda e planador existem (v0.5.1); as construções do mapa são destrutíveis (v0.5.0). Texto desatualizado |
| ROADMAP Etapa 5 | 466 peças do mapa | **646** peças (extrator, v1.2.1): as cidades da v0.5.1 acrescentaram peças |
| GAME_DESIGN §9.5 | Destaques 2–4 itens, Diária 6 | `shop/loja.json`: Destaques 2, Diária 4 |
| ROADMAP Etapas 10 e 11 | Recompensas Recruta Tormenta, Brisa Aurora…; 8 trajes recoloridos na loja | Substituídos na Etapa 13 por personagens novos (Explorador, Botânico, Astrônomo, Guardião; loja com 7 itens) |
| ROADMAP Etapa 13 | Botânico Incomum | Catálogo `passe_t1.json`: Botânico **Épico** |
| GAME_DESIGN §3 | Modos Solo, Dupla, Esquadrão | Só `br` (solo) e `treino` |
| GAME_DESIGN §4 | Dano de queda | Não implementado |
| GAME_DESIGN §9.5 | Preços a partir de Incomum 300 | Existe também Comum 200 |
| `itens.json` | `materiais._doc`: "ainda não há como obtê-los" | A coleta existe desde a v0.5.0 (comentário antigo) |

Essas divergências **não serão corrigidas no Ventania** (o Brisa não modifica
o Ventania). Ficam registradas para quem mantém o Ventania, e o Brisa usa os
valores do código.
