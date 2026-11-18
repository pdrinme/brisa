# Brisa — build do ELF do PS2 e dos testes/prévia no PC.
#
#   make              ELF do PS2 (build/ps2/brisa.elf)
#   make autoteste    ELF que roda 600 quadros, imprime o resumo e sai (emulador sem interface)
#   make testes       testes do núcleo no PC
#   make previa       PNGs da tela no PC (build/previa/*.png)
#   make emulador     autoteste no Play! com vídeo e controle simulado (capturas em build/emulador)
#   make emulador-completo  autoteste no AutoTest do Play! até o fim (600 quadros)
#   make limpar
#
# Requer o toolchain PS2DEV (source tools/ambiente/env.sh) para os alvos do PS2.

PS2DEV ?= /opt/ps2dev
PS2SDK ?= $(PS2DEV)/ps2sdk
GSKIT ?= $(PS2DEV)/gsKit

NUCLEO := $(wildcard src/nucleo/*.cpp) $(wildcard src/jogo/*.cpp) $(wildcard src/render3d/*.cpp) $(wildcard src/conteudo/*.cpp) $(wildcard src/mundo/*.cpp) $(wildcard src/personagem/*.cpp)
PAK := build/conteudo/BRISA.PAK
ASSETS := assets/catalogo_brisa.json $(wildcard assets/*/*)
PS2_SRC := src/main.cpp $(NUCLEO) $(wildcard src/plataforma/ps2/*.cpp)
VU_SRC := $(wildcard src/plataforma/ps2/vu1/*.vsm)
DVP_AS := $(PS2DEV)/dvp/bin/dvp-as
HOST_SRC := $(NUCLEO) $(wildcard src/plataforma/host/*.cpp)
IRX := sio2man padman

# ---------------- PS2 ----------------
EE_CXX := $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-g++
EE_CC := $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-gcc
EE_STRIP := $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-strip
EE_SIZE := $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-size
BIN2C := $(PS2SDK)/bin/bin2c
EE_FLAGS := -D_EE -G0 -O2 -Wall -Wextra -Wno-unused-parameter -ffunction-sections -fdata-sections \
            -Isrc -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I$(GSKIT)/include
EE_CXXFLAGS := $(EE_FLAGS) -std=gnu++17 -fno-exceptions -fno-rtti -MMD -MP
EE_LDFLAGS := -T$(PS2SDK)/ee/startup/linkfile -L$(PS2SDK)/ee/lib -L$(GSKIT)/lib -Wl,-zmax-page-size=128 -Wl,--gc-sections
EE_LIBS := -lgskit -ldmakit -lpacket2 -lpad -lpatches -lc -lm

define objetos_ps2
$(patsubst src/%.cpp,build/$(1)/%.o,$(PS2_SRC)) $(patsubst src/%.vsm,build/$(1)/%.vu.o,$(VU_SRC)) build/$(1)/conteudo_embutido.o $(patsubst %,build/$(1)/irx/%_irx.o,$(IRX))
endef

all: build/ps2/brisa.elf

build/ps2/brisa.elf: $(call objetos_ps2,ps2)
	$(EE_CXX) $(EE_LDFLAGS) -o $@ $^ $(EE_LIBS)
	$(EE_SIZE) $@
	@cp $@ build/ps2/brisa_debug.elf && $(EE_STRIP) $@

# ---------------- Conteúdo ----------------
# make conteudo-ventania VENTANIA=../ventania  reconverte os assets a partir do Ventania (Etapa 3).
VENTANIA ?= ../ventania
conteudo: $(PAK)
$(PAK): $(ASSETS) tools/conversor/empacotar.py tools/conversor/formatos.py
	python3 tools/conversor/empacotar.py --saida $@
conteudo-ventania:
	python3 tools/conversor/construir.py --ventania $(VENTANIA)
	python3 tools/conversor/empacotar.py --saida $(PAK)

build/%/conteudo_embutido.o: src/plataforma/ps2/conteudo_embutido.S $(PAK)
	@mkdir -p $(@D)
	$(EE_CC) -D_EE -G0 '-DBRISA_PAK_CAMINHO="$(PAK)"' -c $< -o $@

# O microprograma depende do .vsm (antes não dependia e não era remontado).
build/ps2/%.vu.o: src/%.vsm
	@mkdir -p $(@D)
	$(DVP_AS) $< -o $@
build/autoteste/%.vu.o: src/%.vsm
	@mkdir -p $(@D)
	$(DVP_AS) $< -o $@

build/ps2/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(EE_CXX) $(EE_CXXFLAGS) -c $< -o $@

autoteste: build/autoteste/brisa_autoteste.elf

build/autoteste/brisa_autoteste.elf: $(call objetos_ps2,autoteste)
	$(EE_CXX) $(EE_LDFLAGS) -o $@ $^ $(EE_LIBS)

build/autoteste/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(EE_CXX) $(EE_CXXFLAGS) -DBRISA_AUTOTESTE=3000 -DBRISA_BENCH_AQUECER=3 -DBRISA_BENCH_MEDIR=6 -c $< -o $@

build/%/irx/sio2man_irx.c: $(PS2SDK)/iop/irx/sio2man.irx
	@mkdir -p $(@D)
	$(BIN2C) $< $@ sio2man_irx
build/%/irx/padman_irx.c: $(PS2SDK)/iop/irx/padman.irx
	@mkdir -p $(@D)
	$(BIN2C) $< $@ padman_irx
build/%_irx.o: build/%_irx.c
	$(EE_CC) -D_EE -G0 -c $< -o $@

# ---------------- PC (testes e prévia) ----------------
HOST_CXX ?= g++
HOST_FLAGS := -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter -Isrc -Itests -fno-exceptions -fno-rtti

build/host/testes: $(PAK) $(HOST_SRC) $(wildcard tests/*.cpp) $(wildcard tests/*.h) $(wildcard src/*/*.h) $(wildcard src/*/*/*.h)
	@mkdir -p $(@D)
	$(HOST_CXX) $(HOST_FLAGS) -o $@ $(filter %.cpp,$^) -lm

build/host/brisa_previa: $(PAK) src/main.cpp $(HOST_SRC) $(wildcard src/*/*.h) $(wildcard src/*/*/*.h)
	@mkdir -p $(@D)
	$(HOST_CXX) $(HOST_FLAGS) -o $@ $(filter %.cpp,$^) -lm

testes: build/host/testes
	./build/host/testes

previa: build/host/brisa_previa
	@mkdir -p build/previa
	./build/host/brisa_previa --roteiro=1 --quadros=60 --png=build/previa/cena_nivel4.png > /dev/null
	./build/host/brisa_previa --nivel=7 --quadros=60 --png=build/previa/cena_nivel7.png > /dev/null
	./build/host/brisa_previa --nivel=3 --carga=1 --quadros=60 --png=build/previa/carga_nivel3.png > /dev/null
	@ls -la build/previa

# ---------------- Emulador (Play!, sem BIOS) ----------------
# emulador: o ELF de autoteste no Play! com GS OpenGL (Mesa em software, Xvfb)
# e controle simulado; salva quadros em build/emulador/*.png e confere a saída.
# Ferramenta: tools/emulador/captura_play (ver docs/FERRAMENTAS_PS2DEV.md).
CAPTURA ?= /opt/brisa-captura/brisa_captura
AUTOTEST ?= /opt/play-autotest/tools/AutoTest/autotest
emulador: autoteste build/host/brisa_previa
	@rm -rf build/emulador
	@mkdir -p build/emulador
	xvfb-run -a -s "-screen 0 1024x768x24" env LIBGL_ALWAYS_SOFTWARE=1 \
	  $(CAPTURA) build/autoteste/brisa_autoteste.elf build/emulador/cena 200,680,1250
	xvfb-run -a -s "-screen 0 1024x768x24" env LIBGL_ALWAYS_SOFTWARE=1 \
	  $(CAPTURA) build/autoteste/brisa_autoteste.elf build/emulador/controle 150 botoes
	python3 tools/emulador/ppm_para_png.py --todos build/emulador
	@echo "--- saída do ELF no emulador (cena) ---"
	@cat build/emulador/cena.log
	tools/emulador/verificar_saida.sh build/emulador/cena.log --cena
	@# Imagem: o quadro 680 do emulador (parado no topo da torre, passo ~338, VU1)
	@# contra a prévia do PC no mesmo passo. Pega o "VU1 não desenhou" que a contagem de lotes não pega.
	./build/host/brisa_previa --roteiro=1 --png=build/emulador/pc_338.png --png-quadro=338 > /dev/null
	python3 tools/emulador/comparar_quadros.py build/emulador/cena_680.png build/emulador/pc_338.png
	tools/emulador/verificar_saida.sh build/emulador/controle.log --controle

# emulador-completo: os 600 quadros até o fim do programa (AutoTest: GS nulo, sem controle).
emulador-completo: autoteste
	@rm -rf build/emulador_completo
	@mkdir -p build/emulador_completo
	cp build/autoteste/brisa_autoteste.elf build/emulador_completo/
	@echo "(saída conferida por tools/emulador/verificar_saida.sh)" > build/emulador_completo/brisa_autoteste.expected
	-cd build/emulador_completo && timeout 1800 $(AUTOTEST) . > autotest.log 2>&1
	@cat build/emulador_completo/brisa_autoteste.result
	tools/emulador/verificar_saida.sh build/emulador_completo/brisa_autoteste.result --completo

limpar:
	rm -rf build

.PHONY: all autoteste testes previa emulador emulador-completo limpar conteudo conteudo-ventania

-include $(shell find build -name '*.d' 2>/dev/null)
