// Caminho A (EE): o pipeline de referência transforma, ilumina, recorta e
// descarta faces no EE; aqui os triângulos viram pacotes GIF (PATH3).
#pragma once
#include "render3d/pipeline_sw.h"

class SaidaGif : public SaidaTriangulos {
public:
    void iniciar(f32 perto);
    void triangulo(const VerticeTela& a, const VerticeTela& b, const VerticeTela& c) override;
    void terminar();  // envia o que sobrou e espera o DMA
    // Pacote de estado para o 3D (TEST Z e cor da neblina), enviado antes dos dois caminhos.
    static void enviar_estado(const Cena3D& cena);
private:
    void abrir_lote();
    void fechar_e_enviar();
    void vertice(const VerticeTela& v);
    f32 perto_ = 0.5f;
    int buf_ = 0;
    u32 usado_ = 0;      // qw no buffer atual
    u32 tag_ = 0;        // posição do GIFtag do lote aberto
    u32 verts_ = 0;      // vértices no lote aberto
};
