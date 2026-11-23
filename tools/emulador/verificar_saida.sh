#!/usr/bin/env bash
# Confere a saída (stdout do EE) do autoteste rodado no emulador.
# Uso: tools/emulador/verificar_saida.sh <arquivo> --cena|--controle|--completo
#   --cena: o roteiro do campo de treino (Etapa 4) até a corrida, com captura de quadros
#   --completo: o roteiro inteiro (AutoTest, até o programa sair)
#   --controle: captura com o controle simulado
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
confere "pacote de conteúdo íntegro (CRC)" "BRISA_CONTEUDO arquivos=[0-9]+ .*crc_falhas=0"
if [ "$modo" = "--controle" ]; then
  confere "controle conectado (estado estável)" "controle 1 conectado"
else
  # Roteiro do campo de treino (src/jogo/roteiro_treino.h), as mesmas expectativas de tests/teste_personagem.cpp.
  confere "subiu a torre pelas rampas (piso a 8 m)" "^BRISA_MARCO topo_da_torre .*pos=-?0\.[0-9]+,8\.[01][0-9],-2[4-9]\.[0-9]+ no_chao=1"
  confere "caiu da torre (queda >= 6 m, aterrissagem pesada)" "^BRISA_MARCO aterrissou .*pos=[-0-9.]+,0\.00,[-0-9.]+ no_chao=1 agachado=0 queda=([6-9]|1[0-9])\.[0-9]+ anim=aterrissar_pesado"
  confere "câmera girou 180°" "^BRISA_MARCO virou_camera .*camera_yaw=180\.0"
  confere "andou agachado" "^BRISA_MARCO agachado_andando .*agachado=1 .*anim=agachado_andar"
  confere "correu" "^BRISA_MARCO correndo .*anim=correr"
  confere "campo pelo VU1 (roteiro)" "BRISA_ROTEIRO caminho=VU1 .*lotes_vu1=[1-9]"
  confere "campo pelo EE (roteiro)" "BRISA_ROTEIRO caminho=EE .*desenhados_ee=[1-9]"
  # Ilha (Etapa 5)
  confere "ilha carregada (mapa, colisão, relevo)" "^BRISA_ILHA carregada_us=[0-9]+ instancias=1944 .*colisores=[0-9]+"
  confere "vista da Vila Brisa pelo VU1" "^BRISA_ILHA_VISTA vila_brisa_chegando caminho=VU1 .*tris=[1-9][0-9]+"
  confere "vista do Mirante (norte)" "^BRISA_ILHA_VISTA mirante_0 caminho=VU1 "
  # A captura encerra o programa no último quadro salvo; o fim só aparece no --completo.
  if [ "$modo" = "--completo" ]; then
    n=$(grep -c "^BRISA_ILHA_VISTA .*caminho=VU1 " "$r")
    if [ "$n" -eq 16 ]; then echo "[OK] 16 vistas da ilha medidas"; else echo "[ERRO] vistas da ilha: $n de 16"; falhas=$((falhas + 1)); fi
    confere "caminhada no relevo (no chão, sobre o relevo)" "^BRISA_MARCO ilha_caminhada .*no_chao=1 acima_do_relevo=0\.0[0-9] anim=correr"
    confere "roteiro da ilha até o fim" "^BRISA_ILHA_OK"
    confere "acenou" "^BRISA_MARCO acenou .*no_chao=1"
    confere "roteiro do treino até o fim" "^BRISA_TREINO_OK"
    confere "terminou sem travar" "^BRISA_FIM_OK"
  fi
fi
[ "$falhas" -eq 0 ] && echo "EMULADOR_OK" || { echo "EMULADOR_FALHOU ($falhas)"; exit 1; }
