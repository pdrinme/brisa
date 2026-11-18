# Ferramentas PS2DEV — validação da Etapa 0

Data: 2026-11-18. Ambiente: contêiner Linux x86_64 sem GPU e sem PS2.

## 1. Toolchain instalado e verificado

| Item | Versão / resultado |
|---|---|
| Pacote | `ps2dev-ubuntu-latest.tar.gz` (release "latest" do ps2dev/ps2dev), 274.977.270 bytes, sha256 `ca22e17c58df40abbcc811fe4613d49f34305cda932102669d35ed534e5e8297` |
| Compilador do EE | `mips64r5900el-ps2-elf-gcc` / `g++` **15.2.0** |
| Compilador do IOP | `mipsel-none-elf-gcc` 15.2.0 |
| Assembler do VU (`dvp-as`) | GNU binutils 2.45.1 |
| OpenVCL (pré-processador de microcódigo do VU) | 0.4.0 |
| ps2-packer, ps2client (rede com ps2link), adpenc (ADPCM), bin2c | presentes |
| PS2SDK — bibliotecas do EE usadas pelo Brisa | libpad, libmc, libaudsrv, libpacket2, libdma, libgraph, libmath3d, libcdvd, libpatches: presentes |
| gsKit | libgskit, libgskit_toolkit, libdmakit: presentes |
| IRX necessários | sio2man, padman, mcman, mcserv, libsd, audsrv, usbd, bdm, bdmfs_fatfs, usbmass_bd: presentes |
| Licença PS2SDK / gsKit | Academic Free License 2.0 (conferida nos arquivos `LICENSE`) |

**Script de verificação:** `tools/ambiente/verificar_toolchain.sh` → `TOOLCHAIN_OK`.
**Instalação:** `tools/ambiente/instalar_ps2dev.sh` e depois `source tools/ambiente/env.sh`.

## 2. Compilações de teste (só validação de toolchain, sem código do Brisa)

| Teste | Resultado |
|---|---|
| Exemplo `cube` do gsKit (3D com gsKit + math3d, Makefile do PS2SDK) | `cube.elf`: ELF 32-bit LSB, MIPS-III N32 (o formato do EE), text 165.812 B |
| Sonda C++17 (`tools/ambiente/sonda/`): `-std=gnu++17 -fno-exceptions -fno-rtti`, templates, ligada com libpad, libmc e audsrv | `sonda.elf` gerado, text 132.692 B |
| Exemplo `cpp-hello` do PS2SDK | Falha: gera um `.erl` com a opção obsoleta `-mno-crt0`. É um problema do exemplo, não do toolchain; não afeta o Brisa, que gera ELF comum |

**Nenhum ELF foi executado:** não há emulador com BIOS nem console neste
ambiente. Executar é o primeiro aceite da Etapa 1.

## 3. Outras ferramentas verificadas

| Ferramenta | Situação |
|---|---|
| Godot 4.7.2-stable (headless) | Baixada e usada para extrair o mapa (só no pipeline) |
| PCSX2 | O download da release (AppImage) responde. **Não instalado:** precisa da BIOS do console do dono, que não pode ser distribuída nem baixada |
| Docker | O cliente existe, mas não há daemon. A imagem `ps2dev/ps2dev` não é necessária (o pacote pré-compilado funciona) |
| Python 3 | Presente; os scripts do pipeline só usam a biblioteca padrão |
| Blender (`bpy` 5.3) | Não instalado nesta etapa. Só seria necessário para o caminho (b) de LOD (`PIPELINE_ASSETS.md` §2) |

## 4. Como testar no PS2 a partir da Etapa 1

1. **Emulador:** PCSX2 com o dump da BIOS do próprio console. Em "Executar ELF", abra `brisa.elf`. Isso serve para iterar rápido, **não para garantir compatibilidade**.
2. **Console via USB:**
   - copie `brisa.elf` e a pasta `BRISA/` (com os `.PAK`) para um pendrive FAT32;
   - abra pelo uLaunchELF ou pelo OPL (num console que já rode homebrew, como o FreeMcBoot).
   - O ELF carrega os módulos USB (`usbd`, `bdm`, `usbmass_bd`, `bdmfs_fatfs`) embutidos.
3. **Console via rede** (PS2 com adaptador de rede ou slim): `ps2link` no console e `ps2client -h <ip> execee host:brisa.elf` no PC. O console lê os dados pelo `host:`, e o log aparece no PC. É o caminho mais rápido para medir desempenho no console.

O caminho de boot (`mass:`, `host:`, `cdrom0:`) é detectado pelo `argv[0]`, e
os dados são procurados ao lado do ELF.
