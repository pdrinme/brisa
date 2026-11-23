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
// CampoTreino.LOOT_POINTS e DUMMIES (bonecos de treino: alvos que não revidam).
const Vec3 PONTOS_LOOT[] = {
    Vec3(-16, 0.1f, -9), Vec3(0, 4.1f, -18), Vec3(16, 0.1f, -9), Vec3(16, 4.1f, -11.2f), Vec3(-16, 1.6f, 6), Vec3(-24, 0, 12),
    Vec3(24, 0, 12), Vec3(-26, 0, -18), Vec3(26, 0, -20), Vec3(0, 0, 22), Vec3(-10, 0, 24), Vec3(12, 0, 24), Vec3(-40, 0, 0),
    Vec3(40, 0, 0), Vec3(0, 0, -40), Vec3(-3, 0, 8), Vec3(3, 0, 8),
};
const Vec3 BONECOS[Treino::N_BONECOS] = {Vec3(-5, 0, -2), Vec3(0, 0, -4), Vec3(5, 0, -2)};
f32 chao_plano(f32, f32) { return 0; }
void card(ListaDesenho2D& l, f32 x, f32 y, f32 w, f32 h) {
    l.retangulo(x + 4, y + 4, w, h, rgb(0x15161c, 140));
    l.retangulo(x, y, w, h, rgb(0x101c33, 215));
    l.moldura(x, y, w, h, 2, TINTA);
}
}  // namespace

bool Treino::iniciar(const Pacote& pak, Arena& arena, char* erro, int ne) {
    if (!bib_.iniciar(&pak, arena, 48)) { snprintf(erro, ne, "sem memória para a biblioteca de modelos"); return false; }
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
    for (const Lugar& l : BAUS) {
        if (!colocar(l)) return false;
        inst_[n_inst_ - 1].bau = static_cast<int>(&l - BAUS);
    }
    for (const Lugar& l : PROPS) if (!colocar(l)) return false;
    // Bordas do campo (paredes invisíveis de CampoTreino._chao).
    mundo_.adicionar_caixa(Vec3(LIMITE, 0, 0), 0, Vec3(-1, 0, -LIMITE), Vec3(1, 60, LIMITE));
    mundo_.adicionar_caixa(Vec3(-LIMITE, 0, 0), 0, Vec3(-1, 0, -LIMITE), Vec3(1, 60, LIMITE));
    mundo_.adicionar_caixa(Vec3(0, 0, LIMITE), 0, Vec3(-LIMITE, 0, -1), Vec3(LIMITE, 60, 1));
    mundo_.adicionar_caixa(Vec3(0, 0, -LIMITE), 0, Vec3(-LIMITE, 0, -1), Vec3(LIMITE, 60, 1));
    if (!jogador_.iniciar(pak, bib_, erro, ne)) return false;
    if (!hc_.iniciar(pak, bib_, arena, jogador_.itens(), erro, ne)) return false;
    // Loot do campo: baús (reabastecem em 45 s) e pontos de loot, com semente fixa.
    loot_.iniciar(&jogador_.itens(), 42);
    loot_.reabastecer = true;
    for (const Lugar& l : BAUS) loot_.adicionar_bau(Vec3(l.x, l.y, l.z), l.rot);
    for (const Vec3& p : PONTOS_LOOT) loot_.adicionar_ponto(p);
    loot_.sortear_pontos();
    for (int i = 0; i < N_BONECOS; i++) {
        bonecos_[i].pos = BONECOS[i];
        bonecos_[i].vit = Vitais();
        bonecos_[i].vit.escudo = 50;
        bonecos_[i].ultimo = 0;
    }
    if ((modelo_boneco_ = bib_.carregar("recruta_base_01", erro, ne)) < 0) return false;
    clipe_idle_ = jogador_.animacoes().achar("idle");
    u32 tam = 0;
    char e[96];
    const u8* d = nullptr;
    ConstrutorMalha chao(arena, 12 * 12 * 2, LIMITE * 1.1f + 1.0f);
    chao.grade(-LIMITE * 1.1f, -LIMITE * 1.1f, LIMITE * 2.2f, 12, rgb(0x6cc24a), rgb(0x6cc24a), chao_plano);
    chao_ = chao.terminar();
    TexturaBrisa tex;
    d = pak.arquivo("texturas/sombra_blob.btex", &tam);
    if (d && tex.carregar(d, tam, e, sizeof e)) textura_sombra_ = plataforma::registrar_textura(tex);
    jogador_.reposicionar(ponto_inicial(), 0);
    return mundo_.descartados() == 0;
}

void Treino::posicionar(Vec3 pes, f32 yaw, f32 pitch) { jogador_.reposicionar(pes, yaw, pitch); }

void Treino::passo(const Controle& c) {
    passos_++;
    if (c.pressionou(BT_START)) hud_ = !hud_;
    // Atalhos de depuração (L1 + R1 + …): SELECT troca o caminho de desenho; ● troca o traje.
    bool atalho = c.segurando(BT_L1) && c.segurando(BT_R1);
    if (atalho && c.pressionou(BT_SELECT)) {
        Caminho3D o = caminho_ == Caminho3D::VU1 ? Caminho3D::EE : Caminho3D::VU1;
        if (plataforma::caminho_disponivel(o)) caminho_ = o;
    }
    if (atalho && c.pressionou(BT_CIRCULO)) definir_traje(jogador_.traje() + 1);
    bool bloquear = !atalho && hc_.controlar_inventario(c, jogador_, loot_, mundo_);
    bloquear = bloquear || hc_.inventario_aberto();
    AlvoCombate alvos[N_BONECOS];
    for (int i = 0; i < N_BONECOS; i++) {
        alvos[i].id = static_cast<u16>(60001 + i);  // DUMMY_ID_BASE do Ventania
        alvos[i].pes = bonecos_[i].pos;
        alvos[i].vit = &bonecos_[i].vit;
        alvos[i].ultimo_acerto = &bonecos_[i].ultimo;
    }
    ev_ = EventosCombate();
    jogador_.passo(c, mundo_, loot_, alvos, N_BONECOS, passos_, bloquear, ev_);
    for (int k = 0; k < ev_.n_acertos; k++) {
        int b = ev_.acertos[k].alvo - 60001;
        if (b >= 0 && b < N_BONECOS) {
            dano_bonecos_ += ev_.acertos[k].dano;
            if (ev_.acertos[k].eliminou) {  // bonecos não morrem: voltam inteiros
                bonecos_[b].vit.reiniciar();
                bonecos_[b].vit.escudo = 50;
                eliminacoes_++;
            }
        }
    }
    // Bonecos se recuperam 3 s depois do último acerto (update_dummies).
    for (Boneco& b : bonecos_)
        if (passos_ - b.ultimo > 90 && (b.vit.vida < Vitais::VIDA_MAX || b.vit.escudo != 50)) {
            b.vit.reiniciar();
            b.vit.escudo = 50;
        }
    loot_.atualizar(passos_, 1.0f / 30.0f);
    hc_.registrar(ev_, jogador_);
    hc_.passo();
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
    cena.adicionar(&chao_, Mat4::identidade());
    for (int i = 0; i < n_inst_; i++) {
        const Pose* pose = inst_[i].bau >= 0 ? hc_.pose_bau(loot_.bau(inst_[i].bau).tampa) : nullptr;
        adicionar_modelo(cena, bib_[inst_[i].modelo], pose, Mat4::translacao(inst_[i].pos) * Mat4::rotacao_y(inst_[i].rot * GRAUS));
        if (inst_[i].bau >= 0) cena.sombra(inst_[i].pos, 0.75f);
    }
    jogador_.montar(cena, bib_, mundo_, &loot_, 4.0f / 3.0f);
    // Bonecos de treino: o traje padrão parado (idle).
    static Pose pose_boneco;
    jogador_.animacoes().amostrar(clipe_idle_, passos_ / 30.0f, bib_[modelo_boneco_], pose_boneco);
    for (const Boneco& b : bonecos_) {
        adicionar_modelo(cena, bib_[modelo_boneco_], &pose_boneco, Mat4::translacao(b.pos) * Mat4::rotacao_y(3.14159265f));
        cena.sombra(b.pos, 0.55f);
    }
    hc_.montar_loot(cena, loot_, bib_, jogador_, passos_ / 30.0f);
    l.limpar(cena.ceu);
    if (hud_) hud(l, est, e3d, info);
    hc_.desenhar(l, jogador_, loot_, cena.camera);
}

void Treino::hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d, const InfoPlataforma& info) const {
    char linha[112], a[16], b[16], c[16], d[16], v[16];
    const Personagem& p = jogador_.personagem();
    card(l, 16, 14, 608, 70);
    snprintf(linha, sizeof linha, "CAMPO DE TREINO · %s · %s", caminho_ == Caminho3D::VU1 ? "VU1" : "EE", info.nome);
    fonte::desenhar(l, 26, 20, 2, AMARELO, linha);
    formato::decimal(a, sizeof a, static_cast<u32>(fabsf(p.pos.x) * 100), 100, 1);
    formato::decimal(b, sizeof b, static_cast<u32>(fmaxf(p.pos.y, 0) * 100), 100, 2);
    formato::decimal(c, sizeof c, static_cast<u32>(fabsf(p.pos.z) * 100), 100, 1);
    formato::decimal(v, sizeof v, static_cast<u32>(jogador_.animador().velocidade() * 100), 100, 1);
    snprintf(linha, sizeof linha, "X %s%s Y %s Z %s%s · %s M/S · %s", p.pos.x < 0 ? "-" : "", a, b, p.pos.z < 0 ? "-" : "", c, v,
             jogador_.animador().estado());
    fonte::desenhar(l, 26, 42, 2, BRANCO, linha);
    formato::decimal(a, sizeof a, e3d.tempo_3d_us, 1000, 1);
    formato::decimal(d, sizeof d, est.fps_x10(), 10, 1);
    snprintf(linha, sizeof linha, "%s · %u TRI · 3D %s MS · %s FPS", p.no_chao ? "NO CHÃO" : "NO AR",
             static_cast<unsigned>(e3d.triangulos_enviados), a, d);
    fonte::desenhar(l, 26, 64, 2, rgb(0xf6f3ec, 190), linha);
    // vida dos bonecos (canto direito do painel)
    for (int i = 0; i < N_BONECOS; i++) {
        snprintf(linha, sizeof linha, "BONECO %d: %d/%d", i + 1, bonecos_[i].vit.vida, bonecos_[i].vit.escudo);
        fonte::desenhar(l, 474, 20 + i * 14, 1, BRANCO, linha);
    }
}
