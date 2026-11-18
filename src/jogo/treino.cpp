#include "treino.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "nucleo/fonte.h"
#include "nucleo/formato.h"
#include "render3d/instancia.h"

namespace {
const f32 GRAUS = 3.14159265f / 180.0f;
const f32 LIMITE = 110.0f;  // CampoTreino.LIMITE
const Cor BRANCO = rgb(0xf6f3ec), TINTA = rgb(0x15161c), AMARELO = rgb(0xffc93c);
const char* const TRAJES[Treino::N_TRAJES] = {"recruta_base_01", "recruta_brisa_01", "capitao_mare_01", "entregador_01",
                                              "escafandro_01", "faz_tudo_01", "espantalho_01", "explorador_01",
                                              "botanico_01", "astronomo_01", "guardiao_01"};
struct Lugar { const char* id; f32 x, y, z, rot; };
// CampoTreino._construcoes() do Ventania (commit 7864cd7).
const Lugar CONSTRUCOES[] = {
    {"piso_madeira", -16, 0, -10, 0}, {"parede_madeira", -16, 0, -12, 0}, {"parede_madeira", -18, 0, -10, 90},
    {"parede_madeira", -14, 0, -10, -90}, {"teto_madeira", -16, 4, -10, 0},
    {"rampa_pedra", 0, 0, -14, 0}, {"piso_pedra", 0, 4, -18, 0}, {"rampa_pedra", 0, 4, -22, 0}, {"piso_pedra", 0, 8, -26, 0},
    {"parede_pedra", 0, 8, -28, 0}, {"parede_pedra", -2, 4, -18, 90},
    {"piso_metal", 16, 0, -10, 0}, {"parede_metal", 16, 0, -12, 0}, {"parede_metal", 18, 0, -10, -90},
    {"parede_metal", 14, 0, -10, 90}, {"piso_metal", 16, 4, -10, 0}, {"rampa_metal", 16, 0, -6, 0},
    {"piso_madeira", -16, 1.5f, 4, 0}, {"piso_madeira", -16, 1.5f, 8, 0},
};
// CampoTreino.CHESTS.
const Lugar BAUS[] = {
    {"bau_ventania_01", -16, 0.1f, -10.6f, 0}, {"bau_ventania_01", 0, 8.1f, -26.6f, 0}, {"bau_ventania_01", 16, 4.1f, -10, 90},
    {"bau_ventania_01", -7, 0, 3, 30}, {"bau_ventania_01", 10, 0, 5, -40}, {"bau_ventania_01", -34, 0, -28, 0},
    {"bau_ventania_01", 36, 0, 30, 180},
};
const Lugar PROPS[] = {
#include "campo_treino_props.inc"
};
f32 chao_plano(f32, f32) { return 0; }
void card(ListaDesenho2D& l, f32 x, f32 y, f32 w, f32 h) {
    l.retangulo(x + 4, y + 4, w, h, rgb(0x15161c, 140));
    l.retangulo(x, y, w, h, rgb(0x101c33, 215));
    l.moldura(x, y, w, h, 2, TINTA);
}
}  // namespace

bool Treino::iniciar(const Pacote& pak, Arena& arena, char* erro, int ne) {
    bib_.iniciar(&pak);
    mundo_.limpar(128.0f);
    mundo_.definir_chao(true, 0.0f);
    auto colocar = [&](const Lugar& l) -> bool {
        int m = bib_.carregar(l.id, erro, ne);
        if (m < 0 || n_inst_ >= 80) return false;
        inst_[n_inst_++] = {m, Vec3(l.x, l.y, l.z), l.rot};
        mundo_.adicionar_modelo(bib_[m], Vec3(l.x, l.y, l.z), l.rot);
        return true;
    };
    for (const Lugar& l : CONSTRUCOES) if (!colocar(l)) return false;
    for (const Lugar& l : BAUS) if (!colocar(l)) return false;
    for (const Lugar& l : PROPS) if (!colocar(l)) return false;
    // Bordas do campo (paredes invisíveis de CampoTreino._chao).
    mundo_.adicionar_caixa(Vec3(LIMITE, 0, 0), 0, Vec3(-1, 0, -LIMITE), Vec3(1, 60, LIMITE));
    mundo_.adicionar_caixa(Vec3(-LIMITE, 0, 0), 0, Vec3(-1, 0, -LIMITE), Vec3(1, 60, LIMITE));
    mundo_.adicionar_caixa(Vec3(0, 0, LIMITE), 0, Vec3(-LIMITE, 0, -1), Vec3(LIMITE, 60, 1));
    mundo_.adicionar_caixa(Vec3(0, 0, -LIMITE), 0, Vec3(-LIMITE, 0, -1), Vec3(LIMITE, 60, 1));
    for (int t = 0; t < N_TRAJES; t++)
        if ((trajes_[t] = bib_.carregar(TRAJES[t], erro, ne)) < 0) return false;
    u32 tam = 0;
    char e[96];
    const u8* d = pak.arquivo("animacoes/humanoide.bani", &tam);
    if (!d || !anims_.carregar(d, tam, e, sizeof e) || !anim_.iniciar(anims_)) { snprintf(erro, ne, "animações do personagem: %s", d ? e : "ausente"); return false; }
    ConstrutorMalha chao(arena, 12 * 12 * 2);
    chao.grade(-LIMITE * 1.1f, -LIMITE * 1.1f, LIMITE * 2.2f, 12, rgb(0x6cc24a), rgb(0x6cc24a), chao_plano);
    chao_ = chao.terminar();
    TexturaBrisa tex;
    d = pak.arquivo("texturas/sombra_blob.btex", &tam);
    if (d && tex.carregar(d, tam, e, sizeof e)) textura_sombra_ = plataforma::registrar_textura(tex);
    p_ = Personagem();
    p_.pos = ponto_inicial();
    cam_ = Camera3P();
    return mundo_.descartados() == 0;
}

void Treino::passo(const Controle& c) {
    const f32 dt = 1.0f / 30.0f;
    passos_++;
    const EstadoControle& e = c.estado();
    if (c.pressionou(BT_START)) hud_ = !hud_;
    if (c.pressionou(BT_SELECT)) definir_traje(traje_ + 1);
    if (c.pressionou(BT_BAIXO)) {
        Caminho3D o = caminho_ == Caminho3D::VU1 ? Caminho3D::EE : Caminho3D::VU1;
        if (plataforma::caminho_disponivel(o)) caminho_ = o;
    }
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
    passo_movimento(p_, cmd, mundo_, dt);
    if (p_.agachado != agachar_ && !agachar_) { /* ainda sem espaço para levantar: continua agachado */ }
    anim_.atualizar(p_, c.pressionou(BT_ESQUERDA), dt);
    cam_.atualizar(p_.pos, p_.agachado, mundo_, dt);
}

void Treino::montar(Cena3D& cena, ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                    const InfoPlataforma& info) {
    cena.limpar();
    cena.ceu = rgb(0xcfe9ff);
    cena.neblina.cor = rgb(0xcfe9ff);
    cena.neblina.inicio = 40.0f;
    cena.neblina.fim = 220.0f;
    cena.luz.direcao = Vec3(-0.369f, 0.766f, 0.527f);  // sol do campo de treino (−50°, −35°)
    cena.luz.limiar = 0.08f;
    cena.luz.suavidade = 0.06f;
    cena.luz.luz = Vec3(1.32f, 1.28f, 1.36f);
    cena.luz.sombra = Vec3(0.74f, 0.82f, 0.95f);
    cena.textura_sombra = textura_sombra_;
    cam_.aplicar(cena.camera, 4.0f / 3.0f);
    cena.adicionar(&chao_, Mat4::identidade());
    for (int i = 0; i < n_inst_; i++)
        adicionar_modelo(cena, bib_[inst_[i].modelo], nullptr,
                         Mat4::translacao(inst_[i].pos) * Mat4::rotacao_y(inst_[i].rot * GRAUS));
    // Personagem: o modelo olha para +z; o jogo usa −z como frente (como no Ventania).
    static Pose pose;
    const ModeloBrisa& m = bib_[trajes_[traje_]];
    anim_.pose(m, pose);
    adicionar_modelo(cena, m, &pose, Mat4::translacao(p_.pos) * Mat4::rotacao_y(p_.yaw + 3.14159265f));
    f32 y_chao = 0;
    if (mundo_.chao(p_.pos.x, p_.pos.z, 0.1f, p_.pos.y + 0.05f, &y_chao)) {
        f32 h = p_.pos.y - y_chao;
        cena.sombra(Vec3(p_.pos.x, y_chao, p_.pos.z), 0.55f * (1.0f - fminf(h / 12.0f, 0.6f)));
    }
    for (int i = 0; i < n_inst_; i++)
        if (strncmp(bib_[inst_[i].modelo].no(0).nome, "bau", 3) == 0) cena.sombra(inst_[i].pos, 0.75f);
    l.limpar(cena.ceu);
    if (hud_) hud(l, est, e3d, info);
}

void Treino::hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d, const InfoPlataforma& info) const {
    char linha[112], a[16], b[16], c[16], d[16], v[16];
    card(l, 16, 14, 608, 70);
    snprintf(linha, sizeof linha, "CAMPO DE TREINO · %s · %s", caminho_ == Caminho3D::VU1 ? "VU1" : "EE", info.nome);
    fonte::desenhar(l, 26, 20, 2, AMARELO, linha);
    formato::decimal(a, sizeof a, static_cast<u32>(fabsf(p_.pos.x) * 100), 100, 1);
    formato::decimal(b, sizeof b, static_cast<u32>(fmaxf(p_.pos.y, 0) * 100), 100, 2);
    formato::decimal(c, sizeof c, static_cast<u32>(fabsf(p_.pos.z) * 100), 100, 1);
    formato::decimal(v, sizeof v, static_cast<u32>(anim_.velocidade() * 100), 100, 1);
    snprintf(linha, sizeof linha, "X %s%s Y %s Z %s%s · %s M/S · %s", p_.pos.x < 0 ? "-" : "", a, b, p_.pos.z < 0 ? "-" : "", c, v,
             anim_.estado());
    fonte::desenhar(l, 26, 42, 2, BRANCO, linha);
    formato::decimal(a, sizeof a, e3d.tempo_3d_us, 1000, 1);
    formato::decimal(d, sizeof d, est.fps_x10(), 10, 1);
    snprintf(linha, sizeof linha, "%s · %u TRI · 3D %s MS · %s FPS", p_.no_chao ? "NO CHÃO" : "NO AR",
             static_cast<unsigned>(e3d.triangulos_enviados), a, d);
    fonte::desenhar(l, 26, 64, 2, rgb(0xf6f3ec, 190), linha);
    card(l, 16, 388, 608, 48);
    fonte::desenhar(l, 26, 392, 2, AMARELO, "X PULA · L3 CORRE · R3 AGACHA · L2 MIRA");
    fonte::desenhar(l, 26, 414, 2, AMARELO, "ESQ. ACENA · SELECT TRAJE · BAIXO 3D · START HUD");
}
