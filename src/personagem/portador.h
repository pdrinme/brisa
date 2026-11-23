// Item na mão do personagem (item_rig.gd + aim_pose_modifier.gd + os TwoBoneIK3D
// do pawn_visual.gd do Ventania), sobre a pose rígida do Brisa. Depois da animação:
//  1. coluna, peito, pescoço e cabeça inclinam para a mira (pesos do Ventania);
//  2. o item vai para a pose da categoria (rifle, precisão, pistola, consumível,
//     picareta) com as ações procedurais (sacar, recuo, recarga, golpe, uso);
//  3. IK de dois ossos leva as mãos aos marcadores do item (grip_r, grip_l, mag, mao).
// Espaço do modelo = espaço do esqueleto do Ventania: frente +z, esquerda +x, cima +y.
#pragma once
#include "conteudo/animacao.h"
#include "conteudo/modelo.h"
#include "nucleo/mat.h"

enum AcaoItem { ACAO_NADA, ACAO_RECARGA, ACAO_USO, ACAO_GOLPE, ACAO_GOLPE_B, ACAO_BAU };

class Portador {
public:
    // pose: PoseItem (combate/itens.h) como inteiro; id: "trovao", "curativo"… (recargas e usos específicos)
    void pedir_item(int item, const ModeloBrisa* modelo, int pose, const char* id);
    int item_na_mao() const { return item_; }
    // Por quadro: pitch da câmera (rad), mirando, velocidade horizontal (m/s), ação e progresso,
    // e o ponto que a mão deve alcançar (alça do baú, espaço do modelo) com o peso pedido.
    void atualizar(f32 dt, f32 pitch, bool mirando, f32 vel_horizontal, int acao, f32 progresso, bool alcancar, Vec3 alcance);
    void disparou(bool forte);  // recuo (trovão e gavião: forte)
    // Aplica sobre a pose da animação. Devolve false se não há item na mão; senão, a
    // matriz do item no espaço do modelo.
    bool aplicar(const ModeloBrisa& personagem, Pose& pose, Mat4* item);
    // Cano (muzzle) e ponta da picareta no espaço do modelo, depois de aplicar().
    bool cano(Vec3* p) const;
    bool ponta_picareta(Vec3* p) const;
    f32 ads() const { return ads_; }
    f32 recuo() const { return recuo_; }
private:
    struct Ossos { int spine = -1, chest = -1, neck = -1, head = -1, braco[2] = {-1, -1}, antebraco[2] = {-1, -1}, mao[2] = {-1, -1}; };
    void achar_ossos(const ModeloBrisa& m);
    Vec3 marcador(const char* nome) const;
    void ik(const ModeloBrisa& m, Pose& p, int lado, Vec3 alvo, Vec3 polo, f32 peso) const;
    const ModeloBrisa* esqueleto_ = nullptr;
    Ossos o_;
    int item_ = 0, pedido_ = 0, pose_ = 0, pose_pedida_ = 0;
    const ModeloBrisa* modelo_ = nullptr;
    const ModeloBrisa* modelo_pedido_ = nullptr;
    char id_[12] = "", id_pedido_[12] = "";
    bool guardando_ = false;
    f32 pitch_ = 0, ads_ = 0, sprint_ = 0, recuo_ = 0, equip_ = 1, abaixar_ = 0, alcance_w_ = 0;
    f32 w_d_ = 0, w_e_ = 0, chute_ = 0, cabeca_extra_ = 0;
    int acao_ = ACAO_NADA;
    f32 progresso_ = 0;
    Vec3 alcance_;
    Mat4 item_m_;
    bool tem_item_m_ = false;
};
