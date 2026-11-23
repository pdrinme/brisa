# Brisa — PlayStation 2 Edition

Battle Royale 3D cartunesco com construção, para **uma pessoa contra bots**, no
**PlayStation 2 original** (C/C++, PS2SDK, gsKit). É uma edição independente da
família **Ventania** ([pdrinme/ventania](https://github.com/pdrinme/ventania)):
tem a mesma personalidade, a mesma direção de arte e os mesmos sistemas, mas
código, motor, pipeline, build e documentação são próprios.

> Não é o Ventania portado. Nada do projeto Godot roda no PS2. O Ventania é
> consultado como referência (somente leitura), e seus assets originais são
> convertidos por um pipeline próprio.

## Status

**Etapa 6 (combate e inventário): pronta e testada no PC e no emulador; falta
jogar no console.** O ELF abre na **Ilha Ventania inteira** (Etapa 5), agora com:
- loot no chão e baús que abrem;
- as 5 armas do Ventania com raridades, munição, curas e escudos;
- a picareta e o inventário de 5 espaços com a tela de inventário;
- a arma nas duas mãos (IK) e o HUD de combate.

O campo de treino (L1 + R1 + START) tem os bonecos de treino para testar o
combate. Os testes no console das Etapas 1 a 6 continuam pendentes
([docs/TESTE_NO_CONSOLE.md](docs/TESTE_NO_CONSOLE.md)). Veja
[BRISA_ROADMAP.md](BRISA_ROADMAP.md).

![Inventário no emulador, pelo VU1](docs/etapa6/emulador_inventario_vu1.png)

## Compilar e testar

```bash
source tools/ambiente/env.sh
```

```bash
make
```

```bash
make testes
```

```bash
make previa
```

```bash
make emulador
```

| Alvo | Resultado |
|---|---|
| `make` | `build/ps2/brisa.elf` (o que vai para o console, com o pacote de conteúdo embutido) |
| `make conteudo-ventania VENTANIA=../ventania` | Reconverte os assets a partir do Ventania (só ao sincronizar) |
| `make dados` | Recompila `data/*.json` → `assets/dados/*.bdat` (sem o Ventania) |
| `make testes` | Testes do núcleo no PC (`TESTES_OK`) |
| `make previa` | Cena renderizada no PC (referência) em `build/previa/*.png` |
| `make emulador` | O ELF no Play! (sem BIOS) com vídeo: os roteiros do treino e da ilha, com imagens comparadas às do PC no mesmo passo, e o controle simulado; quadros em `build/emulador/` (`EMULADOR_OK`) |
| `make emulador-completo` | O ELF no AutoTest do Play! até o fim dos roteiros (treino e as 16 vistas da ilha) |

As ferramentas de emulador são instaladas por `tools/ambiente/instalar_emulador.sh`.

## Documentos

| Documento | Conteúdo |
|---|---|
| [docs/estudo/SISTEMAS_VENTANIA.md](docs/estudo/SISTEMAS_VENTANIA.md) | O Ventania sistema por sistema: como funciona, regras, visual, adaptação ao PS2, o que simplificar e o que refazer, com o estado real do código |
| [BRISA_ARCHITECTURE.md](BRISA_ARCHITECTURE.md) | Hardware, módulos, renderização, memória, formatos, save, patches, testes, decisões |
| [BRISA_GAME_DESIGN.md](BRISA_GAME_DESIGN.md) | Regras do Brisa, valores iniciais (do Ventania) e o que muda |
| [BRISA_ROADMAP.md](BRISA_ROADMAP.md) | Etapas 0–13, aceites e riscos |
| [ASSET_LICENSES.md](ASSET_LICENSES.md) | Política, reaproveitamento arquivo a arquivo, dependências |
| [docs/PIPELINE_ASSETS.md](docs/PIPELINE_ASSETS.md) | Conversão Ventania → Brisa (modelos, texturas, mapa, dados, áudio) |
| [docs/ESTILO_VISUAL_PS2.md](docs/ESTILO_VISUAL_PS2.md) | A identidade visual do Ventania traduzida para o PS2 |
| [docs/FORMATOS.md](docs/FORMATOS.md) | Formatos binários do Brisa (pacote, modelo, animação, textura) |
| [docs/FERRAMENTAS_PS2DEV.md](docs/FERRAMENTAS_PS2DEV.md) | Toolchain validado, versões, emulador Play! e como testar |
| [docs/TESTE_NO_CONSOLE.md](docs/TESTE_NO_CONSOLE.md) | Roteiro do teste no PS2 real e o que reportar |
| [docs/SINCRONIA_VENTANIA.md](docs/SINCRONIA_VENTANIA.md) | Versão de referência do Ventania e o processo de sincronia |
| [docs/auditoria/](docs/auditoria/) | Saídas medidas: assets, reaproveitamento, layout da ilha, densidade por setor |

## Ferramentas de auditoria do Ventania

```bash
tools/ambiente/instalar_ps2dev.sh
```

```bash
tools/ambiente/verificar_toolchain.sh
```

```bash
python3 tools/auditoria/auditar_ventania.py --ventania ../ventania --json docs/auditoria/assets_ventania.json --md docs/auditoria/assets_ventania.md
```

```bash
python3 tools/auditoria/lista_reuso.py docs/auditoria/assets_ventania.json > docs/auditoria/REUSO_GLB.md
```

```bash
GODOT=/caminho/godot-4.7.2 tools/auditoria/extrair_ilha.sh ../ventania docs/auditoria/ilha_ventania_layout.json
```

```bash
python3 tools/auditoria/densidade_mapa.py --layout docs/auditoria/ilha_ventania_layout.json --assets docs/auditoria/assets_ventania.json --setor 64
```

## Estrutura (a de agora e a planejada)

```
brisa-ps2/
├── src/
│   ├── nucleo/              ✅ tipos, arena, passo fixo, controle, estatísticas, lista 2D, fonte, formato, mat (3D)
│   ├── render3d/            ✅ malhas, cena, luz toon, pipeline de referência (caminho EE)
│   ├── jogo/                ✅ telas (tela_boot, cena_teste, vitrine, treino, ilha, jogador, roteiros do autoteste)
│   ├── plataforma/ps2/      ✅ IOP/IRX, gsKit, libpad, relógio, caminho EE (GIF), caminho VU1 (VIF1 + vu1/toon.vsm)
│   ├── plataforma/host/     ✅ PC: rasterizador de software, PNG, controle por roteiro
│   ├── main.cpp             ✅
│   ├── conteudo/            ✅ pacote, modelos, animações (mistura de poses), texturas, biblioteca de modelos
│   ├── mundo/               ✅ colisão (caixas, rampas, cunhas, relevo, grade, raio), mapa (BMAP), relevo 3D
│   ├── personagem/          ✅ movimento, câmera em 3ª pessoa, animador, portador (item na mão, IK)
│   ├── combate/             ✅ itens, inventário, vida/escudo, tiro, loot
│   └── (planejado) armas/ inventario/ construcao/ bots/ partida/ ui/ audio/ progressao/ save/
├── data/                    ✅ JSON de jogo (itens.json; cosméticos, temporadas e loja depois)
├── assets/                  ✅ assets convertidos (modelos, animações, texturas) + catalogo_brisa.json
├── tools/
│   ├── ambiente/            ✅ instalar e verificar o toolchain (+ sonda)
│   ├── auditoria/           ✅ leitor GLB, auditoria, extrator da ilha, densidade, reuso
│   ├── emulador/            ✅ capturador no Play!, verificação da saída, PPM→PNG
│   ├── conversor/           ✅ GLB → BMOD/BANI, PNG → BTEX, mapa (BMAP, estradas, minimapa), QEM, catálogo, empacotador, testes
│   ├── (planejado) conversor_mapa/ compilador_dados/
├── tests/                   ✅ testes no PC (núcleo, 3D, conteúdo, personagem, mapa)
├── docs/
├── Makefile                 ✅
└── BRISA_*.md, ASSET_LICENSES.md, CHANGELOG.md
```
