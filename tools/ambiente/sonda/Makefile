# Sonda do toolchain (Etapa 0): só confere que C++17 sem exceções liga com
# libpad, libmc e audsrv e gera um ELF do EE. NÃO é código do jogo.
EE_BIN = sonda.elf
EE_OBJS = sonda.o
EE_CXXFLAGS += -std=gnu++17 -fno-exceptions -fno-rtti
EE_LIBS = -lpad -lmc -laudsrv -lpatches
all: $(EE_BIN)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal_cpp
