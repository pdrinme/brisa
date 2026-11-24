#include "camera3p.h"

#include <math.h>

namespace {
f32 lerp(f32 a, f32 b, f32 t) { return a + (b - a) * (t > 1 ? 1 : t); }
const f32 GRAUS = 3.14159265f / 180.0f;
}  // namespace

void Camera3P::olhar(f32 ex, f32 ey, f32 dt) {
    // Ventania: add_look(eixo · 900 · dt · sensibilidade), 0,15° por unidade → 135°/s.
    f32 fator = mirando ? 0.5f : 1.0f;
    f32 inv = inverter_y ? -1.0f : 1.0f;
    yaw = fmodf(yaw - ex * 900.0f * dt * sensibilidade * 0.15f * fator + 360.0f, 360.0f);
    pitch -= ey * 900.0f * dt * sensibilidade * 0.15f * fator * inv;
    pitch = pitch < PITCH_MIN ? PITCH_MIN : (pitch > PITCH_MAX ? PITCH_MAX : pitch);
}

void Camera3P::atualizar(Vec3 pes, bool agachado, const MundoColisao& m, f32 dt) {
    altura_ = lerp(altura_, agachado ? ALTURA_AGACHADO : ALTURA_PE, dt * 12.0f);
    fov_ = lerp(fov_, mirando ? FOV_MIRA : FOV, dt * 10.0f);
    f32 y = yaw * GRAUS, p = pitch * GRAUS;
    // Base da rotação (yaw, pitch): direita, cima e "para trás" (+z local).
    Vec3 frente(-sinf(y) * cosf(p), sinf(p), -cosf(y) * cosf(p));
    Vec3 direita(cosf(y), 0, -sinf(y));
    Vec3 pivo = pes + Vec3(0, altura_, 0);
    Vec3 ombro = pivo + direita * OMBRO;
    f32 desejado = (mirando ? DIST_MIRA : DIST) + distancia_extra;
    // Braço de mola: raio do ombro até a câmera; encolhe na hora, estica devagar.
    Vec3 fim = ombro - frente * desejado;
    ImpactoRaio r = m.raio(ombro, fim);
    // A margem só vale quando o raio bate em algo (como o SpringArm3D da Godot).
    f32 livre = r.t < 1.0f ? r.t * desejado - MARGEM : desejado;
    if (livre < 0.2f) livre = 0.2f;
    f32 alvo = desejado < livre ? desejado : livre;
    braco_ = alvo < braco_ ? alvo : lerp(braco_, alvo, dt * 10.0f);
    pos_ = ombro - frente * braco_;
    alvo_ = pos_ + frente * 10.0f;
}

void Camera3P::aplicar(Camera3D& cam, f32 aspecto) const {
    cam.configurar(pos_, alvo_, fov_ * GRAUS, aspecto, 0.1f, 300.0f);
}
