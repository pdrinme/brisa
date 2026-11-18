// Etapa 4: movimento, colisão, câmera, animador e o roteiro do campo de treino.
#include <math.h>
#include <string.h>

#include "conteudo/pak.h"
#include "jogo/roteiro_treino.h"
#include "jogo/treino.h"
#include "mundo/colisao.h"
#include "nucleo/arena.h"
#include "personagem/camera3p.h"
#include "personagem/movimento.h"
#include "plataforma/plataforma.h"
#include "teste.h"

namespace {
const f32 DT = 1.0f / 30.0f;

MundoColisao& mundo_plano() {
    static MundoColisao m;
    m.limpar(128.0f);
    m.definir_chao(true, 0.0f);
    return m;
}

Personagem em_pe(Vec3 pos) {
    Personagem p;
    p.pos = pos;
    p.no_chao = true;
    return p;
}

// Anda `passos` passos com o comando dado; devolve a velocidade horizontal do último.
f32 andar(Personagem& p, const ComandoMovimento& c, const MundoColisao& m, int passos) {
    for (int i = 0; i < passos; i++) passo_movimento(p, c, m, DT);
    return sqrtf(p.vel.x * p.vel.x + p.vel.z * p.vel.z);
}

ComandoMovimento frente() {
    ComandoMovimento c;
    c.y = 1.0f;
    return c;
}

const Pacote* pacote() {
    static Pacote p;
    static bool ok = false, tentou = false;
    if (!tentou) {
        tentou = true;
        u32 t = 0;
        const u8* d = plataforma::conteudo(&t);
        char e[128];
        ok = d && p.abrir(d, t, e, sizeof e);
    }
    return ok ? &p : nullptr;
}

// O campo de treino completo (o mesmo do jogo), criado uma vez.
Treino* treino_novo() {
    alignas(64) static u8 memoria[2 * 1024 * 1024];
    static Treino t;
    const Pacote* pak = pacote();
    if (!pak) return nullptr;
    Arena arena(memoria, sizeof memoria);
    char e[160];
    return t.iniciar(*pak, arena, e, sizeof e) ? &t : nullptr;
}
}  // namespace

// --- Movimento (números do Ventania: player_movement.gd / pawn_body.gd) ---

TESTE(movimento_velocidades_do_ventania) {
    MundoColisao& m = mundo_plano();
    Personagem p = em_pe(Vec3(0, 0, 0));
    ComandoMovimento c = frente();
    VERIFICA(fabsf(andar(p, c, m, 30) - mov::ANDAR) < 0.05f);
    VERIFICA(p.pos.z < -3.0f && fabsf(p.pos.x) < 1e-3f);  // yaw 0: frente é −z
    c.correr = true;
    VERIFICA(fabsf(andar(p, c, m, 30) - mov::CORRER) < 0.05f);
    VERIFICA(p.correndo);
    c.correr = false;
    c.agachar = true;
    VERIFICA(fabsf(andar(p, c, m, 30) - mov::AGACHADO) < 0.05f);
    VERIFICA(p.agachado && fabsf(p.altura() - mov::ALTURA_AGACHADO) < 1e-4f);
    // Correr para trás não vale (só com o analógico para a frente).
    c.agachar = false;
    c.correr = true;
    c.y = -1.0f;
    andar(p, c, m, 30);
    VERIFICA(!p.correndo);
    // Aceleração no chão: de parado a 4,5 m/s em 0,1 s (45 m/s²).
    Personagem q = em_pe(Vec3(0, 0, 0));
    passo_movimento(q, frente(), m, DT);
    VERIFICA(fabsf(sqrtf(q.vel.x * q.vel.x + q.vel.z * q.vel.z) - 45.0f * DT) < 0.02f);
}

TESTE(movimento_direcao_segue_o_yaw_da_camera) {
    MundoColisao& m = mundo_plano();
    Personagem p = em_pe(Vec3(0, 0, 0));
    ComandoMovimento c = frente();
    c.yaw = 3.14159265f / 2;  // câmera virada 90° para a esquerda: frente vira −x
    andar(p, c, m, 30);
    VERIFICA(p.pos.x < -3.0f && fabsf(p.pos.z) < 0.01f);
    Personagem q = em_pe(Vec3(0, 0, 0));
    ComandoMovimento d;
    d.x = 1.0f;  // direita com yaw 0 = +x
    andar(q, d, m, 30);
    VERIFICA(q.pos.x > 3.0f && fabsf(q.pos.z) < 0.01f);
    // Diagonal não passa da velocidade máxima.
    Personagem r = em_pe(Vec3(0, 0, 0));
    ComandoMovimento e;
    e.x = 1.0f;
    e.y = 1.0f;
    VERIFICA(andar(r, e, m, 30) <= mov::ANDAR + 0.01f);
}

TESTE(salto_e_queda) {
    MundoColisao& m = mundo_plano();
    Personagem p = em_pe(Vec3(0, 0, 0));
    ComandoMovimento c;
    c.pular = true;
    passo_movimento(p, c, m, DT);
    VERIFICA(p.pulou && !p.no_chao);
    c.pular = false;
    f32 pico = 0;
    int n = 0;
    while (!p.no_chao && n < 120) { passo_movimento(p, c, m, DT); pico = fmaxf(pico, p.pos.y); n++; }
    // v²/2g = 7,4²/44 = 1,24 m (com o passo de 30 Hz fica um pouco acima).
    VERIFICA(pico > 1.2f && pico < 1.45f);
    VERIFICA(p.no_chao && p.aterrissou && fabsf(p.pos.y) < 1e-4f);
    VERIFICA(n > 15 && n < 25);  // ~0,67 s no ar
    // Agachado não pula.
    Personagem q = em_pe(Vec3(0, 0, 0));
    ComandoMovimento d;
    d.agachar = true;
    passo_movimento(q, d, m, DT);
    d.pular = true;
    passo_movimento(q, d, m, DT);
    VERIFICA(!q.pulou && q.no_chao);
    // Queda de 10 m: altura registrada e velocidade presa em QUEDA_MAX.
    Personagem r;
    r.pos = Vec3(0, 10, 0);
    r.pico = 10;
    n = 0;
    while (!r.no_chao && n < 200) { passo_movimento(r, ComandoMovimento(), m, DT); VERIFICA(r.vel.y >= -mov::QUEDA_MAX); n++; }
    VERIFICA(r.aterrissou && fabsf(r.altura_queda - 10.0f) < 0.01f);
}

TESTE(colisao_parede_degrau_e_obstaculo) {
    MundoColisao& m = mundo_plano();
    // Parede de 3 m com a face em z = −2.
    m.adicionar_caixa(Vec3(0, 0, -3), 0, Vec3(-5, 0, -1), Vec3(5, 3, 1));
    Personagem p = em_pe(Vec3(0, 0, 0));
    andar(p, frente(), m, 60);
    VERIFICA(fabsf(p.pos.z - (-2.0f + mov::RAIO)) < 0.02f);
    VERIFICA(fabsf(p.pos.y) < 1e-4f && p.no_chao);
    VERIFICA(sqrtf(p.vel.x * p.vel.x + p.vel.z * p.vel.z) < 0.1f);  // a animação vê parado

    // Degrau de 0,3 m (≤ 0,35): sobe. Obstáculo de 0,5 m: bloqueia.
    MundoColisao& d = mundo_plano();
    d.adicionar_caixa(Vec3(0, 0, -3), 0, Vec3(-5, 0, -1), Vec3(5, 0.3f, 1));
    d.adicionar_caixa(Vec3(10, 0, -3), 0, Vec3(-5, 0, -1), Vec3(5, 0.5f, 1));
    Personagem a = em_pe(Vec3(0, 0, 0)), b = em_pe(Vec3(10, 0, 0));
    andar(a, frente(), d, 30);
    andar(b, frente(), d, 30);
    VERIFICA(fabsf(a.pos.y - 0.3f) < 1e-3f && a.pos.z < -2.5f);
    VERIFICA(fabsf(b.pos.y) < 1e-3f && b.pos.z > -2.0f);

    // Parede girada 45°: escorrega ao longo dela, sem atravessar.
    MundoColisao& g = mundo_plano();
    g.adicionar_caixa(Vec3(0, 0, -3), 45, Vec3(-5, 0, -0.5f), Vec3(5, 3, 0.5f));
    Personagem s = em_pe(Vec3(0, 0, 0));
    andar(s, frente(), g, 60);
    f32 lx, lz;
    g.colisor(0).local(s.pos.x, s.pos.z, &lx, &lz);
    VERIFICA(lz > 0.5f + mov::RAIO - 0.02f);  // fora da parede
    VERIFICA(fabsf(s.pos.x) > 1.0f);          // deslizou de lado
}

TESTE(teto_baixo_obriga_a_agachar) {
    MundoColisao& m = mundo_plano();
    // Laje com a base a 1,4 m entre z = −2 e z = −6 (como debaixo dos pisos a 1,5 m do campo).
    m.adicionar_caixa(Vec3(0, 0, -4), 0, Vec3(-3, 1.4f, -2), Vec3(3, 1.6f, 2));
    Personagem p = em_pe(Vec3(0, 0, 0));
    andar(p, frente(), m, 30);  // em pé: a laje é parede
    VERIFICA(p.pos.z > -2.0f + mov::RAIO - 0.03f);
    ComandoMovimento c = frente();
    c.agachar = true;
    andar(p, c, m, 20);  // agachado (1,15 m): passa
    VERIFICA(p.pos.z < -3.0f && p.agachado);
    c.agachar = false;
    c.y = 0;
    passo_movimento(p, c, m, DT);
    VERIFICA(p.agachado);  // sem espaço para levantar
    VERIFICA(!m.livre(p.pos, mov::ALTURA, mov::RAIO));
    c.y = 1.0f;
    andar(p, c, m, 40);  // sai pelo outro lado e levanta sozinho
    VERIFICA(p.pos.z < -6.5f && !p.agachado);
    // Pulo embaixo de um teto: a cabeça para no teto.
    MundoColisao& t = mundo_plano();
    t.adicionar_caixa(Vec3(0, 0, 0), 0, Vec3(-3, 2.3f, -3), Vec3(3, 2.6f, 3));
    Personagem q = em_pe(Vec3(0, 0, 0));
    ComandoMovimento pulo;
    pulo.pular = true;
    f32 pico = 0;
    for (int i = 0; i < 30; i++) { passo_movimento(q, pulo, t, DT); pulo.pular = false; pico = fmaxf(pico, q.pos.y); }
    VERIFICA(pico <= 2.3f - mov::ALTURA + 1e-3f && pico > 0.4f);
    VERIFICA(q.no_chao);
}

TESTE(raio_contra_caixa_girada_e_chao) {
    MundoColisao& m = mundo_plano();
    m.adicionar_caixa(Vec3(0, 0, -5), 30, Vec3(-1, 0, -1), Vec3(1, 2, 1));
    ImpactoRaio r = m.raio(Vec3(0, 1, 0), Vec3(0, 1, -10));
    VERIFICA(r.colisor == 0 && r.t > 0.35f && r.t < 0.45f);
    ImpactoRaio s = m.raio(Vec3(0, 1, 0), Vec3(0, 1, 10));
    VERIFICA(s.colisor < 0 && s.t >= 1.0f);
    ImpactoRaio c = m.raio(Vec3(10, 2, 0), Vec3(10, -2, 0));  // atravessa o chão em y = 0
    VERIFICA(fabsf(c.t - 0.5f) < 0.01f);
}

// --- Câmera (player_camera.gd) ---

TESTE(camera_ombro_braco_de_mola_e_limites) {
    MundoColisao& m = mundo_plano();
    Camera3P cam;
    cam.pitch = 0;
    for (int i = 0; i < 30; i++) cam.atualizar(Vec3(0, 0, 0), false, m, DT);
    VERIFICA(fabsf(cam.distancia_atual() - Camera3P::DIST) < 1e-3f);
    Vec3 c = cam.posicao();
    VERIFICA(fabsf(c.x - Camera3P::OMBRO) < 1e-3f && fabsf(c.y - Camera3P::ALTURA_PE) < 1e-3f && fabsf(c.z - Camera3P::DIST) < 1e-3f);
    // Parede 1 m atrás do personagem: o braço encolhe na hora.
    m.adicionar_caixa(Vec3(0, 0, 1.5f), 0, Vec3(-5, 0, -0.5f), Vec3(5, 4, 0.5f));
    cam.atualizar(Vec3(0, 0, 0), false, m, DT);
    VERIFICA(cam.distancia_atual() < 1.0f && cam.posicao().z < 1.0f);
    // Sem a parede, volta a esticar devagar (não salta).
    MundoColisao& livre = mundo_plano();
    cam.atualizar(Vec3(0, 0, 0), false, livre, DT);
    VERIFICA(cam.distancia_atual() < 2.0f);
    for (int i = 0; i < 60; i++) cam.atualizar(Vec3(0, 0, 0), false, livre, DT);
    VERIFICA(fabsf(cam.distancia_atual() - Camera3P::DIST) < 0.01f);
    // Mira: braço mais curto. Agachado: pivô mais baixo.
    cam.mirando = true;
    for (int i = 0; i < 60; i++) cam.atualizar(Vec3(0, 0, 0), true, livre, DT);
    VERIFICA(fabsf(cam.distancia_atual() - Camera3P::DIST_MIRA) < 0.01f);
    VERIFICA(fabsf(cam.posicao().y - Camera3P::ALTURA_AGACHADO) < 0.01f);
    // Analógico: 135°/s; pitch preso em −80..70.
    Camera3P g;
    g.olhar(1.0f, 0.0f, 1.0f);
    VERIFICA(fabsf(g.yaw - 225.0f) < 0.5f);  // 0 − 135°, guardado em 0..360
    for (int i = 0; i < 10; i++) g.olhar(0.0f, 1.0f, 1.0f);
    VERIFICA(g.pitch >= Camera3P::PITCH_MIN - 1e-3f && g.pitch <= Camera3P::PITCH_MAX + 1e-3f);
    VERIFICA(g.pitch == Camera3P::PITCH_MIN || g.pitch == Camera3P::PITCH_MAX);
}

// --- Campo de treino com os assets do Ventania ---

TESTE(treino_carrega_o_campo_do_ventania) {
    Treino* t = treino_novo();
    VERIFICA(t != nullptr);
    if (!t) return;
    VERIFICA(t->mundo().descartados() == 0);
    VERIFICA(t->mundo().n() > 60);
    // Rampa de pedra do Ventania: colisão de rampa, não caixa.
    int rampas = 0;
    for (int i = 0; i < t->mundo().n(); i++) rampas += t->mundo().eh_rampa(i);
    VERIFICA(rampas == 3);  // 2 de pedra + 1 de metal
    // Chão nas alturas certas: piso da torre a 4 e a 8 m (a laje tem 0,1 m).
    f32 y = 0;
    VERIFICA(t->mundo().chao(0, -18, 0.2f, 10, &y) && fabsf(y - 4.1f) < 0.02f);
    VERIFICA(t->mundo().chao(0, -26, 0.2f, 10, &y) && fabsf(y - 8.1f) < 0.02f);
    // Meio da rampa: metade do caminho entre o chão e o piso.
    VERIFICA(t->mundo().chao(0, -14, 0.05f, 10, &y) && y > 1.5f && y < 2.6f);
}

TESTE(treino_agachado_passa_debaixo_do_piso_elevado) {
    Treino* t = treino_novo();
    if (!t) return;
    const MundoColisao& m = t->mundo();
    // Os pisos de madeira a 1,5 m (x = −16, z = 4 e 8): em pé não passa, agachado passa.
    Personagem p = em_pe(Vec3(-16, 0, 12));
    andar(p, frente(), m, 30);
    VERIFICA(p.pos.z > 9.0f);
    ComandoMovimento c = frente();
    c.agachar = true;
    andar(p, c, m, 60);
    VERIFICA(p.pos.z < 4.0f && p.agachado && fabsf(p.pos.y) < 1e-3f);
}

TESTE(animador_estados) {
    Treino* t = treino_novo();
    if (!t) return;
    u32 tam = 0;
    const u8* d = pacote()->arquivo("animacoes/humanoide.bani", &tam);
    static BancoAnimacoes banco;
    char e[96];
    VERIFICA(d && banco.carregar(d, tam, e, sizeof e));
    Animador a;
    VERIFICA(a.iniciar(banco));
    Personagem p = em_pe(Vec3(0, 0, 0));
    for (int i = 0; i < 10; i++) a.atualizar(p, false, DT);
    VERIFICA(strcmp(a.estado(), "idle") == 0);
    p.vel = Vec3(0, 0, -4.5f);
    for (int i = 0; i < 10; i++) a.atualizar(p, false, DT);
    VERIFICA(strcmp(a.estado(), "andar") == 0);
    p.vel = Vec3(0, 0, -6.5f);
    for (int i = 0; i < 10; i++) a.atualizar(p, false, DT);
    VERIFICA(strcmp(a.estado(), "correr") == 0);
    p.agachado = true;
    p.vel = Vec3(0, 0, -2.5f);
    for (int i = 0; i < 10; i++) a.atualizar(p, false, DT);
    VERIFICA(strcmp(a.estado(), "agachado_andar") == 0);
    p.agachado = false;
    p.no_chao = false;
    p.pulou = true;
    a.atualizar(p, false, DT);
    p.pulou = false;
    VERIFICA(strcmp(a.estado(), "pular") == 0);
    for (int i = 0; i < 40; i++) a.atualizar(p, false, DT);
    VERIFICA(strcmp(a.estado(), "queda") == 0 && a.peso_ar() > 0.9f);
    p.no_chao = true;
    p.aterrissou = true;
    p.altura_queda = 8.0f;
    p.vel = Vec3();
    a.atualizar(p, false, DT);
    p.aterrissou = false;
    VERIFICA(strcmp(a.estado(), "aterrissar_pesado") == 0);
    for (int i = 0; i < 90; i++) a.atualizar(p, false, DT);
    a.atualizar(p, true, DT);
    VERIFICA(strcmp(a.estado(), "acenar") == 0);
    // A pose sai inteira (matrizes finitas) para um traje do jogo.
    static Pose pose;
    ModeloBrisa m;
    const u8* dm = pacote()->arquivo("modelos/recruta_base_01.bmod", &tam);
    VERIFICA(dm && m.carregar(dm, tam, e, sizeof e));
    a.pose(m, pose);
    VERIFICA(pose.n == m.n_nos());
    bool finito = true;
    for (int i = 0; i < pose.n; i++)
        for (int k = 0; k < 16; k++) finito = finito && isfinite(pose.mundo[i].m[k]);
    VERIFICA(finito);
}

// O mesmo roteiro do autoteste do PS2, com as mesmas expectativas de
// tools/emulador/verificar_saida.sh.
TESTE(treino_roteiro_completo) {
    Treino* t = treino_novo();
    if (!t) return;
    Controle ctl;
    struct Marca { Vec3 pos; bool chao, agachado; f32 queda; char anim[24]; f32 yaw; u32 passo; } marcas[N_ROTEIRO_TREINO];
    f32 pico_pulo = 0, y_pulo = -1;
    for (int f = 0; f < N_ROTEIRO_TREINO; f++) {
        const FaixaRoteiro& fx = ROTEIRO_TREINO[f];
        for (u32 k = fx.inicio; k < fx.fim; k++) {
            EstadoControle e;
            e.modo = ModoControle::ANALOGICO;
            e.botoes = fx.botoes;
            e.lx = fx.lx; e.ly = fx.ly; e.rx = fx.rx; e.ry = fx.ry;
            ctl.ler(e);
            ctl.iniciar_passo();
            t->passo(ctl);
            if (fx.botoes == BT_XIS) y_pulo = t->personagem().pos.y;
            if (strcmp(fx.marco ? fx.marco : "", "pulo") == 0) pico_pulo = fmaxf(pico_pulo, t->personagem().pos.y);
        }
        const Personagem& p = t->personagem();
        marcas[f] = {p.pos, p.no_chao, p.agachado, p.altura_queda, "", t->camera().yaw, t->passos()};
        strncpy(marcas[f].anim, t->animador().estado(), sizeof marcas[f].anim - 1);
        marcas[f].anim[sizeof marcas[f].anim - 1] = 0;
    }
    auto marca = [&](const char* nome) -> const Marca* {
        for (int f = 0; f < N_ROTEIRO_TREINO; f++)
            if (ROTEIRO_TREINO[f].marco && strcmp(ROTEIRO_TREINO[f].marco, nome) == 0) return &marcas[f];
        return nullptr;
    };
    const Marca* topo = marca("topo_da_torre");
    VERIFICA(topo && topo->pos.y > 8.0f && topo->pos.y < 8.2f && topo->chao);
    VERIFICA(topo && topo->pos.z < -24.0f && fabsf(topo->pos.x) < 0.5f);
    const Marca* aterr = marca("aterrissou");
    VERIFICA(aterr && fabsf(aterr->pos.y) < 0.01f && aterr->queda > 6.0f && aterr->chao);
    VERIFICA(y_pulo >= 0 && pico_pulo - y_pulo > 1.2f && pico_pulo - y_pulo < 1.45f);
    const Marca* virou = marca("virou_camera");
    const Marca* pulo = marca("pulo");
    VERIFICA(virou && pulo && fabsf(fabsf(virou->yaw - pulo->yaw) - 180.0f) < 1.0f);
    const Marca* ag = marca("agachado_andando");
    VERIFICA(ag && ag->agachado && strcmp(ag->anim, "agachado_andar") == 0);
    const Marca* cor = marca("correndo");
    VERIFICA(cor && !cor->agachado && strcmp(cor->anim, "correr") == 0);
    const Marca* acenou = marca("acenou");
    VERIFICA(acenou && acenou->chao);
    VERIFICA(t->passos() == ROTEIRO_TREINO[N_ROTEIRO_TREINO - 1].fim);
}
