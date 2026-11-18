#!/usr/bin/env bash
# Instala o toolchain PS2DEV pré-compilado (Linux x86_64) em $PS2DEV (padrão /opt/ps2dev).
# Validado na Etapa 0 em 2026-11-18: release "latest" do ps2dev/ps2dev,
# GCC 15.2.0 (EE mips64r5900el-ps2-elf e IOP mipsel-none-elf), binutils 2.45.1, gsKit e PS2SDK.
set -euo pipefail
PS2DEV="${PS2DEV:-/opt/ps2dev}"
URL="${PS2DEV_URL:-https://github.com/ps2dev/ps2dev/releases/download/latest/ps2dev-ubuntu-latest.tar.gz}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
echo "Baixando $URL"
curl -fSL -o "$TMP/ps2dev.tar.gz" "$URL"
sha256sum "$TMP/ps2dev.tar.gz"
mkdir -p "$(dirname "$PS2DEV")"
tar -xzf "$TMP/ps2dev.tar.gz" -C "$(dirname "$PS2DEV")"
echo "Instalado em $PS2DEV. Adicione ao ambiente:"
echo "  source tools/ambiente/env.sh"
