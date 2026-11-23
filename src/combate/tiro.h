// Matemática do tiro e do golpe (combat.gd e hitbox.gd do Ventania): raio de
// mira pelo ombro, dispersão por estado, projéteis determinísticos, hitboxes
// analíticas (cápsula do corpo + esfera da cabeça) e tempos da picareta.
#pragma once
#include "itens.h"
#include "nucleo/mat.h"

namespace tiro {
const f32 OLHO_PE = 1.55f, OLHO_AGACHADO = 1.05f, OMBRO = 0.55f;  // = Camera3P
const f32 RAIO_CORPO = 0.4f, RAIO_CABECA = 0.25f;
const int MAX_PROJETEIS = 16;

struct Raio { Vec3 origem, dir; };
// yaw e pitch em radianos (yaw 0 = −z; pitch + = para cima).
Raio mira(Vec3 pes, bool agachado, f32 yaw, f32 pitch);
f32 dispersao(const BancoItens& b, int arma, bool mirando, bool movendo, bool agachado, bool no_chao);
// Direções dos projéteis, uniformes no disco de `graus` (mesma semente → mesmas direções).
void projeteis(Vec3 dir, f32 graus, int n, u32 semente, Vec3* saida);
inline u32 semente_tiro(u32 entidade, u32 seq) { return entidade * 1000003u + seq * 7919u; }
int dano_projetil(const BancoItens& b, int arma, int raridade, bool cabeca);

// Hitbox de um alvo em pé ou agachado (pés no chão).
struct Forma { Vec3 a, b, cabeca; };
Forma forma(Vec3 pes, bool agachado);
f32 raio_esfera(Vec3 o, Vec3 d, Vec3 c, f32 r);                // distância ou −1
f32 raio_capsula(Vec3 o, Vec3 d, Vec3 a, Vec3 b, f32 r);
struct Acerto { f32 t = -1; bool cabeca = false; bool acertou() const { return t >= 0; } };
Acerto testar(Vec3 o, Vec3 d, Vec3 pes, bool agachado);

// Picareta (a 30 Hz): duração do golpe e passo do impacto.
int passos_golpe(const BancoItens& b);
int passo_impacto(const BancoItens& b);
f32 alcance_golpe(const BancoItens& b);
}  // namespace tiro
