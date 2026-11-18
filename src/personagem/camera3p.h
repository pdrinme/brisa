// Câmera em terceira pessoa por cima do ombro (gameplay/character/player_camera.gd
// do Ventania): altura dos olhos 1,55 m (agachado 1,05), ombro 0,55 m à direita,
// distância 3,2 m (mirando 1,7), FOV 70° (mirando 55°), pitch de −80° a +70°,
// analógico a 135°/s com sensibilidade 1, e um "braço de mola" que encolhe
// quando há parede entre o personagem e a câmera.
#pragma once
#include "mundo/colisao.h"
#include "render3d/cena3d.h"

class Camera3P {
public:
    static constexpr f32 ALTURA_PE = 1.55f, ALTURA_AGACHADO = 1.05f, OMBRO = 0.55f, DIST = 3.2f, DIST_MIRA = 1.7f;
    static constexpr f32 FOV = 70.0f, FOV_MIRA = 55.0f, PITCH_MIN = -80.0f, PITCH_MAX = 70.0f;
    static constexpr f32 MARGEM = 0.2f;
    f32 yaw = 0;      // graus (0 = olhando para −z)
    f32 pitch = -10;  // graus
    f32 sensibilidade = 1.0f;
    bool inverter_y = false;
    bool mirando = false;
    // Analógico direito (−1..1, já com zona morta) aplicado por dt segundos.
    void olhar(f32 eixo_x, f32 eixo_y, f32 dt);
    // Segue o personagem (pés) e ajusta o braço contra o mundo.
    void atualizar(Vec3 pes, bool agachado, const MundoColisao& mundo, f32 dt);
    void aplicar(Camera3D& cam, f32 aspecto) const;
    f32 yaw_rad() const { return yaw * 3.14159265f / 180.0f; }
    Vec3 posicao() const { return pos_; }
    Vec3 alvo() const { return alvo_; }
    f32 distancia_atual() const { return braco_; }
private:
    f32 altura_ = ALTURA_PE, braco_ = DIST, fov_ = FOV;
    Vec3 pos_, alvo_;
};
