#include "movimento.h"

#include <math.h>

namespace {
// Move no plano até `alvo` em subpassos de no máximo 0,2 m, empurrando para fora
// das paredes e subindo degraus/rampas. No chão, mantém a velocidade ao longo
// do piso nas rampas (floor_constant_speed do Ventania).
void mover_horizontal(Personagem& p, f32 dx, f32 dz, const MundoColisao& m) {
    f32 dist = sqrtf(dx * dx + dz * dz);
    int n = static_cast<int>(dist / 0.2f) + 1;
    f32 sx = dx / n, sz = dz / n;
    f32 ux = dist > 1e-6f ? dx / dist : 0, uz = dist > 1e-6f ? dz / dist : 0;
    for (int i = 0; i < n; i++) {
        Vec3 antes = p.pos;
        p.pos.x += sx;
        p.pos.z += sz;
        // Numa rampa, o chão na borda da frente do cilindro já está mais alto que os pés:
        // o degrau conta a partir dele (senão o piso no alto da rampa vira parede).
        // Só vale sobre rampa: no plano, o degrau continua de 0,35 m.
        f32 passo = mov::DEGRAU;
        f32 yf, yc;
        int sob = -1;
        if (p.no_chao && m.chao(p.pos.x, p.pos.z, 0.05f, p.pos.y + mov::DEGRAU, &yc, &sob) && m.eh_rampa(sob) &&
            m.chao(p.pos.x + ux * mov::RAIO, p.pos.z + uz * mov::RAIO, 0.05f, p.pos.y + mov::DEGRAU + mov::RAIO, &yf) &&
            yf > p.pos.y)
            passo += fminf(yf - p.pos.y, mov::RAIO);
        m.empurrar(p.pos, p.altura(), mov::RAIO, passo);
        if (p.no_chao) {
            f32 y;
            if (m.chao(p.pos.x, p.pos.z, mov::RAIO * 0.6f, p.pos.y + mov::DEGRAU, &y) && y > p.pos.y) {
                // Subindo: anda menos na horizontal para o passo ao longo do piso ter o mesmo tamanho.
                f32 hx = p.pos.x - antes.x, hz = p.pos.z - antes.z;
                f32 h = sqrtf(hx * hx + hz * hz), dy = y - p.pos.y;
                if (h > 1e-5f) {
                    f32 k = h / sqrtf(h * h + dy * dy);
                    p.pos.x = antes.x + hx * k;
                    p.pos.z = antes.z + hz * k;
                    m.chao(p.pos.x, p.pos.z, mov::RAIO * 0.6f, p.pos.y + mov::DEGRAU, &y);
                }
                if (y > p.pos.y) p.pos.y = y;
                m.empurrar(p.pos, p.altura(), mov::RAIO, passo);
            }
        }
    }
}
}  // namespace

// Queda livre (rápida, pouco controle) e planador (abre sozinho a 38 m do chão:
// desce devagar e anda mais). Ao tocar o chão, volta ao movimento normal.
static void passo_queda(Personagem& p, const ComandoMovimento& c, const MundoColisao& m, f32 dt) {
    f32 y_chao = -1e9f;
    bool tem = m.chao(p.pos.x, p.pos.z, mov::RAIO * 0.6f, p.pos.y + 0.01f, &y_chao);
    if (p.queda == QUEDA_LIVRE && tem && p.pos.y - y_chao < mov::PLANADOR_ALTURA) p.queda = PLANADOR;
    bool planando = p.queda == PLANADOR;
    bool mergulho = mergulhando(p.queda, c);
    f32 mx = c.x, my = c.y, l = sqrtf(mx * mx + my * my);
    if (l > 1.0f) { mx /= l; my /= l; }
    f32 v = planando ? mov::PLANADOR_HORIZONTAL : (mergulho ? mov::MERGULHO_HORIZONTAL : mov::QUEDA_HORIZONTAL);
    f32 cs = cosf(c.yaw), sn = sinf(c.yaw);
    f32 alvo_x = (cs * mx - sn * my) * v, alvo_z = (-sn * mx - cs * my) * v;
    f32 hx = p.vel.x, hz = p.vel.z;
    f32 dx = alvo_x - hx, dz = alvo_z - hz, d = sqrtf(dx * dx + dz * dz), acel = mov::QUEDA_ACEL * dt;
    if (d <= acel || d < 1e-6f) { hx = alvo_x; hz = alvo_z; }
    else { hx += dx / d * acel; hz += dz / d * acel; }
    f32 vy = p.vel.y;
    auto aproximar = [](f32 a, f32 b, f32 k) { return a < b ? (a + k > b ? b : a + k) : (a - k < b ? b : a - k); };
    if (planando) {
        vy = aproximar(vy, -mov::PLANADOR_DESCIDA, mov::GRAVIDADE * dt);
    } else {
        f32 terminal = mergulho ? mov::MERGULHO_TERMINAL : mov::QUEDA_TERMINAL;
        // saindo do mergulho, freia até a terminal normal (sem tranco)
        vy = vy >= -terminal ? fmaxf(vy - mov::GRAVIDADE * dt, -terminal) : aproximar(vy, -terminal, mov::GRAVIDADE * 1.5f * dt);
    }
    p.no_chao = false;
    mover_horizontal(p, hx * dt, hz * dt, m);
    f32 novo_y = p.pos.y + vy * dt;
    tem = m.chao(p.pos.x, p.pos.z, mov::RAIO * 0.6f, p.pos.y + 0.01f, &y_chao);
    p.vel = Vec3(hx, vy, hz);
    if (tem && novo_y <= y_chao) {
        p.pos.y = y_chao;
        p.queda = NO_CHAO_NORMAL;
        p.no_chao = true;
        p.aterrissou = true;
        p.altura_queda = 0;  // pouso do planador: aterrissagem normal
        p.vel.y = 0;
    } else {
        p.pos.y = novo_y;
    }
    p.pico = p.pos.y;
}

void passo_movimento(Personagem& p, const ComandoMovimento& c, const MundoColisao& m, f32 dt) {
    p.yaw = c.yaw;
    p.pulou = p.aterrissou = false;
    if (p.queda == NO_ONIBUS) return;  // a posição vem do ônibus (a partida)
    if (p.queda != NO_CHAO_NORMAL) {
        p.agachado = p.correndo = false;
        passo_queda(p, c, m, dt);
        return;
    }
    // Agachar / levantar (levantar só se couber em pé).
    if (c.agachar && !p.agachado) p.agachado = true;
    else if (!c.agachar && p.agachado && m.livre(p.pos, mov::ALTURA, mov::RAIO - 0.02f)) p.agachado = false;

    bool no_chao = p.no_chao;
    p.correndo = c.correr && !p.agachado && c.y >= mov::CORRER_FRENTE_MIN;
    f32 vmax = p.agachado ? mov::AGACHADO : (p.correndo ? mov::CORRER : mov::ANDAR);
    f32 mx = c.x, my = c.y, l = sqrtf(mx * mx + my * my);
    if (l > 1.0f) { mx /= l; my /= l; }
    // Direção no mundo: yaw da câmera (0 = olhando para −z), como Basis(UP, yaw)·(x, 0, −y).
    f32 cs = cosf(c.yaw), sn = sinf(c.yaw);
    f32 alvo_x = (cs * mx - sn * my) * vmax, alvo_z = (-sn * mx - cs * my) * vmax;
    f32 hx = p.vel.x, hz = p.vel.z;
    f32 dx = alvo_x - hx, dz = alvo_z - hz, d = sqrtf(dx * dx + dz * dz);
    f32 acel = (no_chao ? mov::ACEL_CHAO : mov::ACEL_AR) * dt;
    if (d <= acel || d < 1e-6f) { hx = alvo_x; hz = alvo_z; }
    else { hx += dx / d * acel; hz += dz / d * acel; }

    f32 vy = p.vel.y;
    if (no_chao) {
        vy = mov::GRUDAR_VEL;
        if (c.pular && !p.agachado) { vy = mov::SALTO; p.pulou = true; }
    } else {
        vy = fmaxf(vy - mov::GRAVIDADE * dt, -mov::QUEDA_MAX);
    }

    // Horizontal (com paredes, degraus e rampas)…
    Vec3 antes = p.pos;
    mover_horizontal(p, hx * dt, hz * dt, m);
    // …depois vertical (chão e teto).
    f32 novo_y = p.pos.y + vy * dt;
    bool chao_agora = false;
    f32 y_chao;
    if (vy > 0) {
        f32 y_teto;
        if (m.teto(p.pos.x, p.pos.z, mov::RAIO, p.pos.y + p.altura(), novo_y + p.altura(), &y_teto)) {
            novo_y = y_teto - p.altura();
            vy = 0;
        }
        p.pos.y = novo_y;
    } else {
        bool tem = m.chao(p.pos.x, p.pos.z, mov::RAIO * 0.6f, p.pos.y + 0.01f, &y_chao);
        if (tem && novo_y <= y_chao) {
            p.pos.y = y_chao;
            chao_agora = true;
        } else {
            p.pos.y = novo_y;
        }
        // "Grudar" ao descer degraus/rampas (só quem já estava no chão e não pulou).
        if (!chao_agora && no_chao && !p.pulou) {
            if (m.chao(p.pos.x, p.pos.z, mov::RAIO * 0.6f, p.pos.y + 0.01f, &y_chao) && p.pos.y - y_chao <= mov::GRUDAR_DIST) {
                p.pos.y = y_chao;
                chao_agora = true;
            }
        }
    }
    if (chao_agora && vy < 0) vy = 0;
    // Velocidade real (o que as paredes deixaram andar) para a animação.
    p.vel = Vec3((p.pos.x - antes.x) / dt, vy, (p.pos.z - antes.z) / dt);
    if (!chao_agora && p.no_chao) p.pico = p.pos.y;
    if (!chao_agora) p.pico = fmaxf(p.pico, p.pos.y);
    if (chao_agora && !p.no_chao) {
        p.aterrissou = true;
        p.altura_queda = p.pico - p.pos.y;
    }
    p.no_chao = chao_agora;
}
