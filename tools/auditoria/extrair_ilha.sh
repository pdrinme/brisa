#!/usr/bin/env bash
# Extrai o layout da Ilha Ventania rodando o gerador determinístico do PRÓPRIO
# Ventania na Godot headless, numa CÓPIA temporária do projeto (a importação da
# Godot escreve em .godot/; o repositório do Ventania nunca é alterado).
#
# Uso: tools/auditoria/extrair_ilha.sh <caminho-do-ventania> <saida.json>
# Requer: Godot 4.7.2 (GODOT=/caminho/godot).
set -euo pipefail
VENTANIA="$(cd "$1" && pwd)"
SAIDA="$(realpath -m "$2")"
GODOT="${GODOT:-godot}"
AQUI="$(cd "$(dirname "$0")" && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cp -r "$VENTANIA/game" "$TMP/game"
cp "$AQUI/extrair_ilha.gd" "$TMP/game/_brisa_extrair.gd"
echo "Ventania: $(git -C "$VENTANIA" log -1 --format='%h %s' 2>/dev/null || echo 'sem git')"
"$GODOT" --headless --path "$TMP/game" --import >/dev/null 2>&1 || true
"$GODOT" --headless --path "$TMP/game" -s res://_brisa_extrair.gd -- "$SAIDA" 2>&1 | grep -E "EXTRAIDO|ERROR: .*_brisa" || { echo "falhou"; exit 1; }
