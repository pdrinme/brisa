// Interface da plataforma. O núcleo e as telas só conhecem isto; a implementação
// fica em plataforma/ps2 (PS2SDK + gsKit) ou plataforma/host (PC, testes e prévia).
#pragma once
#include "nucleo/controle.h"
#include "nucleo/desenho2d.h"

struct InfoPlataforma {
    char nome[32] = "";          // "PS2" ou "HOST"
    char video[48] = "";         // ex.: "NTSC 640×448 ENTRELAÇADO"
    int largura = 640;
    int altura = 448;
    u32 hz_video = 60;           // campos por segundo
    u32 memoria_livre = 0;       // bytes (maior bloco alocável no boot)
    u32 tamanho_programa = 0;    // bytes do ELF carregado (texto + dados + bss)
    char caminho_boot[64] = "";  // argv[0]
    char modulos[64] = "";       // como os módulos do controle foram carregados
    char memoria_detalhe[128] = "";  // como a memória livre foi medida (log)
};

namespace plataforma {
bool iniciar(InfoPlataforma& info, int argc, char** argv);
void ler_controle(EstadoControle& estado);
u64 tempo_us();                                  // relógio monotônico
void desenhar_e_apresentar(const ListaDesenho2D& lista);  // envia e espera o vsync
void log(const char* texto);
void encerrar();
}
