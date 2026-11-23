#include "portador.h"

#include <math.h>
#include <string.h>

namespace {
const f32 GRAUS = 3.14159265f / 180.0f;
const f32 DURACAO_TROCA = 0.22f, DURACAO_GUARDAR = 0.12f, DURACAO_RECUO = 0.18f;
const f32 VEL_ANDAR = 4.5f;
// Poses por categoria (posição do grip_r relativa aos ombros, no espaço da mira).
const Vec3 POSE_QUADRIL[4] = {Vec3(), Vec3(-0.11f, -0.10f, 0.12f), Vec3(-0.11f, -0.10f, 0.08f), Vec3(-0.06f, -0.06f, 0.40f)};
const Vec3 POSE_MIRA[4] = {Vec3(), Vec3(-0.07f, 0.03f, 0.13f), Vec3(-0.07f, -0.04f, 0.10f), Vec3(-0.03f, 0.03f, 0.44f)};
const Vec3 OMBROS(0, 0.12f, 0), BOLSO(0.20f, -0.50f, 0.10f);
const Vec3 POLO[2] = {Vec3(-0.65f, -0.55f, -0.15f), Vec3(0.65f, -0.55f, -0.05f)};  // direito, esquerdo
const char* const OSSOS_BRACO[2][3] = {{"RightUpperArm", "RightLowerArm", "RightHand"}, {"LeftUpperArm", "LeftLowerArm", "LeftHand"}};

f32 mover(f32 v, f32 alvo, f32 passo) { return v < alvo ? (v + passo > alvo ? alvo : v + passo) : (v - passo < alvo ? alvo : v - passo); }
f32 suave(f32 t) {
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return t * t * (3 - 2 * t);
}
f32 envelope(f32 p, f32 entrada, f32 saida) { return suave(p / entrada) * suave((1 - p) / saida); }
Vec3 lerp(Vec3 a, Vec3 b, f32 t) { return a + (b - a) * t; }
f32 lerpf(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
Vec3 origem(const Mat4& m) { return Vec3(m.m[12], m.m[13], m.m[14]); }
Vec3 xf(const Mat4& m, Vec3 p) {
    Vec4 r = transformar(m, p);
    return Vec3(r.x, r.y, r.z);
}
// Rotação (Rodrigues) em torno de um eixo unitário, como Basis(eixo, ângulo) da Godot.
Mat4 rot(Vec3 e, f32 a) {
    f32 c = cosf(a), s = sinf(a), t = 1 - c;
    Mat4 r = Mat4::identidade();
    r(0, 0) = t * e.x * e.x + c;       r(0, 1) = t * e.x * e.y - s * e.z; r(0, 2) = t * e.x * e.z + s * e.y;
    r(1, 0) = t * e.x * e.y + s * e.z; r(1, 1) = t * e.y * e.y + c;       r(1, 2) = t * e.y * e.z - s * e.x;
    r(2, 0) = t * e.x * e.z - s * e.y; r(2, 1) = t * e.y * e.z + s * e.x; r(2, 2) = t * e.z * e.z + c;
    return r;
}
const Vec3 X(1, 0, 0), Y(0, 1, 0), FRENTE_GODOT(0, 0, -1);  // Vector3.RIGHT, UP, FORWARD
Mat4 rx(f32 g) { return rot(X, g * GRAUS); }
// Menor rotação que leva a direção a em b.
Mat4 arco(Vec3 a, Vec3 b) {
    a = normalizar(a);
    b = normalizar(b);
    Vec3 e = vetorial(a, b);
    f32 s = sqrtf(ponto(e, e)), c = ponto(a, b);
    if (s < 1e-6f) return Mat4::identidade();
    return rot(e * (1.0f / s), atan2f(s, c));
}
// Em torno de um ponto: T(o) · R · T(−o).
Mat4 em_torno(Vec3 o, const Mat4& r) { return Mat4::translacao(o) * r * Mat4::translacao(Vec3() - o); }
// Aplica D ao nó e aos descendentes (pais vêm antes dos filhos no BMOD).
void aplicar_sub(const ModeloBrisa& m, Pose& p, int raiz, const Mat4& d) {
    for (int k = 0; k < p.n; k++) {
        int a = k;
        while (a >= 0 && a != raiz) a = m.no(a).pai;
        if (a == raiz) p.mundo[k] = d * p.mundo[k];
    }
}
// Trilha de chaves com suavização entre elas (item_rig.gd _trilha).
struct Chave { f32 t; Vec3 v; };
Vec3 trilha(const Chave* k, int n, f32 t) {
    if (t <= k[0].t) return k[0].v;
    for (int i = 0; i + 1 < n; i++)
        if (t <= k[i + 1].t) return lerp(k[i].v, k[i + 1].v, suave((t - k[i].t) / fmaxf(1e-4f, k[i + 1].t - k[i].t)));
    return k[n - 1].v;
}
f32 trilha_f(const f32 (*k)[2], int n, f32 t) {
    if (t <= k[0][0]) return k[0][1];
    for (int i = 0; i + 1 < n; i++)
        if (t <= k[i + 1][0]) return lerpf(k[i][1], k[i + 1][1], suave((t - k[i][0]) / fmaxf(1e-4f, k[i + 1][0] - k[i][0])));
    return k[n - 1][1];
}
// Picareta: [p, posição do grip_r, ângulos (inclinação X, rolagem Z, giro Y) em graus].
struct ChavePic { f32 t; Vec3 pos, ang; };
const ChavePic PIC_REPOUSO = {0, Vec3(-0.30f, -0.36f, 0.17f), Vec3(28, 20, 0)};
const ChavePic PIC_CORRENDO = {0, Vec3(-0.27f, -0.44f, 0.08f), Vec3(48, 18, 0)};
const ChavePic PIC_COSTAS = {0, Vec3(-0.18f, 0.12f, -0.22f), Vec3(-110, 30, 0)};
const ChavePic PIC_GOLPE_A[5] = {{0.0f, Vec3(-0.30f, -0.36f, 0.17f), Vec3(28, 20, 0)}, {0.30f, Vec3(-0.30f, 0.04f, -0.02f), Vec3(-62, 26, 0)},
                                 {0.42f, Vec3(-0.22f, -0.10f, 0.42f), Vec3(95, 8, 0)}, {0.54f, Vec3(-0.16f, -0.32f, 0.34f), Vec3(128, -4, 0)},
                                 {1.0f, Vec3(-0.30f, -0.36f, 0.17f), Vec3(28, 20, 0)}};
const ChavePic PIC_GOLPE_B[5] = {{0.0f, Vec3(-0.30f, -0.36f, 0.17f), Vec3(28, 20, 0)}, {0.30f, Vec3(-0.36f, -0.10f, 0.06f), Vec3(80, 90, -80)},
                                 {0.42f, Vec3(-0.16f, -0.12f, 0.42f), Vec3(86, 90, 0)}, {0.54f, Vec3(0.08f, -0.20f, 0.32f), Vec3(86, 90, 50)},
                                 {1.0f, Vec3(-0.30f, -0.36f, 0.17f), Vec3(28, 20, 0)}};
Mat4 rot_pic(Vec3 a) { return rot(Y, a.z * GRAUS) * rot(FRENTE_GODOT, -a.y * GRAUS) * rot(X, a.x * GRAUS); }
}  // namespace

void Portador::pedir_item(int item, const ModeloBrisa* modelo, int pose, const char* id) {
    pedido_ = item;
    modelo_pedido_ = modelo;
    pose_pedida_ = pose;
    strncpy(id_pedido_, id ? id : "", sizeof id_pedido_ - 1);
    if (item == item_) { guardando_ = false; return; }
    if (item_ != 0 && !guardando_ && equip_ > 0.05f) { guardando_ = true; return; }  // guarda o anterior primeiro
    if (guardando_ && equip_ > 0) return;
    item_ = item;
    modelo_ = modelo;
    pose_ = pose;
    strncpy(id_, id_pedido_, sizeof id_);
    guardando_ = false;
    equip_ = 0;
}

void Portador::atualizar(f32 dt, f32 pitch, bool mirando, f32 vh, int acao, f32 prog, bool alcancar, Vec3 alcance) {
    f32 lim = 70 * GRAUS;
    pitch = pitch < -lim ? -lim : (pitch > lim ? lim : pitch);
    pitch_ += (pitch - pitch_) * 0.5f;
    ads_ = mover(ads_, mirando ? 1.0f : 0.0f, dt * 7.0f);
    sprint_ = mover(sprint_, vh > VEL_ANDAR + 0.6f && ads_ < 0.1f ? 1.0f : 0.0f, dt * 5.0f);
    if (guardando_) {
        equip_ = mover(equip_, 0, dt / DURACAO_GUARDAR);
        if (equip_ <= 0) {
            guardando_ = false;
            item_ = pedido_;
            modelo_ = modelo_pedido_;
            pose_ = pose_pedida_;
            strncpy(id_, id_pedido_, sizeof id_);
        }
    } else {
        equip_ = mover(equip_, 1, dt / DURACAO_TROCA);
    }
    recuo_ = mover(recuo_, 0, dt / DURACAO_RECUO * fmaxf(recuo_, 0.3f));
    cabeca_extra_ = mover(cabeca_extra_, 0, dt * 2.0f);
    acao_ = acao;
    progresso_ = prog < 0 ? 0 : (prog > 1 ? 1 : prog);
    alcance_w_ = mover(alcance_w_, alcancar ? 1.0f : 0.0f, dt * 4.0f);
    if (alcancar) alcance_ = alcance;
    bool consumivel = pose_ == 4;
    abaixar_ = mover(abaixar_, fmaxf(sprint_, consumivel ? 0 : alcance_w_), dt * 5.0f);
    // as mãos entram e saem do IK suavemente
    f32 alvo_d = item_ != 0 ? 1.0f : alcance_w_, alvo_e = (pose_ >= 1 && pose_ <= 3) ? 1.0f : (pose_ == 5 ? alcance_w_ : 0);
    if (consumivel) alvo_e = (acao_ == ACAO_USO && (strcmp(id_, "curativo") == 0 || strcmp(id_, "kit_cura") == 0)) ? envelope(progresso_, 0.15f, 0.1f) : 0;
    w_d_ = mover(w_d_, alvo_d, dt * 8.0f);
    w_e_ = mover(w_e_, alvo_e, dt * 8.0f);
}

void Portador::disparou(bool forte) {
    recuo_ = fminf(recuo_ + (forte ? 1.4f : 0.7f), 2.0f);
}

void Portador::achar_ossos(const ModeloBrisa& m) {
    esqueleto_ = &m;
    o_ = Ossos();
    for (int i = 0; i < m.n_nos(); i++) {
        const char* n = m.no(i).nome;
        if (!strcmp(n, "Spine")) o_.spine = i;
        else if (!strcmp(n, "Chest")) o_.chest = i;
        else if (!strcmp(n, "Neck")) o_.neck = i;
        else if (!strcmp(n, "Head")) o_.head = i;
        for (int l = 0; l < 2; l++) {
            if (!strcmp(n, OSSOS_BRACO[l][0])) o_.braco[l] = i;
            if (!strcmp(n, OSSOS_BRACO[l][1])) o_.antebraco[l] = i;
            if (!strcmp(n, OSSOS_BRACO[l][2])) o_.mao[l] = i;
        }
    }
}

Vec3 Portador::marcador(const char* nome) const {
    if (!modelo_) return Vec3();
    for (int i = 0; i < modelo_->n_marcadores(); i++)
        if (!strcmp(modelo_->marcador(i).nome, nome)) {
            static Pose repouso;
            pose_repouso(*modelo_, repouso);
            return posicao_marcador(*modelo_, repouso, i);
        }
    return Vec3();
}

// IK de dois ossos com polo (TwoBoneIK3D com extend_end_bone de 6,5 cm).
void Portador::ik(const ModeloBrisa& m, Pose& p, int l, Vec3 alvo, Vec3 polo, f32 peso) const {
    if (peso <= 0.001f || o_.braco[l] < 0 || o_.antebraco[l] < 0 || o_.mao[l] < 0) return;
    Vec3 s = origem(p.mundo[o_.braco[l]]), e = origem(p.mundo[o_.antebraco[l]]), w = origem(p.mundo[o_.mao[l]]);
    Vec3 se = e - s, ew = w - e;
    f32 a = sqrtf(ponto(se, se)), b0 = sqrtf(ponto(ew, ew)), b = b0 + 0.065f;
    if (a < 1e-4f || b0 < 1e-4f) return;
    Vec3 ponta = w + ew * (0.065f / b0);
    Vec3 t = lerp(ponta, alvo, peso);
    Vec3 st = t - s;
    f32 d = sqrtf(ponto(st, st));
    if (d < 1e-4f) return;
    Vec3 u = st * (1.0f / d);
    f32 dmin = fabsf(a - b) + 1e-3f, dmax = a + b - 1e-3f;
    d = d < dmin ? dmin : (d > dmax ? dmax : d);
    Vec3 pv = polo - s;
    Vec3 v = pv - u * ponto(pv, u);
    if (ponto(v, v) < 1e-8f) v = vetorial(u, Vec3(0, 0, 1));
    v = normalizar(v);
    f32 ca = (a * a + d * d - b * b) / (2 * a * d);
    ca = ca < -1 ? -1 : (ca > 1 ? 1 : ca);
    f32 sa = sqrtf(1 - ca * ca);
    Vec3 e2 = s + (u * ca + v * sa) * a;
    Mat4 d1 = em_torno(s, arco(se, e2 - s));
    aplicar_sub(m, p, o_.braco[l], d1);
    Vec3 w1 = origem(p.mundo[o_.mao[l]]);
    Mat4 d2 = em_torno(e2, arco(w1 - e2, t - e2));
    aplicar_sub(m, p, o_.antebraco[l], d2);
}

bool Portador::aplicar(const ModeloBrisa& m, Pose& p, Mat4* saida) {
    if (esqueleto_ != &m) achar_ossos(m);
    tem_item_m_ = false;
    if (o_.chest < 0) return false;
    // 1. Mira: coluna, peito, pescoço e cabeça (aim_pose_modifier.gd).
    const int ossos[4] = {o_.spine, o_.chest, o_.neck, o_.head};
    const f32 pesos[4] = {0.22f, 0.33f, 0.17f, 0.28f};
    for (int i = 0; i < 4; i++) {
        if (ossos[i] < 0) continue;
        f32 ang = pitch_ * pesos[i] + (i == 3 ? cabeca_extra_ : 0) + (i == 1 ? chute_ : 0);
        if (fabsf(ang) < 0.0005f) continue;
        aplicar_sub(m, p, ossos[i], em_torno(origem(p.mundo[ossos[i]]), rot(X, -ang)));  // +ang levanta a frente
    }
    // 2. Item na pose da categoria (item_rig.gd).
    Vec3 chest = origem(p.mundo[o_.chest]);
    Vec3 ombros = chest + OMBROS;
    Mat4 mira = rot(X, -pitch_);
    Vec3 alvo_d, alvo_e;
    bool tem_e = false;
    chute_ = 0;
    if (pose_ >= 1 && pose_ <= 3 && modelo_) {
        Vec3 off = lerp(POSE_QUADRIL[pose_], POSE_MIRA[pose_], ads_);
        off = lerp(off, Vec3(-0.10f, -0.24f, 0.20f), abaixar_);  // corrida/alcance: "pronto baixo"
        Mat4 r = rot(Y, 28 * GRAUS * abaixar_) * rx(42 * abaixar_);
        f32 e = suave(equip_);
        off = off + Vec3(0, -0.32f, -0.05f) * (1 - e);           // sacar: sobe de baixo
        r = r * rx(65 * (1 - e));
        off = off + Vec3(0, 0.015f, -0.06f) * recuo_;              // recuo: para trás, cano para cima
        r = r * rx(-9 * recuo_);
        chute_ = -0.05f * recuo_;
        f32 rw = acao_ == ACAO_RECARGA ? envelope(progresso_, 0.12f, 0.14f) : 0;
        off = off + Vec3(0.06f, -0.07f, -0.07f) * rw;              // recarga: inclina a arma
        r = r * rot(FRENTE_GODOT, -36 * GRAUS * rw) * rx(-18 * rw);
        Vec3 grip = ombros + xf(mira, off);
        Mat4 base = mira * r;
        Vec3 g_r = marcador("grip_r");
        item_m_ = Mat4::translacao(grip) * base * Mat4::translacao(Vec3() - g_r);
        alvo_d = grip;
        Vec3 gl = xf(item_m_, marcador("grip_l")), mag = xf(item_m_, marcador("mag")), bolso = chest + BOLSO;
        Vec3 mao = gl;
        if (acao_ == ACAO_RECARGA) {
            Vec3 baixo = xf(base, Vec3(0, -0.12f, 0));
            if (pose_ == 3) {
                Chave k[] = {{0, gl}, {0.18f, mag}, {0.32f, mag + baixo}, {0.45f, bolso}, {0.6f, bolso}, {0.76f, mag + baixo}, {0.86f, mag}, {1, gl}};
                mao = trilha(k, 8, progresso_);
            } else if (!strcmp(id_, "trovao")) {
                Vec3 porta = mag + xf(base, Vec3(0, -0.03f, 0));  // espingarda: 3 cartuchos e a bomba
                Chave k[] = {{0, gl}, {0.12f, bolso}, {0.22f, porta}, {0.36f, bolso}, {0.46f, porta}, {0.60f, bolso}, {0.70f, porta},
                             {0.84f, gl}, {0.91f, gl + xf(base, Vec3(0, 0, -0.12f))}, {1, gl}};
                mao = trilha(k, 10, progresso_);
            } else {
                Chave k[] = {{0, gl}, {0.14f, mag}, {0.28f, mag + baixo}, {0.42f, bolso}, {0.55f, bolso}, {0.68f, mag + baixo},
                             {0.78f, mag}, {0.86f, mag + xf(base, Vec3(0.06f, 0.06f, 0.02f))}, {1, gl}};
                mao = trilha(k, 9, progresso_);
            }
        }
        if (alcance_w_ > 0) mao = lerp(mao, alcance_, alcance_w_);
        alvo_e = mao;
        tem_e = true;
        tem_item_m_ = true;
    } else if (pose_ == 5 && modelo_) {
        // Picareta: repouso, corrida, golpes A/B e sacar por cima do ombro.
        Vec3 off = lerp(PIC_REPOUSO.pos, PIC_CORRENDO.pos, sprint_), ang = lerp(PIC_REPOUSO.ang, PIC_CORRENDO.ang, sprint_);
        if (acao_ == ACAO_GOLPE || acao_ == ACAO_GOLPE_B) {
            const ChavePic* g = acao_ == ACAO_GOLPE_B ? PIC_GOLPE_B : PIC_GOLPE_A;
            Chave kp[5], ka[5];
            for (int i = 0; i < 5; i++) { kp[i] = {g[i].t, g[i].pos}; ka[i] = {g[i].t, g[i].ang}; }
            off = trilha(kp, 5, progresso_);
            ang = trilha(ka, 5, progresso_);
            const f32 kc[5][2] = {{0, 0}, {0.30f, -0.10f}, {0.42f, 0.16f}, {0.6f, 0.06f}, {1, 0}};
            chute_ = trilha_f(kc, 5, progresso_);  // o tronco acompanha o golpe
        }
        f32 e = suave(equip_);
        off = lerp(PIC_COSTAS.pos, off, e);
        ang = lerp(PIC_COSTAS.ang, ang, e);
        Mat4 base = mira * rot_pic(ang);
        Vec3 grip = ombros + xf(mira, off), g_r = marcador("grip_r");
        item_m_ = Mat4::translacao(grip) * base * Mat4::translacao(Vec3() - g_r);
        alvo_d = grip;
        if (alcance_w_ > 0) { alvo_e = alcance_; tem_e = true; }
        tem_item_m_ = true;
    } else if (pose_ == 4 && modelo_) {
        // Consumível: segurando à frente do quadril; usando, a animação de cada um.
        bool usando = acao_ == ACAO_USO;
        f32 pr = usando ? progresso_ : 0, env = usando ? envelope(pr, 0.15f, 0.1f) : 0;
        Vec3 seg = chest + Vec3(-0.20f, -0.36f, 0.24f), pos = seg;
        Mat4 r_seg = rx(-20), r = r_seg;
        if (!strncmp(id_, "celula", 6)) {
            Vec3 cab = o_.head >= 0 ? origem(p.mundo[o_.head]) : chest;
            Vec3 boca = cab + Vec3(-0.02f, -0.06f, 0.17f);
            f32 gole = sinf(pr * 2 * 3.14159265f * 3) * 0.5f + 0.5f;
            pos = lerp(seg, boca, env);
            r = rx(lerpf(-20, -70 - 20 * gole, env));  // inclina o frasco
            cabeca_extra_ = (18 + 8 * gole) * GRAUS * env;
        } else if (!strcmp(id_, "curativo")) {
            Vec3 centro = chest + Vec3(0.06f, -0.12f, 0.30f);
            f32 a = pr * 2 * 3.14159265f * 5;
            pos = lerp(seg, centro + Vec3(cosf(a) * 0.07f, sinf(a) * 0.07f, -0.02f), env);
            r = r_seg * rot(Y, a * 0.3f * env);
            alvo_e = centro + Vec3(0.10f, -0.03f, 0.02f);
            tem_e = true;
        } else if (!strcmp(id_, "kit_cura")) {
            Vec3 frente = chest + Vec3(0, -0.18f, 0.32f);
            f32 toque = fabsf(sinf(pr * 2 * 3.14159265f * 6)) * 0.05f;
            pos = lerp(seg, frente, env);
            r = rx(lerpf(-20, 0, env));
            alvo_e = frente + Vec3(0.14f, 0.06f + toque, 0.02f);
            tem_e = true;
        }
        pos = pos + Vec3(0, -0.25f, 0) * (1 - suave(equip_));
        Vec3 mao_m = marcador("mao");
        item_m_ = Mat4::translacao(pos) * r * Mat4::translacao(Vec3() - mao_m);
        alvo_d = pos;
        tem_item_m_ = true;
    } else if (alcance_w_ > 0) {
        alvo_d = alcance_;  // sem item: a mão direita alcança o baú
    }
    // 3. IK das duas mãos.
    ik(m, p, 0, alvo_d, chest + POLO[0], w_d_);
    if (tem_e) ik(m, p, 1, alvo_e, chest + POLO[1], w_e_);
    if (tem_item_m_ && saida) *saida = item_m_;
    return tem_item_m_;
}

bool Portador::cano(Vec3* p) const {
    if (!tem_item_m_ || pose_ < 1 || pose_ > 3) return false;
    *p = xf(item_m_, marcador("muzzle"));
    return true;
}

bool Portador::ponta_picareta(Vec3* p) const {
    if (!tem_item_m_ || pose_ != 5) return false;
    *p = xf(item_m_, marcador("cabeca"));
    return true;
}
