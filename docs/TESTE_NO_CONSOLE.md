# Teste no PlayStation 2 de verdade

O emulador não garante compatibilidade, então **o aceite de cada etapa precisa
deste teste**. Ele leva cerca de 15 minutos.

## 1. O que você precisa

- Um PS2 que já rode homebrew (por exemplo, com FreeMcBoot no Memory Card e uLaunchELF ou OPL).
- Um pendrive em **FAT32** e um DualShock 2 na **porta 1**.
- Opcional, para medir pelo PC: um PS2 com rede (o slim, ou o fat com adaptador de rede), um cabo de rede e o `ps2link` no console.

## 2. Arquivo

`build/ps2/brisa.elf` (cerca de 240 KB). É gerado por `make`, ou você recebe pronto.

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
