#!/usr/bin/env bash
# Confere o toolchain PS2DEV: versões, bibliotecas e IRX necessários e compila a sonda.
set -uo pipefail
cd "$(dirname "$0")"
source ./env.sh
falhas=0
ok() { echo "[OK] $*"; }
erro() { echo "[ERRO] $*"; falhas=$((falhas + 1)); }
for b in mips64r5900el-ps2-elf-gcc mips64r5900el-ps2-elf-g++ mipsel-none-elf-gcc dvp-as openvcl ps2-packer ps2client adpenc bin2c; do
  if command -v "$b" >/dev/null; then ok "$b: $("$b" --version 2>/dev/null | head -1)"; else erro "falta $b"; fi
done
for l in libpad libmc libaudsrv libpacket2 libdma libgraph libmath3d libcdvd libpatches; do
  [ -f "$PS2SDK/ee/lib/$l.a" ] && ok "$l.a" || erro "falta $PS2SDK/ee/lib/$l.a"
done
for l in libgskit libdmakit; do
  [ -f "$GSKIT/lib/$l.a" ] && ok "$l.a (gsKit)" || erro "falta $GSKIT/lib/$l.a"
done
for i in sio2man padman mcman mcserv libsd audsrv usbd bdm bdmfs_fatfs usbmass_bd; do
  [ -f "$PS2SDK/iop/irx/$i.irx" ] && ok "$i.irx" || erro "falta $i.irx"
done
( cd sonda && make clean >/dev/null 2>&1; make >/tmp/brisa_sonda.log 2>&1 ) && [ -f sonda/sonda.elf ] \
  && ok "sonda.elf: $(file -b sonda/sonda.elf | cut -d, -f1-2)" || { erro "sonda não compilou (ver /tmp/brisa_sonda.log)"; }
( cd sonda && make clean >/dev/null 2>&1 )
[ "$falhas" -eq 0 ] && echo "TOOLCHAIN_OK" || { echo "TOOLCHAIN_FALHOU ($falhas)"; exit 1; }
