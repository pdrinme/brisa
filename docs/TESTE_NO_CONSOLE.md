# Teste no PlayStation 2 de verdade

O emulador não garante compatibilidade, então **o aceite de cada etapa precisa
deste teste**. Ele leva cerca de 15 minutos.

## 1. O que você precisa

- Um PS2 que já rode homebrew (por exemplo, com FreeMcBoot no Memory Card e uLaunchELF ou OPL).
- Um pendrive em **FAT32** e um DualShock 2 na **porta 1**.
- Opcional, para medir pelo PC: um PS2 com rede (o slim, ou o fat com adaptador de rede), um cabo de rede e o `ps2link` no console.

## 2. Arquivo

`build/ps2/brisa.elf` (cerca de 5,4 MB a partir da Etapa 3, com o conteúdo embutido). É gerado por `make`, ou você recebe pronto.

## 3. Pelo pendrive (uLaunchELF)

1. Copie `brisa.elf` para a raiz do pendrive.
2. No PS2, abra o uLaunchELF → `mass:/` → `brisa.elf`.
3. Pelo OPL: coloque o ELF na pasta `APPS/` do pendrive e abra pela aba "Aplicativos".

## 4. Pela rede (ps2link + ps2client), que mostra o log no PC

1. No console, abra o `ps2link` (ele mostra o IP).
2. No PC:

```bash
ps2client -h 192.168.0.10 execee host:brisa.elf
```

   Rode o comando dentro da pasta `build/ps2/` e troque o IP pelo que o console mostrar.
3. As linhas `BRISA: ...` aparecem no terminal do PC. A cada 600 quadros sai uma linha com o tempo de quadro.

## 5. O que conferir (e me mandar)

Na tela do Brisa:

| Verificar | Esperado |
|---|---|
| Imagem | Fundo azul-petróleo → índigo, faixas de vento passando, "BRISA" grande e branco com contorno escuro, e "PLAYSTATION 2 EDITION" em amarelo |
| Quadrado | Gira sozinho, sem tremer nem picotar |
| Controle | "CONTROLE 1: ANALÓGICO" (a luz ANALOG do controle acesa) |
| Botões | Cada botão e o direcional acendem em amarelo; os pontos dos analógicos se movem; L3 e R3 acendem ao apertar os analógicos |
| ✕ | Troca a cor do quadrado (coral → amarelo → verde-água → branco) |
| L1 / R1 | Diminui / aumenta o quadrado |
| Analógico direito | Para a direita, gira mais rápido; para a esquerda, mais devagar ou ao contrário |
| START | Aparece "PAUSADO" e tudo para; START de novo continua |
| Desconectar e reconectar o controle | "DESCONECTADO" em vermelho; ao reconectar, volta para ANALÓGICO |
| 10 minutos parado na tela | Não trava, a imagem continua e o FPS não cai |

**Me mande** uma foto da tela (com o rodapé legível) depois de 10 minutos e
estes valores do rodapé:
- QUADRO e FPS (esperado: 16,7 ms e 59,9 FPS em NTSC, ou 20,0 ms e 50,0 FPS em PAL);
- CPU e PIOR;
- MEMÓRIA LIVRE e PROGRAMA;
- a linha do vídeo;
- o modelo do console (SCPH-…), a região e como você abriu o ELF (uLaunchELF, OPL ou ps2link).

Se a tela ficar preta, se a imagem rolar ou se o controle não responder, anote
em que momento aconteceu e o que aparece.

## 6. Valores do emulador, para comparar

Medidos no Play! (sem BIOS), que **não** é uma medição de console:
- quadro de 16,7 ms (59,8 FPS);
- CPU de 1,4 a 1,6 ms;
- 28,0 MB livres;
- programa de 572 KB;
- NTSC 640×448 entrelaçado;
- módulos da ROM (XSIO2MAN+XPADMAN).

---

# Etapa 2 — Motor gráfico (benchmark no console)

Os números de desempenho **só valem se forem medidos no console**: o emulador
não reproduz a velocidade do EE, do VU1 nem do GS. Este é o teste que decide
entre os dois caminhos de renderização (BR-ADR-1).

## O que aparece

A cena de teste: chão de grama, casas de tábuas, torres de pedra, galpões de
metal, pinheiros, árvores redondas, baús azul-petróleo e amarelo e pedras, com
luz em 2 faixas (sombra azulada), Z-buffer e neblina no horizonte. Em cima fica
um painel com o caminho, os objetos, os triângulos, o quadro, a CPU, o tempo do
3D e o FPS.

## Controles

| Botão | Efeito |
|---|---|
| SELECT | Troca o caminho: **VU1** (microprograma próprio) ↔ **EE** (CPU) |
| L1 / R1 | Menos / mais objetos (níveis 1 a 7: 24 a 1.536 objetos) |
| ● | Alterna para a "carga": esferas de 1.280 triângulos (níveis 1 a 5: 4 a 48 esferas) |
| Analógico esquerdo | Gira a câmera e aproxima/afasta |
| Analógico direito (vertical) | Altura da câmera |
| START | Para a rotação automática |
| **▲** | **Benchmark automático** (cerca de 2 minutos): mede as 12 configurações em cada caminho |

## O que me mandar

1. Aperte **▲** e espere o painel mostrar "BENCHMARK (MS: QUADRO/CPU/3D)". Tire uma foto da tabela, que mostra as últimas 11 linhas.
2. Se estiver usando o **ps2link**, mande as linhas `BRISA_BENCH ...` do terminal: são as 24 medições completas.
3. Em cada caminho, confira se as imagens são iguais: casas com o lado do sol claro e o da sombra azulado, a neblina no horizonte, e nada "atravessando" (Z-buffer).
4. Anote se a imagem treme, se aparecem triângulos voando ou buracos, e em qual caminho.

## Valores do emulador (Play!), só para referência

Eles não medem o console. Ver `BRISA_ROADMAP.md`, Etapa 2.

---

# Etapa 3 — Vitrine do kit com os assets do Ventania

O ELF agora abre na **vitrine do kit**: a mesma composição da imagem da
vitrine do Ventania (`docs/art/vitrine_kit_quality.png` de lá), com os
modelos, as animações e os ícones convertidos.

| Botão | Efeito |
|---|---|
| L1 / R1 | Troca o traje do personagem da esquerda (11 trajes) |
| R2 | LOD: AUTO → 0 → 1 → 2 |
| ○ / ■ | Próxima / anterior das 16 animações |
| SELECT | Caminho VU1 ↔ EE |
| ✕ | Pausa a animação |
| START | Mostra/esconde o painel |
| ▲ | Volta a câmera para a posição da vitrine |
| L3 segurado + R3 | Alterna para a cena de benchmark da Etapa 2 |

**Conferir e me mandar:**
1. Uma foto da TV **sem o painel** (START), com a câmera na posição inicial. Ela vai ser comparada com `docs/etapa3/ventania_x_brisa_emulador.png`.
2. O painel mostra "CRC OK"? (o pacote embutido conferido no console)
3. Os 11 trajes aparecem inteiros (sem peças voando ou faltando) nos 3 LODs?
4. As animações tocam sem tremer, os ícones de baixo aparecem com transparência certa e as sombras redondas ficam sob os personagens e os baús?
5. O FPS e o "3D" do painel, nos caminhos VU1 e EE.

---

# Etapa 4 — Personagem no campo de treino

O ELF agora abre no **campo de treino do Ventania**, com o personagem jogável.
A vitrine (Etapa 3) e o benchmark (Etapa 2) continuam no ELF: **L1 + R1 +
START** alterna treino → vitrine → benchmark.

| Botão | Efeito |
|---|---|
| Analógico esquerdo | Anda (o personagem vai para onde a câmera olha) |
| Analógico direito | Gira a câmera |
| ✕ | Pula |
| L3 segurado | Corre (só para a frente) |
| R3 | Agacha / levanta |
| L2 segurado | Mira (câmera mais perto, FOV menor) |
| ◀ no direcional | Acena |
| SELECT | Troca o traje (11) |
| ▼ no direcional | Caminho de desenho VU1 ↔ EE |
| START | Mostra/esconde o painel |

**Conferir e me mandar:**
1. **Subir a torre de pedra** (em frente ao ponto inicial, um pouco à esquerda) pelas duas rampas até o baú no alto. A subida é contínua, sem engasgar na emenda entre rampa e piso? O painel mostra Y 8,1 no alto?
2. **Pular do alto:** o personagem cai, toca a **aterrissagem pesada** e não perde vida (não há dano de queda, BR-ADR-13).
3. **Paredes:** andar contra as paredes da cabana de madeira e do galpão de metal. O personagem para sem atravessar e sem tremer, e desliza quando chega em ângulo?
4. **Agachado:** passar por baixo dos dois pisos de madeira a 1,5 m (à esquerda do ponto inicial). Em pé ele não passa; agachado passa; e embaixo deles R3 não deixa levantar.
5. **Câmera:** encostar as costas numa parede. A câmera chega perto sem entrar na parede e volta devagar ao sair?
6. **Animações:** parado, andando, correndo, agachado, pulando, caindo e acenando. Elas trocam suavemente e os pés não "patinam" demais?
7. **Desempenho:** o FPS e o "3D" do painel no alto da torre e no meio do campo, nos caminhos VU1 e EE. No emulador, o VU1 dá 59,8 FPS e 1,3–2,6 ms de 3D, e o EE dá 35 FPS e 43 ms. **Esses valores não medem o console.**
8. **Pelo ps2link:** as linhas `BRISA: quadro …` a cada 300 quadros.

Se algo atravessar, travar ou tremer, anote onde (os valores X, Y e Z do
painel) e o que você estava fazendo.
