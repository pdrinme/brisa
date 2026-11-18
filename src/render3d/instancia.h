// Coloca um modelo do pacote (com pose) na cena 3D: uma entrada por parte rígida.
#pragma once
#include "cena3d.h"
#include "conteudo/modelo.h"

// pose == nullptr: pose de repouso (usa as matrizes de repouso já compostas).
// Devolve quantas partes entraram.
int adicionar_modelo(Cena3D& cena, const ModeloBrisa& m, const Pose* pose, const Mat4& instancia);
