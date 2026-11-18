#!/usr/bin/env bash
# Compila as ferramentas de teste em emulador do Brisa a partir do Play!
# (emulador de PS2 com BIOS em alto nível: não precisa do dump da BIOS).
#   - AutoTest do Play! (roda um ELF sem vídeo e grava o stdout do EE)
#   - brisa_captura (tools/emulador/captura_play): GS em OpenGL (Mesa em software),
#     controle simulado e captura de quadros.
# Validado na Etapa 1 (2026-11-18) com o Play! no commit 1f27232 (2026-10-24).
# Variáveis: PLAY_SRC (padrão /opt/play-src), PLAY_BUILD (/opt/play-autotest), CAPTURA_BUILD (/opt/brisa-captura).
set -euo pipefail
AQUI="$(cd "$(dirname "$0")/../.." && pwd)"
PLAY_SRC="${PLAY_SRC:-/opt/play-src}"
PLAY_BUILD="${PLAY_BUILD:-/opt/play-autotest}"
CAPTURA_BUILD="${CAPTURA_BUILD:-/opt/brisa-captura}"
apt-get install -y -q cmake g++ zlib1g-dev libgl-dev libglx-dev libglu1-mesa-dev libx11-dev xvfb libgl1-mesa-dri
if [ ! -d "$PLAY_SRC/.git" ]; then
  git clone --depth 1 https://github.com/jpd002/Play- "$PLAY_SRC"
fi
git -C "$PLAY_SRC" submodule update --init --recursive --depth 1
cmake -S "$PLAY_SRC" -B "$PLAY_BUILD" -DBUILD_PLAY=OFF -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build "$PLAY_BUILD" --target autotest -j"$(nproc)"
cmake -S "$AQUI/tools/emulador/captura_play" -B "$CAPTURA_BUILD" -DPLAY_SRC="$PLAY_SRC" -DCMAKE_BUILD_TYPE=Release
cmake --build "$CAPTURA_BUILD" --target brisa_captura -j"$(nproc)"
echo "AUTOTEST=$PLAY_BUILD/tools/AutoTest/autotest"
echo "CAPTURA=$CAPTURA_BUILD/brisa_captura"
