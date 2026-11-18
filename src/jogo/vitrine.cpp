#include "vitrine.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "nucleo/fonte.h"
#include "nucleo/formato.h"
#include "render3d/instancia.h"

namespace {
const char* const TRAJES[Vitrine::N_TRAJES][2] = {
    {"recruta_base_01", "Recruta"}, {"recruta_brisa_01", "Recruta Brisa"}, {"capitao_mare_01", "Capitão Maré"},
    {"entregador_01", "Entregador"}, {"escafandro_01", "Escafandro"}, {"faz_tudo_01", "Faz-Tudo"},
    {"espantalho_01", "Espantalho"}, {"explorador_01", "Explorador das Nuvens"}, {"botanico_01", "Botânico"},
    {"astronomo_01", "Astrônomo"}, {"guardiao_01", "Guardião da Tempestade"}};
const char* const ICONES[] = {"rajada", "trovao", "zumbido", "faisca", "gaviao", "picareta", "curativo", "kit_cura",
                              "celula_p", "celula_g", "municao"};
const char* const MODOS_LOD[] = {"LOD AUTO", "LOD 0", "LOD 1", "LOD 2"};
const f32 GRAUS = 3.14159265f / 180.0f;
const Cor BRANCO = rgb(0xf6f3ec), TINTA = rgb(0x15161c), AMARELO = rgb(0xffc93c);
const Cor NOITE_CARTAO = rgb(0x101c33, 215);
const Vec3 ALVO(1.5f, 2.2f, -4.0f);

// Posições da vitrine do Ventania (game/tests/vitrine_kit.gd, commit 7864cd7).
struct Lugar { const char* id; f32 x, y, z, rot; f32 sombra; };
const Lugar LUGARES[] = {
    {"piso_madeira", -8, 0.11f, -6, 0, 0}, {"parede_madeira", -8, 0, -8, 0, 0}, {"parede_madeira", -10, 0, -6, 90, 0},
    {"teto_madeira", -8, 4, -6, 0, 0}, {"rampa_pedra", 0, 0, -6, 0, 0}, {"piso_pedra", 0, 4, -10, 0, 0},
    {"parede_pedra", 0, 4, -12, 0, 0}, {"teto_pedra", -4, 0, -14, 0, 0}, {"piso_metal", 8, 0.11f, -6, 0, 0},
    {"parede_metal", 8, 0, -8, 0, 0}, {"parede_metal", 10, 0, -6, -90, 0}, {"teto_metal", 8, 4, -6, 0, 0},
    {"rampa_madeira", -8, 0, -2, 0, 0}, {"rampa_metal", 8, 0, -2, 0, 0},
    {"arvore_redonda_01", -13, 0, -1, 20, 1.6f}, {"pinheiro_01", -15, 0, -11, 0, 1.3f}, {"pinheiro_01", 15, 0, -14, 40, 1.3f},
    {"arvore_redonda_01", 14, 0, -1, 140, 1.6f}, {"pinheiro_01", 5, 0, -17, 10, 1.3f}, {"pedra_01", 6.5f, 0, 1.5f, 30, 0.9f},
    {"arbusto_01", -4, 0, 2.5f, 0, 0.8f}, {"arbusto_01", 9.5f, 0, 3, 70, 0.8f},
};
const f32 BAU1[4] = {1.8f, 0, 2.5f, -20}, BAU2[4] = {3.6f, 0, 3.2f, 15};
const f32 PERS1[4] = {-0.6f, 0, 4.2f, 15}, PERS2[4] = {0.9f, 0, 4.6f, -10};

f32 chao_plano(f32, f32) { return 0.0f; }

void card(ListaDesenho2D& l, f32 x, f32 y, f32 w, f32 h) {
    l.retangulo(x + 4, y + 4, w, h, rgb(0x15161c, 140));
    l.retangulo(x, y, w, h, NOITE_CARTAO);
    l.moldura(x, y, w, h, 2, TINTA);
}

Mat4 lugar(f32 x, f32 y, f32 z, f32 rot) { return Mat4::translacao(Vec3(x, y, z)) * Mat4::rotacao_y(rot * GRAUS); }
}  // namespace

int Vitrine::carregar(const Pacote& pak, const char* id, char* erro, int ne) {
    for (int i = 0; i < n_; i++)
        if (strcmp(ids_[i], id) == 0) return i;
    if (n_ >= MAX_MODELOS) { snprintf(erro, ne, "modelos demais"); return -1; }
    char nome[64];
    snprintf(nome, sizeof nome, "modelos/%s.bmod", id);
    u32 t = 0;
    const u8* d = pak.arquivo(nome, &t);
    if (!d) { snprintf(erro, ne, "%s não está no pacote", nome); return -1; }
    char e2[96];
    if (!modelos_[n_].carregar(d, t, e2, sizeof e2)) { snprintf(erro, ne, "%s: %s", nome, e2); return -1; }
    snprintf(ids_[n_], sizeof ids_[n_], "%s", id);
    bytes_modelos_ += t;
    n_modelos_++;
    return n_++;
}

bool Vitrine::iniciar(const Pacote& pak, Arena& arena, char* erro, int ne) {
    bytes_pak_ = pak.tamanho();
    arquivos_pak_ = pak.n();
    char e[96] = "";
    crc_falhas_ = pak.verificar(e, sizeof e);
    for (const Lugar& l : LUGARES) {
        int m = carregar(pak, l.id, erro, ne);
        if (m < 0) return false;
        pecas_[n_pecas_++] = {m, l.x, l.y, l.z, l.rot};
    }
    if ((bau_ = carregar(pak, "bau_ventania_01", erro, ne)) < 0) return false;
    for (int t = 0; t < N_TRAJES; t++) {
        char id[48];
        trajes_[t][0] = carregar(pak, TRAJES[t][0], erro, ne);
        snprintf(id, sizeof id, "%s_lod1", TRAJES[t][0]);
        trajes_[t][1] = carregar(pak, id, erro, ne);
        snprintf(id, sizeof id, "%s_lod2", TRAJES[t][0]);
        trajes_[t][2] = carregar(pak, id, erro, ne);
        if (trajes_[t][0] < 0 || trajes_[t][1] < 0 || trajes_[t][2] < 0) return false;
    }
    u32 t = 0;
    const u8* d = pak.arquivo("animacoes/humanoide.bani", &t);
    if (!d || !anims_.carregar(d, t, e, sizeof e)) { snprintf(erro, ne, "humanoide.bani: %s", d ? e : "ausente"); return false; }
    for (int i = 0; i < N_TRAJES; i++)
        for (int k = 0; k < 3; k++)
            if (modelos_[trajes_[i][k]].assinatura() != anims_.assinatura()) {
                snprintf(erro, ne, "%s: esqueleto diferente das animações", ids_[trajes_[i][k]]);
                return false;
            }
    d = pak.arquivo("animacoes/bau_ventania_01.bani", &t);
    if (!d || !anim_bau_.carregar(d, t, e, sizeof e)) { snprintf(erro, ne, "bau_ventania_01.bani: %s", d ? e : "ausente"); return false; }
    if (modelos_[bau_].assinatura() != anim_bau_.assinatura()) { snprintf(erro, ne, "baú: animação não confere"); return false; }
    clipe_ = anims_.achar("idle");
    clipe_aceno_ = anims_.achar("acenar");
    clipe_abrir_ = anim_bau_.achar("abrir");
    if (clipe_ < 0 || clipe_aceno_ < 0 || clipe_abrir_ < 0) { snprintf(erro, ne, "clipe idle/acenar/abrir ausente"); return false; }
    // Chão da vitrine: 80 × 80 m de grama (#6cc24a), em 8 × 8 quadrados para a neblina por vértice.
    ConstrutorMalha chao(arena, 8 * 8 * 2);
    chao.grade(-40, -40, 80, 8, rgb(0x6cc24a), rgb(0x6cc24a), chao_plano);
    malha_chao_ = chao.terminar();
    // Texturas: sombra blob e ícones do inventário.
    TexturaBrisa tex;
    d = pak.arquivo("texturas/sombra_blob.btex", &t);
    if (d && tex.carregar(d, t, e, sizeof e)) textura_sombra_ = plataforma::registrar_textura(tex);
    for (const char* ic : ICONES) {
        char nome[64];
        snprintf(nome, sizeof nome, "texturas/icone_%s.btex", ic);
        d = pak.arquivo(nome, &t);
        if (d && tex.carregar(d, t, e, sizeof e)) icones_[n_icones_++] = plataforma::registrar_textura(tex);
    }
    return true;
}

void Vitrine::definir_clipe(int c) {
    int n = anims_.n_clipes();
    clipe_ = ((c % n) + n) % n;
}

int Vitrine::lod_para(f32 d) const {
    switch (modo_lod_) {
    case LOD_0: return 0;
    case LOD_1: return 1;
    case LOD_2: return 2;
    default: return d < 14.0f ? 0 : (d < 32.0f ? 1 : 2);
    }
}

void Vitrine::passo(const Controle& c) {
    if (c.pressionou(BT_START)) hud_ = !hud_;
    if (c.pressionou(BT_SELECT)) {
        Caminho3D o = caminho_ == Caminho3D::VU1 ? Caminho3D::EE : Caminho3D::VU1;
        if (plataforma::caminho_disponivel(o)) caminho_ = o;
    }
    if (c.pressionou(BT_R1)) definir_traje(traje_ + 1);
    if (c.pressionou(BT_L1)) definir_traje(traje_ - 1);
    if (c.pressionou(BT_R2)) definir_lod(modo_lod_ + 1);
    if (c.pressionou(BT_CIRCULO)) definir_clipe(clipe_ + 1);
    if (c.pressionou(BT_QUADRADO)) definir_clipe(clipe_ - 1);
    if (c.pressionou(BT_XIS)) pausado_ = !pausado_;
    if (c.pressionou(BT_TRIANGULO)) { angulo_ = 1.5707963f; distancia_ = 16.56f; altura_ = 3.6f; }
    f32 lx = Controle::eixo(c.estado().lx), ly = Controle::eixo(c.estado().ly), ry = Controle::eixo(c.estado().ry);
    angulo_ += lx * 0.04f;
    distancia_ += ly * 0.4f;
    altura_ -= ry * 0.15f;
    distancia_ = distancia_ < 3 ? 3 : (distancia_ > 60 ? 60 : distancia_);
    altura_ = altura_ < 0.5f ? 0.5f : (altura_ > 30 ? 30 : altura_);
    if (!pausado_) t_ += 1.0f / 30.0f;
}

void Vitrine::montar(Cena3D& cena, ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                     const InfoPlataforma& info) {
    cena.limpar();
    // Céu e neblina como aparecem na imagem da vitrine do Ventania (a neblina da
    // Godot cobre o fundo: o céu sai em #cfe9ff).
    cena.ceu = rgb(0xcfe9ff);
    cena.neblina.cor = rgb(0xcfe9ff);
    cena.neblina.inicio = 30.0f;
    cena.neblina.fim = 260.0f;
    // Calibração: a Godot ilumina em espaço linear e soma a luz ambiente; medido na
    // imagem de referência, o lado do sol fica ~1,3× a cor da paleta e o da sombra
    // ~0,75–0,95× (azulado). docs/etapa3/ tem a comparação.
    cena.luz.luz = Vec3(1.32f, 1.28f, 1.36f);
    cena.luz.sombra = Vec3(0.74f, 0.82f, 0.95f);
    // Sol da vitrine (rotação −50°, −35°) e a faixa de luz do toon_paleta.gdshader
    // (limite 0,05, suavidade 0,06 → centro 0,08).
    cena.luz.direcao = Vec3(-0.369f, 0.766f, 0.527f);
    cena.luz.limiar = 0.08f;
    cena.luz.suavidade = 0.06f;
    cena.textura_sombra = textura_sombra_;
    Vec3 olho(ALVO.x + cosf(angulo_) * distancia_, altura_, ALVO.z + sinf(angulo_) * distancia_);
    cena.camera.configurar(olho, ALVO, 55.0f * GRAUS, 4.0f / 3.0f, 0.3f, 300.0f);

    cena.adicionar(&malha_chao_, Mat4::identidade());
    for (int i = 0; i < n_pecas_; i++) {
        const Peca& p = pecas_[i];
        adicionar_modelo(cena, modelos_[p.modelo], nullptr, lugar(p.x, p.y, p.z, p.rot));
    }
    for (const Lugar& l2 : LUGARES)
        if (l2.sombra > 0) cena.sombra(Vec3(l2.x, 0, l2.z), l2.sombra);
    // Baús: o primeiro abre e fecha em ciclo; o segundo fica fechado.
    static Pose pose;
    f32 dur = anim_bau_.duracao(clipe_abrir_);
    f32 tb = fmodf(t_ + 0.6f, dur * 2 + 2.0f);  // começa aberto (seek 0,6 s na vitrine do Ventania)
    anim_bau_.amostrar(clipe_abrir_, tb < dur + 1 ? tb : dur - (tb - dur - 1), modelos_[bau_], pose);
    adicionar_modelo(cena, modelos_[bau_], &pose, lugar(BAU1[0], BAU1[1], BAU1[2], BAU1[3]));
    anim_bau_.amostrar(clipe_abrir_, 0, modelos_[bau_], pose);
    adicionar_modelo(cena, modelos_[bau_], &pose, lugar(BAU2[0], BAU2[1], BAU2[2], BAU2[3]));
    cena.sombra(Vec3(BAU1[0], 0, BAU1[2]), 0.75f);
    cena.sombra(Vec3(BAU2[0], 0, BAU2[2]), 0.75f);
    // Personagens: o 1º com o traje e o clipe escolhidos; o 2º é o Recruta acenando.
    f32 d1 = comprimento(olho - Vec3(PERS1[0], 1, PERS1[2]));
    lod_usado_ = lod_para(d1);
    const ModeloBrisa& p1 = modelos_[trajes_[traje_][lod_usado_]];
    anims_.amostrar(clipe_, t_, p1, pose);
    int antes = cena.n;
    adicionar_modelo(cena, p1, &pose, lugar(PERS1[0], PERS1[1], PERS1[2], PERS1[3]));
    (void)antes;
    tris_ = p1.triangulos();
    f32 d2 = comprimento(olho - Vec3(PERS2[0], 1, PERS2[2]));
    const ModeloBrisa& p2 = modelos_[trajes_[0][lod_para(d2)]];
    f32 da = anims_.duracao(clipe_aceno_);
    anims_.amostrar(clipe_aceno_, fmodf(t_ + 0.75f, da + 1.0f), p2, pose);
    adicionar_modelo(cena, p2, &pose, lugar(PERS2[0], PERS2[1], PERS2[2], PERS2[3]));
    cena.sombra(Vec3(PERS1[0], 0, PERS1[2]), 0.55f);
    cena.sombra(Vec3(PERS2[0], 0, PERS2[2]), 0.55f);

    l.limpar(cena.ceu);
    if (hud_) hud(l, est, e3d, info);
}

void Vitrine::hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d, const InfoPlataforma& info) const {
    char linha[112], a[16], b[16], c[16], d[16];
    card(l, 16, 14, 608, 92);
    snprintf(linha, sizeof linha, "VITRINE DO KIT · %s · %s", caminho_ == Caminho3D::VU1 ? "VU1" : "EE", info.nome);
    fonte::desenhar(l, 26, 20, 2, AMARELO, linha);
    snprintf(linha, sizeof linha, "%s · %s (%d) · %u TRI", TRAJES[traje_][1], MODOS_LOD[modo_lod_], lod_usado_,
             static_cast<unsigned>(tris_));
    fonte::desenhar(l, 26, 42, 2, BRANCO, linha);
    formato::decimal(a, sizeof a, est.media_quadro_us(), 1000, 1);
    formato::decimal(b, sizeof b, e3d.tempo_3d_us, 1000, 1);
    formato::decimal(c, sizeof c, est.fps_x10(), 10, 1);
    (void)a;
    snprintf(linha, sizeof linha, "ANIM %s · CENA %u TRI · 3D %s · %s FPS", anims_.nome(clipe_),
             static_cast<unsigned>(e3d.triangulos_enviados), b, c);
    fonte::desenhar(l, 26, 64, 2, BRANCO, linha);
    formato::decimal(a, sizeof a, bytes_pak_ / 1024, 1024, 1);
    formato::decimal(d, sizeof d, info.memoria_livre / 1024, 1024, 1);
    snprintf(linha, sizeof linha, "PACOTE %u ARQ. · %s MB · CRC %s · LIVRE %s MB", static_cast<unsigned>(arquivos_pak_), a,
             crc_falhas_ ? "FALHOU" : "OK", d);
    fonte::desenhar(l, 26, 86, 2, crc_falhas_ ? rgb(0xff6b5b) : rgb(0xf6f3ec, 190), linha);
    // Ícones do inventário (texturas CLUT convertidas do Ventania).
    card(l, 16, 352, 608, 82);
    for (int i = 0; i < n_icones_; i++) l.sprite(26 + i * 54, 358, 48, 48, icones_[i]);
    fonte::desenhar(l, 26, 410, 2, AMARELO, "L1/R1 TRAJE · R2 LOD · O ANIM · SELECT VU1/EE");
}
