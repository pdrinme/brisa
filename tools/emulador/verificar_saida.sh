#!/usr/bin/env bash
# Confere a saída (stdout do EE) do autoteste rodado no emulador.
# Uso: tools/emulador/verificar_saida.sh <arquivo> --parcial|--completo
#   --parcial: captura com controle simulado (o programa é interrompido depois das capturas)
#   --completo: AutoTest até o fim (600 quadros, sem controle)
set -u
r="$1"
modo="${2:---completo}"
falhas=0
confere() { if grep -qE "$2" "$r"; then echo "[OK] $1"; else echo "[ERRO] $1"; falhas=$((falhas + 1)); fi; }
confere "iniciou (versão, vídeo, memória)" "^BRISA [0-9.]+ \(etapa [0-9]+\) em PS2"
confere "módulos do controle carregados" "modulos (IRX EMBUTIDOS|ROM)"
confere "vídeo NTSC ou PAL" "video (NTSC|PAL) 640×(448|512)"
confere "porta do controle aberta" "padPortOpen porta 1: aberta"
if [ "$modo" = "--parcial" ]; then
  confere "controle conectado (estado estável)" "controle 1 conectado"
else
  confere "600 quadros" "quadro 600 \|"
  confere "nenhuma primitiva descartada" "descartadas 0 \|"
  confere "300 passos de 30 Hz" "passos 300$"
  confere "terminou sem travar" "^BRISA_FIM_OK"
fi
[ "$falhas" -eq 0 ] && echo "EMULADOR_OK" || { echo "EMULADOR_FALHOU ($falhas)"; exit 1; }
