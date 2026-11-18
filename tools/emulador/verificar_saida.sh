#!/usr/bin/env bash
# Confere a saída (stdout do EE) do autoteste rodado no emulador.
# Uso: tools/emulador/verificar_saida.sh <arquivo> --cena|--controle|--completo
#   --cena: captura sem botões (roteiro: a mesma cena pelo VU1 e pelo EE)
#   --controle: captura com o controle simulado
#   --completo: AutoTest até o fim (roteiro e benchmark inteiro)
set -u
r="$1"
modo="${2:---completo}"
falhas=0
confere() { if grep -qE "$2" "$r"; then echo "[OK] $1"; else echo "[ERRO] $1"; falhas=$((falhas + 1)); fi; }
confere "iniciou (versão, vídeo, memória)" "^BRISA [0-9.]+ \(etapa [0-9]+\) em PS2"
confere "módulos do controle carregados" "modulos (IRX EMBUTIDOS|ROM)"
confere "vídeo NTSC ou PAL" "video (NTSC|PAL) 640×(448|512)"
confere "porta do controle aberta" "padPortOpen porta 1: aberta"
confere "microprograma do VU1 carregado" "VU1 pronto"
confere "malhas da cena criadas" "malhas da cena: [0-9]+ KB"
if [ "$modo" = "--controle" ]; then
  confere "controle conectado (estado estável)" "controle 1 conectado"
elif [ "$modo" = "--cena" ]; then
  confere "cena pelo VU1 (roteiro)" "BRISA_ROTEIRO caminho=VU1 .*lotes_vu1=[1-9]"
  confere "cena pelo EE (roteiro)" "BRISA_ROTEIRO caminho=EE .*desenhados_ee=[1-9]"
else
  confere "cena pelo VU1 (roteiro)" "BRISA_ROTEIRO caminho=VU1 .*lotes_vu1=[1-9]"
  confere "cena pelo EE (roteiro)" "BRISA_ROTEIRO caminho=EE .*desenhados_ee=[1-9]"
  n=$(grep -c "^BRISA_BENCH caminho=" "$r")
  if [ "$n" -eq 24 ]; then echo "[OK] 24 medições do benchmark"; else echo "[ERRO] benchmark com $n medições (esperado 24)"; falhas=$((falhas + 1)); fi
  confere "benchmark terminou" "^BRISA_BENCH_FIM"
  confere "terminou sem travar" "^BRISA_FIM_OK"
fi
[ "$falhas" -eq 0 ] && echo "EMULADOR_OK" || { echo "EMULADOR_FALHOU ($falhas)"; exit 1; }
