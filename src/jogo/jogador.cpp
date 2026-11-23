#include "jogador.h"

#include <math.h>
#include <stdio.h>

#include "render3d/instancia.h"

namespace {
const char* const TRAJES[Jogador::N_TRAJES] = {"recruta_base_01", "recruta_brisa_01", "capitao_mare_01", "entregador_01",
                                               "escafandro_01", "faz_tudo_01", "espantalho_01", "explorador_01",
                                               "botanico_01", "astronomo_01", "guardiao_01"};
}  // namespace

bool Jogador::iniciar(const Pacote& pak, BibliotecaModelos& bib, char* erro, int ne) {
    for (int t = 0; t < N_TRAJES; t++)
        if ((trajes_[t] = bib.carregar(TRAJES[t], erro, ne)) < 0) return false;
    u32 tam = 0;
    char e[96];
    const u8* d = pak.arquivo("animacoes/humanoide.bani", &tam);
    if (!d || !anims_.carregar(d, tam, e, sizeof e) || !anim_.iniciar(anims_)) {
        snprintf(erro, ne, "animações do personagem: %s", d ? e : "ausente");
        return false;
    }
    reposicionar(Vec3(), 0);
    return true;
}

void Jogador::reposicionar(Vec3 pes, f32 yaw, f32 pitch) {
    p_ = Personagem();
    p_.pos = pes;
    cam_ = Camera3P();
    cam_.yaw = fmodf(yaw + 720.0f, 360.0f);
    cam_.pitch = pitch;
    agachar_ = false;
}

void Jogador::passo(const Controle& c, const MundoColisao& m) {
    const f32 dt = 1.0f / 30.0f;
    const EstadoControle& e = c.estado();
    if (c.pressionou(BT_R3)) agachar_ = !agachar_;  // R3 alterna (Ventania)
    cam_.mirando = c.segurando(BT_L2);
    cam_.olhar(Controle::eixo(e.rx), Controle::eixo(e.ry), dt);
    ComandoMovimento cmd;
    cmd.x = Controle::eixo(e.lx);
    cmd.y = -Controle::eixo(e.ly);  // analógico para cima = frente
    cmd.yaw = cam_.yaw_rad();
    cmd.pular = c.pressionou(BT_XIS);
    cmd.correr = c.segurando(BT_L3);
    cmd.agachar = agachar_;
    passo_movimento(p_, cmd, m, dt);  // sem espaço para levantar, continua agachado
    anim_.atualizar(p_, c.pressionou(BT_ESQUERDA), dt);
    cam_.atualizar(p_.pos, p_.agachado, m, dt);
}

void Jogador::montar(Cena3D& cena, const BibliotecaModelos& bib, const MundoColisao& m, f32 aspecto) const {
    cam_.aplicar(cena.camera, aspecto);
    // O modelo olha para +z; o jogo usa −z como frente (como no Ventania).
    static Pose pose;
    const ModeloBrisa& mod = bib[trajes_[traje_]];
    anim_.pose(mod, pose);
    adicionar_modelo(cena, mod, &pose, Mat4::translacao(p_.pos) * Mat4::rotacao_y(p_.yaw + 3.14159265f));
    f32 y_chao = 0;
    if (m.chao(p_.pos.x, p_.pos.z, 0.1f, p_.pos.y + 0.05f, &y_chao)) {
        f32 h = p_.pos.y - y_chao;
        cena.sombra(Vec3(p_.pos.x, y_chao, p_.pos.z), 0.55f * (1.0f - fminf(h / 12.0f, 0.6f)));
    }
}
