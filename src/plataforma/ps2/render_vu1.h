// Caminho B (VU1): o EE só monta uma cadeia de DMA para o VIF1 (referências às
// malhas, sem copiar vértices) e o microprograma vu1/toon.vsm faz o resto.
// Objetos que cruzam o plano perto vão para o caminho A (o VU1 não recorta).
#pragma once
#include "render3d/pipeline_sw.h"

namespace vu1 {
bool iniciar();  // envia o microprograma e configura o buffer duplo
// Desenha a cena. Objetos cortados pelo plano perto são entregues a `ee`.
void desenhar(const Cena3D& cena, SaidaTriangulos& ee, Estatisticas3D& est);
const int MAX_VERTS_LOTE = 96;  // 13 + 5·96 + 1 = 494 qw ≤ 512 (metade da memória do VU1)
}
