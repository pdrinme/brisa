# Game Design — Brisa (PlayStation 2)

Versão 0.1 (Etapa 0). Edição independente da família **Ventania**. Os números
partem dos dados do Ventania v1.2.1 (detalhes e origem de cada um em
`docs/estudo/SISTEMAS_VENTANIA.md`). Todos ficam em `data/*.json` e podem ser
balanceados para o Brisa sem mexer no código. Onde o Brisa diverge, a tabela diz
o porquê.

## 1. Identidade

- **O que é:** Battle Royale 3D cartunesco com construção, para **uma pessoa contra bots**, no PlayStation 2 original.
- **Promessa:** parecer um jogo lançado na época do PS2, e não um jogo moderno reduzido. Menus completos, progressão, desbloqueáveis, partidas inteiras e nenhuma dependência de internet.
- **Pilares herdados do Ventania:**
  1. diversão antes de gráficos;
  2. construção rápida e legível;
  3. progressão gratuita e justa: tudo se desbloqueia jogando, sem pagamento;
  4. roda bem no hardware-alvo.
- **Pilar próprio do Brisa:** divertido sozinho. Os bots são o "multiplayer" do jogo.
- **Tema e tom:** vento (redemoinhos, cachecóis, óculos de aviador, cata-ventos, bandeirolas), colorido e bem-humorado. Nada imita outro jogo.
- **Nomes:** jogo **Brisa**, moeda **Faíscas**, zona **a Tormenta**, ilha **Ilha Ventania** (ou uma ilha derivada; ver §6). Os nomes são os do Ventania, que pertencem ao mesmo dono.

## 2. Loop

1. **Tela inicial** → carrega o Memory Card → **lobby** com o personagem equipado.
2. **JOGAR** → escolha do modo → tela de carregamento.
3. **Partida:** aparecer na ilha só com a picareta (mais tarde, pular do Ônibus dos Ventos) → coletar → lutar e construir → fugir da Tormenta.
4. **Fim:** vitória ou eliminação (com espectador) → resultados → XP, missões e passe.
5. **Progresso:** subir o passe, ganhar Faíscas, comprar na loja e equipar no Armário. Salva no Memory Card.

## 3. Modos

| Modo | Descrição | Origem |
|---|---|---|
| **Battle Royale** | 1 jogador + bots, Tormenta completa | Ventania "br" |
| **Partida personalizada** | Número de bots, dificuldade (Fácil, Médio, Difícil, Mista), velocidade da Tormenta e materiais iniciais | Adapta a "sala privada" do Ventania |
| **Treino** | Ilha ou campo com bonecos, 300 de cada material, sem Tormenta e sem XP | Ventania "treino" |

Fora do escopo inicial: Dupla, Esquadrão e revive (só documentados no
Ventania). Uma tela dividida para 2 jogadores pode ser avaliada depois da
Etapa 13, se houver folga de desempenho.

**Participantes:** começa com **12** (1 + 11 bots); a meta é **16**, se as
medições permitirem (BRISA_ARCHITECTURE BR-ADR-6). A mistura de dificuldades é
a do Ventania: Fácil, Médio, Médio, Difícil.

## 4. Personagem e controles (DualShock 2)

- Movimento idêntico ao Ventania: andar 4,5 m/s, correr 6,5 m/s (só para a frente), agachado 2,5 m/s, salto de cerca de 1,4 m, gravidade 22 m/s².
- Câmera por cima do ombro (distância 3,2 m, ombro 0,55 m, FOV 70°; mirando 1,7 m e FOV 55°), com troca de ombro.
- **Vida 100 + escudo 100**; o dano passa primeiro pelo escudo.
- **Dano de queda:** está no GAME_DESIGN do Ventania, mas não foi implementado lá. Proposta para o Brisa: sem dano de queda na primeira versão (decisão na Etapa 4).
- **Mira assistida leve** (opcional, nas configurações): uma desaceleração do analógico sobre um alvo, padrão em jogos de console da época. Ela não existe no Ventania.

| Ação | Botão | Equivalente no controle do Ventania |
|---|---|---|
| Mover / olhar | Analógico esquerdo / direito | igual |
| Pular | ✕ | A |
| Atirar / usar / colocar peça | R2 | RT |
| Mirar | L2 | LT |
| Recarregar / trocar material (construção) | ■ | X |
| Interagir (segurar: abrir baú, pegar) | ▲ | Y |
| Modo construção | ● | B |
| Trocar espaço / peça | L1 / R1 | LB / RB |
| Correr | L3 | L3 |
| Agachar | R3 | R3 |
| Picareta | → (direcional) | ► |
| Mapa | ↑ | ▲ (direcional) |
| Editar peça (se existir no Brisa) | ↓ | ▼ |
| Emote | ← | ◄ |
| Inventário | SELECT | Select |
| Pausa | START | Start |

Todos remapeáveis por um conjunto de esquemas prontos (como em jogos da época),
e não tecla a tecla.

**Implementado na Etapa 6** (BR-ADR-18): este mapeamento, com duas ressalvas
até a Etapa 10. START mostra ou esconde o painel de depuração (a pausa vem com os
menus), e os atalhos de depuração usam L1 + R1 segurados + START (modo), SELECT
(caminho de desenho) ou ● (traje).

## 5. Combate, itens e inventário

Valores iniciais = Ventania (`itens.json`).

- **Armas:** Rajada (Rifle de Assalto), Trovão (Espingarda), Zumbido (Submetralhadora), Faísca (Pistola) e Gavião (Rifle de Precisão). A tabela completa está no estudo, §5.
- **Raridades:** Comum, Incomum, Raro, Épico e Lendário (multiplicadores de dano 1,00 a 1,22 e de recarga 1,00 a 0,80), mais o Mítico (só recompensas e eventos).
- **Munição:** leve 360/30, média 360/30, pesada 60/6, cartucho 80/10 (máximo/caixa).
- **Cura e escudo:** Curativo, Kit de cura, Célula de escudo pequena e grande (valores do Ventania).
- **Inventário:** picareta fixa + 5 espaços, munição e materiais em reserva, e a regra "nunca troca em silêncio".
- **Picareta:** cadência 1,6/s, alcance 2,4 m, 20 de dano em jogadores e 50 em construções.
- **Baús:** 1 arma + 1 consumível + 2 caixas de munição.
- **Explosivos:** fora do escopo inicial (como no Ventania).

## 6. Mapa

- **Ponto de partida:** o layout real da **Ilha Ventania**, extraído do gerador do Ventania. São cerca de 600 m (área jogável de 524 m) e 6 POIs: Vila Brisa, Pedreira Cinza, Porto Ferrugem, Fazenda Girassol, Mirante do Vento e Acampamento Coral. Há 646 peças de construção destrutíveis, 1.219 props, 37 baús, 102 pontos de loot e 24 pontos de início.
- **Decidido na Etapa 5 (BR-ADR-16, provisório até o benchmark no console):** a **Ilha Ventania inteira**, com distância de visão de 240 m e neblina. As opções eram:
  - **(a)** a Ilha Ventania inteira, com distância de visão curta e neblina (**escolhida**);
  - **(b)** uma ilha compacta (cerca de 350–400 m) com os mesmos POIs mais próximos;
  - **(c)** as duas, com (b) como padrão.
- **Por quê:** o mapa inteiro cabe na memória, e a pior vista (Vila Brisa) envia 41,9 mil triângulos, dentro da meta de 40–70 mil (medições em BRISA_ARCHITECTURE §18). A ilha compacta exigiria um layout diferente do Ventania. Ela continua como último recurso, se o console não sustentar a Vila.
- **A densidade** (12–16 participantes em 600 m) é tratada na partida (Etapa 8): Tormenta e pontos de início, medindo o tempo até o primeiro encontro.
- **Tormenta:** as mesmas 7 fases. Se o mapa encolher, os raios são escalados por um fator único em `data/partida.json`.

## 7. Construção

- Grade de 4 m. Parede, piso e rampa desde o início; **teto se o desempenho permitir** (o modelo tem só 44 triângulos, então é provável).
- **Materiais:** madeira (150 de vida em 1 s), pedra (300 em 1,6 s) e metal (500 em 2,4 s). Custo 10; nasce com 15% da vida; máximo de 999 de cada.
- **Destruição** por tiro e picareta; desabamento de peças sem apoio (BFS em lotes).
- **Coleta:** árvore 7, rocha 6, peça do mapa 4 por golpe + 10 ao derrubar.
- **Limite de peças de jogadores:** começa em 300 (o Ventania usa 1.200) e sobe conforme a medição.
- **Edição de ladrilhos** (portas e janelas): depois do básico, se houver desempenho e memória.
- Colisões simples (caixas e planos), sem física de corpos rígidos.

## 8. Bots

O desenho do Ventania (utility scoring com inércia, execução por estados,
percepção sem "saber magicamente" e recuperação de travamento), com as três
dificuldades e os mesmos parâmetros de `data/bots.json`. Comportamentos
obrigatórios para o Brisa:
- explorar (inclusive cômodos e andares de cima);
- procurar armas e abrir baús;
- coletar itens e materiais;
- atacar (mira que assenta, rajadas, distância preferida por arma);
- procurar cobertura, recuar e curar;
- fugir da Tormenta;
- sobreviver.

Os bots Difíceis levantam uma parede ao levar tiro, como no Ventania.
**Critério de aceite** (herdado do Ventania): uma partida completa só com bots
termina com vencedor, e o tempo preso fica abaixo de 1% do tempo vivo.

## 9. Partida

- Início direto (aparecer espalhado só com a picareta, como no Ventania v0.4). O **Ônibus dos Ventos**, a queda livre e o planador vêm na Etapa 8, se o desempenho permitir (eles também dão sentido aos planadores cosméticos).
- Contagem de 10 s, proteção de 3 s no início, fim com resultado em 15 s.
- **Espectador** depois da eliminação (segue quem eliminou; ✕ troca o alvo).
- **Resultados:** colocação, vencedor, eliminações, tempo vivo, dano, baús, curas, materiais e construções, XP detalhado e o passe. Botões Jogar de novo, Lobby e Assistir.

## 10. Interface (TV)

- A linguagem visual do Ventania ("adesivo ao vento"):
  - fundo azul-petróleo → índigo com rajadas animadas;
  - contorno grosso escuro e sombra dura deslocada;
  - botões levemente inclinados com o texto em caixa alta;
  - amarelo só para a ação principal;
  - foco sempre visível com contorno amarelo.
- **Legível em TV de tubo:**
  - texto do corpo com pelo menos 18 px de altura em 640×448;
  - nada importante fora da área segura (90% da tela);
  - sem linhas de 1 px horizontais (cintilam no entrelaçado);
  - contraste alto.
- **Fonte:** Fredoka (OFL-1.1, a mesma do Ventania), rasterizada em bitmap.
- **Telas:** inicial, menu principal, seleção de modo, lobby, passe, loja, Armário, missões, configurações, créditos, carregamento, HUD, inventário, mapa, pausa e resultados.

## 11. Progressão (100% offline e gratuita)

Os valores iniciais são os do Ventania.

- **XP por partida:**
  - participação 50;
  - 10 por minuto vivo (até 20 min);
  - 40 por eliminação (até 15);
  - top 10 +60, top 5 +120, vitória +300;
  - 10 por baú (até 20);
  - coleta e construções com teto.

  O treino não dá XP.
- **Nível de conta:** 800 + 80 × (n − 1) de XP por nível, teto de 3.000, máximo 200. +50 Faíscas por nível, bônus nos níveis 5 e 10, e o emote Aceno no nível 2.
- **Missões:** 3 diárias e 5 semanais, pelo relógio do console (as mesmas listas do Ventania).
- **Passe:** 50 níveis de 1.450 XP, recompensas a cada nível e 8 desafios de temporada. Meta de 25–30 h. **Sem trilha premium.**
- **Ajuste específico do Brisa:** com 12–16 participantes (contra 16–24 no Ventania), as fontes de XP por eliminação ficam menores por partida. O validador de temporada do Brisa (Etapa 11) recalcula as horas até o nível 50 e o balanceamento é corrigido nos dados.

## 12. Cosméticos, loja e Armário

- **Categorias:** Personagens (trajes), Picaretas, Mochilas, Emotes, Planadores (se o ônibus entrar) e Outros (acessórios). Nenhum cosmético altera hitbox, tamanho, velocidade ou dano.
- **Do Ventania (convertidos):** 11 trajes, 2 picaretas, 3 planadores e o emote Aceno.
- **Novos e originais do Brisa:** mochilas (o Ventania não tem nenhuma) e emotes. A primeira leva (Etapa 12) tem 3 mochilas e 4 emotes, no mesmo esqueleto e na mesma paleta. Os conceitos ainda serão aprovados.
- **Loja:**
  - Destaques (2 itens, troca semanal) e Diária (4 itens, troca diária);
  - o mesmo algoritmo determinístico do Ventania, com semente pela data do RTC do console;
  - sem RTC válido, troca a cada N partidas (configurável);
  - agenda explícita por arquivo de conteúdo;
  - preços: comum 200, incomum 300, raro 600, épico 900, lendário 1.500 Faíscas.
- **Temporada = pacote de conteúdo.** A Temporada 1 do Brisa reaproveita a estrutura da "Primeiros Ventos" (as mesmas recompensas: Explorador, Botânico, Folha, Astrônomo e Guardião). As datas são opcionais (§11 da arquitetura).
- **Armário:** ver, equipar (traje, picareta, mochila, planador, 6 emotes) e coleção com a origem de cada item.

## 13. Fora do escopo

Online e rede, contas, amigos, grupos, matchmaking, dinheiro real, veículos,
explosivos, Dupla/Esquadrão/revive, mapa acima de 600 m e mais de 16
participantes (até que medições digam o contrário).
