#include "ilha.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "conteudo/textura.h"
#include "nucleo/fonte.h"
#include "nucleo/formato.h"
#include "render3d/instancia.h"

namespace {
const f32 GRAUS = 3.14159265f / 180.0f;
const Cor BRANCO = rgb(0xf6f3ec), TINTA = rgb(0x15161c), AMARELO = rgb(0xffc93c);
const f32 MARGEM_SETOR = 8.0f;  // instâncias passam um pouco da borda do setor

void card(ListaDesenho2D& l, f32 x, f32 y, f32 w, f32 h) {
    l.retangulo(x + 4, y + 4, w, h, rgb(0x15161c, 140));
    l.retangulo(x, y, w, h, rgb(0x101c33, 215));
    l.moldura(x, y, w, h, 2, TINTA);
}

// T(pos) · Ry(rot) · S(escala), sem multiplicar matrizes.
Mat4 matriz_instancia(const InstanciaMapa& k) {
    Mat4 m = Mat4::identidade();
    f32 c = cosf(k.rot * GRAUS) * k.escala, s = sinf(k.rot * GRAUS) * k.escala;
    m(0, 0) = c; m(0, 2) = s;
    m(1, 1) = k.escala;
    m(2, 0) = -s; m(2, 2) = c;
    m(0, 3) = k.x; m(1, 3) = k.y; m(2, 3) = k.z;
    return m;
}

// Seta do jogador (triângulo) no mapa: ponta para onde a câmera olha.
void seta(ListaDesenho2D& l, f32 x, f32 y, f32 yaw, f32 tam, Cor cor) {
    f32 fx = -sinf(yaw), fy = -cosf(yaw);  // −z = para cima no mapa
    f32 rx = -fy, ry = fx;
    f32 xs[4] = {x + fx * tam, x - fx * tam * 0.6f + rx * tam * 0.7f, x - fx * tam * 0.6f - rx * tam * 0.7f, x - fx * tam * 0.3f};
    f32 ys[4] = {y + fy * tam, y - fy * tam * 0.6f + ry * tam * 0.7f, y - fy * tam * 0.6f - ry * tam * 0.7f, y - fy * tam * 0.3f};
    f32 x2[4] = {xs[0], xs[1], xs[3], xs[2]}, y2[4] = {ys[0], ys[1], ys[3], ys[2]};
    l.quad(x2, y2, cor);
}
}  // namespace

bool Ilha::iniciar(const Pacote& pak, Arena& arena, char* erro, int ne) {
    u32 tam = 0;
    char e[96];
    const u8* d = pak.arquivo("mapa/ilha.bmap", &tam);
    if (!d || !mapa_.carregar(d, tam, e, sizeof e)) { snprintf(erro, ne, "mapa: %s", d ? e : "mapa/ilha.bmap ausente"); return false; }
    if (mapa_.n_modelos() > MAX_MODELOS) { snprintf(erro, ne, "mapa com %d modelos (máximo %d)", mapa_.n_modelos(), MAX_MODELOS); return false; }
    if (mapa_.n_instancias() > MAX_INSTANCIAS) { snprintf(erro, ne, "mapa com instâncias demais"); return false; }
    if (!bib_.iniciar(&pak, arena, mapa_.n_modelos() + Jogador::N_TRAJES * 3 + Jogador::MAX_ITENS + 4)) { snprintf(erro, ne, "sem memória para os modelos do mapa"); return false; }
    for (int i = 0; i < mapa_.n_modelos(); i++) {
        int k = bib_.carregar(mapa_.modelo(i).nome, erro, ne);
        if (k < 0) return false;
        bib_de_[i] = static_cast<i16>(k);
    }
    if (!jogador_.iniciar(pak, bib_, erro, ne)) return false;
    if (!hc_.iniciar(pak, bib_, arena, jogador_.itens(), erro, ne)) return false;
    d = pak.arquivo("mapa/ilha.bnav", &tam);
    if (!d || !nav_.carregar(d, tam, e, sizeof e)) { snprintf(erro, ne, "navegação: %s", d ? e : "mapa/ilha.bnav ausente"); return false; }
    if (!grupo_.iniciar(pak, &jogador_.recursos(), &mapa_, &nav_, erro, ne)) return false;
    // Loot da partida (loot_server.gd): os baús do mapa e os pontos de loot; sem reabastecer.
    loot_.iniciar(&jogador_.itens(), 20261116);  // a semente da ilha do Ventania
    loot_.reabastecer = false;
    for (int i = 0; i < mapa_.n_instancias() && i < MAX_INSTANCIAS; i++) {
        const InstanciaMapa& k = mapa_.instancia(i);
        bau_da_inst_[i] = -1;
        if (k.categoria == CAT_BAU) {
            bau_da_inst_[i] = static_cast<i16>(loot_.n_baus());
            loot_.adicionar_bau(Vec3(k.x, k.y, k.z), k.rot);
        }
    }
    for (int i = 0; i < mapa_.n_pontos_loot(); i++) loot_.adicionar_ponto(mapa_.ponto_loot(i));
    loot_.sortear_pontos();
    // Colisão: peças, props e baús (as estradas não colidem), o relevo e o limite da
    // costa (ilha_ventania.gd _limite_costa: 160 caixas a 9 m da praia).
    mundo_.limpar(304.0f);
    mundo_.definir_chao(false);
    mundo_.definir_relevo(&mapa_.relevo());
    for (int i = 0; i < mapa_.n_instancias(); i++) {
        const InstanciaMapa& k = mapa_.instancia(i);
        if (k.categoria == CAT_ESTRADA) continue;
        mundo_.adicionar_modelo(bib_[bib_de_[k.modelo]], Vec3(k.x, k.y, k.z), k.rot, k.escala);
    }
    const int N_COSTA = 160;
    for (int k = 0; k < N_COSTA; k++) {
        f32 a0 = k * 2 * 3.14159265f / N_COSTA, a1 = (k + 1) * 2 * 3.14159265f / N_COSTA;
        f32 r0 = mapa_.raio_costa(a0) + 9.0f, r1 = mapa_.raio_costa(a1) + 9.0f;
        f32 x0 = cosf(a0) * r0, z0 = sinf(a0) * r0, x1 = cosf(a1) * r1, z1 = sinf(a1) * r1;
        f32 dx = x1 - x0, dz = z1 - z0, L = sqrtf(dx * dx + dz * dz);
        mundo_.adicionar_caixa(Vec3((x0 + x1) * 0.5f, 50.0f, (z0 + z1) * 0.5f), -atan2f(dz, dx) / GRAUS,
                               Vec3(-(L + 1.5f) * 0.5f, -70.0f, -1.0f), Vec3((L + 1.5f) * 0.5f, 70.0f, 1.0f));
    }
    if (mundo_.descartados() > 0) { snprintf(erro, ne, "colisão: %d colisores ou referências descartados", mundo_.descartados()); return false; }
    if (!terreno_.iniciar(mapa_, arena)) { snprintf(erro, ne, "sem memória para o relevo"); return false; }
    terreno_.dist_max = dist_visao;
    TexturaBrisa tex;
    d = pak.arquivo("texturas/minimapa.btex", &tam);
    if (d && tex.carregar(d, tam, e, sizeof e)) tex_minimapa_ = plataforma::registrar_textura(tex);
    d = pak.arquivo("texturas/sombra_blob.btex", &tam);
    if (d && tex.carregar(d, tam, e, sizeof e)) tex_sombra_ = plataforma::registrar_textura(tex);
    // Começa no primeiro ponto de início de partida do Ventania, olhando para o centro.
    Vec3 s = mapa_.spawn(0);
    ir_para(s.x, s.z, atan2f(s.x, s.z) / GRAUS);
    return true;
}

void Ilha::ir_para(f32 x, f32 z, f32 yaw, f32 pitch) {
    f32 y = mapa_.relevo().altura(x, z);
    jogador_.reposicionar(Vec3(x, y + 0.05f, z), yaw);
    // assenta no chão (pisos de casa, se houver) sem mexer no x e z
    Controle parado;
    EstadoControle e;
    e.modo = ModoControle::ANALOGICO;
    parado.ler(e);
    parado.iniciar_passo();
    EventosCombate ev;
    for (int i = 0; i < 8; i++) jogador_.passo(parado, mundo_, loot_, nullptr, 0, passos_, true, ev);
    jogador_.reposicionar(jogador_.personagem().pos, yaw, pitch);
    for (int i = 0; i < 2; i++) jogador_.passo(parado, mundo_, loot_, nullptr, 0, passos_, true, ev);
}

void Ilha::passo(const Controle& c) {
    passos_++;
    if (c.pressionou(BT_START)) hud_ = !hud_;
    // Atalhos de depuração (L1 + R1 + …): SELECT troca o caminho de desenho; ● troca o traje.
    bool atalho = c.segurando(BT_L1) && c.segurando(BT_R1);
    if (atalho && c.pressionou(BT_SELECT)) {
        Caminho3D o = caminho_ == Caminho3D::VU1 ? Caminho3D::EE : Caminho3D::VU1;
        if (plataforma::caminho_disponivel(o)) caminho_ = o;
    }
    if (atalho && c.pressionou(BT_CIRCULO)) jogador_.definir_traje(jogador_.traje() + 1);
    if (c.pressionou(BT_CIMA) && !hc_.inventario_aberto()) mapa_cheio_ = !mapa_cheio_;
    bool bloquear = (!atalho && hc_.controlar_inventario(c, jogador_, loot_, mundo_)) || hc_.inventario_aberto() || mapa_cheio_;
    ev_ = EventosCombate();
    if (bots_ativos_) grupo_.pensar(&jogador_.lutador(), mundo_, loot_, passos_);
    AlvoCombate alvos[GrupoBots::MAX_LUTADORES];
    int na = bots_ativos_ ? grupo_.alvos(&jogador_.lutador(), nullptr, alvos) : 0;
    jogador_.passo(c, mundo_, loot_, alvos, na, passos_, bloquear, ev_);
    if (bots_ativos_) {
        grupo_.processar(jogador_.lutador(), ev_, &jogador_.lutador(), loot_, passos_);
        grupo_.simular(&jogador_.lutador(), mundo_, loot_, passos_);
        // o jogador eliminado reaparece em 4 s (fora da partida BR, como no treino do Ventania)
        Lutador& j = jogador_.lutador();
        if (!j.vivo && jogador_reviver_ == 0) jogador_reviver_ = passos_ + GrupoBots::REAPARECER;
        if (!j.vivo && grupo_.reaparecer && passos_ >= jogador_reviver_) {
            jogador_reviver_ = 0;
            j.vivo = true;
            j.comb.iniciar(&jogador_.itens(), Jogador::ID);
            Vec3 s = mapa_.spawn(static_cast<int>(passos_ % static_cast<u32>(mapa_.n_spawns())));
            ir_para(s.x, s.z, atan2f(s.x, s.z) / GRAUS);
        }
    }
    loot_.atualizar(passos_, 1.0f / 30.0f);
    hc_.registrar(ev_, jogador_);
    hc_.passo();
    // Caiu para fora do mundo (não deveria): volta para o início.
    if (jogador_.personagem().pos.y < mapa_.nivel_mar() - 30.0f) {
        Vec3 s = mapa_.spawn(0);
        ir_para(s.x, s.z, 0);
    }
}

void Ilha::ativar_bots(int n, u32 semente, bool reaparecer, bool com_jogador) {
    grupo_.reaparecer = reaparecer;
    grupo_.povoar(n, semente, 1);
    bots_ativos_ = n > 0;
    com_jogador_ = com_jogador;
    Lutador& j = jogador_.lutador();
    j.vivo = com_jogador;  // fora: partida só de bots (teste de aceite)
    if (com_jogador) {
        j.comb.vit = Vitais();
        jogador_reviver_ = 0;
    }
}

void Ilha::passo_sem_jogador() {
    passos_++;
    grupo_.pensar(nullptr, mundo_, loot_, passos_);
    grupo_.simular(nullptr, mundo_, loot_, passos_);
    loot_.atualizar(passos_, 1.0f / 30.0f);
}

void Ilha::montar_bots(Cena3D& cena) {
    if (!bots_ativos_) return;
    Frustum fr = Frustum::de(cena.camera.vp);
    Vec3 cam = cena.camera.posicao;
    for (int i = 0; i < grupo_.n_bots(); i++) {
        Lutador& b = grupo_.bot(i);
        if (!b.vivo) continue;
        Vec3 d = b.p.pos - cam;
        f32 dist = sqrtf(ponto(d, d));
        if (dist > 150.0f || fr.esfera(b.p.pos + Vec3(0, 0.9f, 0), 1.1f) == 0) continue;
        b.montar(cena, mundo_, dist < 18.0f ? 0 : (dist < 55.0f ? 1 : 2), dist < 40.0f);
    }
}

// Nomes acima dos bots perto (como os jogadores do Ventania).
void Ilha::hud_bots(ListaDesenho2D& l, const Camera3D& cam) const {
    if (!bots_ativos_) return;
    for (int i = 0; i < grupo_.n_bots(); i++) {
        const Lutador& b = grupo_.bot(i);
        if (!b.vivo) continue;
        Vec3 p = b.p.pos + Vec3(0, 2.05f, 0), d = p - cam.posicao;
        if (ponto(d, d) > 30.0f * 30.0f) continue;
        Vec4 c = transformar(cam.vp, p);
        if (c.w < cam.perto) continue;
        f32 x = 320.0f + 320.0f * c.x / c.w, y = 224.0f - 224.0f * c.y / c.w;
        if (x < 0 || x > 640 || y < 0 || y > 448) continue;
        fonte::desenhar_adesivo(l, x - fonte::largura(b.nome, 1) * 0.5f, y - 10, 1, BRANCO, TINTA, 1, 0, b.nome);
    }
}

const char* Ilha::regiao() const {
    int r = mapa_.regiao(jogador_.personagem().pos.x, jogador_.personagem().pos.z);
    return r >= 0 ? mapa_.poi(r).nome : nullptr;
}

void Ilha::montar_mapa(Cena3D& cena) {
    est_ = EstatisticasIlha();
    Frustum fr = Frustum::de(cena.camera.vp);
    Vec3 cam = cena.camera.posicao;
    terreno_.montar(cena, fr, cam, est_.terreno);
    const int S = mapa_.setores_lado();
    const f32 T = mapa_.tam_setor(), o = mapa_.origem();
    for (int sj = 0; sj < S; sj++)
        for (int si = 0; si < S; si++) {
            const SetorMapa& s = mapa_.setor(si, sj);
            if (s.n == 0) continue;
            Vec3 c(o + (si + 0.5f) * T, (s.ymin + s.ymax) * 0.5f, o + (sj + 0.5f) * T);
            f32 hy = (s.ymax - s.ymin) * 0.5f;
            f32 r = sqrtf(T * T * 0.5f + hy * hy) + MARGEM_SETOR;
            f32 dx = c.x - cam.x, dz = c.z - cam.z;
            if (sqrtf(dx * dx + dz * dz) - r > dist_visao) continue;
            if (fr.esfera(c, r) == 0) continue;
            est_.setores++;
            for (u32 i = s.inicio; i < s.inicio + s.n; i++) {
                const InstanciaMapa& k = mapa_.instancia(static_cast<int>(i));
                est_.instancias++;
                const ModeloMapa& mm = mapa_.modelo(k.modelo);
                f32 ex = k.x - cam.x, ey = k.y - cam.y, ez = k.z - cam.z;
                f32 dist = sqrtf(ex * ex + ey * ey + ez * ez);
                if (dist - k.raio > mm.dist_max || dist - k.raio > dist_visao) continue;
                int qual = k.modelo;
                // Cadeia de LODs: cada entrada aponta para a próxima (LOD1 → LOD2).
                if (mm.lod >= 0 && dist > mm.dist_lod) {
                    est_.em_lod1++;
                    do { qual = mapa_.modelo(qual).lod; } while (mapa_.modelo(qual).lod >= 0 && dist > mapa_.modelo(qual).dist_lod);
                }
                const ModeloBrisa& m = bib_[bib_de_[qual]];
                const Pose* pose = k.categoria == CAT_BAU && bau_da_inst_[i] >= 0 && qual == k.modelo
                                       ? hc_.pose_bau(loot_.bau(bau_da_inst_[i]).tampa) : nullptr;
                if (adicionar_modelo(cena, m, pose, matriz_instancia(k)) > 0) {
                    est_.desenhadas++;
                    est_.triangulos_instancias += m.triangulos();
                    if (k.categoria < N_CATEGORIAS) est_.tris_categoria[k.categoria] += m.triangulos();
                }
                if (k.categoria == CAT_BAU && dist < 40.0f) cena.sombra(Vec3(k.x, k.y, k.z), 0.75f);
            }
        }
}

void Ilha::montar(Cena3D& cena, ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                  const InfoPlataforma& info) {
    u64 t0 = plataforma::tempo_us();
    cena.limpar();
    cena.ceu = rgb(0x8fd3ff);           // fundo do Ventania (ilha_ventania.gd _ambiente)
    cena.neblina.cor = rgb(0xcfe9ff);
    cena.neblina.inicio = 60.0f;
    cena.neblina.fim = dist_visao;
    cena.luz.direcao = Vec3(-0.369f, 0.766f, 0.527f);  // sol (−50°, −35°), como no Ventania
    cena.luz.limiar = 0.08f;
    cena.luz.suavidade = 0.06f;
    cena.luz.luz = Vec3(1.32f, 1.28f, 1.36f);
    cena.luz.sombra = Vec3(0.74f, 0.82f, 0.95f);
    cena.textura_sombra = tex_sombra_;
    jogador_.montar(cena, bib_, mundo_, &loot_, 4.0f / 3.0f);
    montar_mapa(cena);
    montar_bots(cena);
    hc_.montar_loot(cena, loot_, bib_, jogador_, passos_ / 30.0f);
    est_.cpu_corte_us = static_cast<u32>(plataforma::tempo_us() - t0);
    l.limpar(cena.ceu);
    if (mapa_cheio_) {
        card(l, 112, 16, 416, 416);
        minimapa(l, 120, 24, 400, 300.0f);
        for (int p = 0; p < mapa_.n_pois(); p++) {
            const PoiMapa& q = mapa_.poi(p);
            f32 px = 120 + (q.x - mapa_.origem()) / 600.0f * 400, py = 24 + (q.z - mapa_.origem()) / 600.0f * 400;
            fonte::desenhar_adesivo(l, px - fonte::largura(q.nome, 1) * 0.5f, py - 5, 1, BRANCO, TINTA, 1, 0, q.nome);
        }
    }
    if (hud_) hud(l, est, e3d, info);
    if (!mapa_cheio_) {
        hud_bots(l, cena.camera);
        hc_.desenhar(l, jogador_, loot_, cena.camera);
    }
    if (bots_ativos_ && hud_) {
        // vivos e o feed de eliminações (canto direito, abaixo do minimapa)
        char t[64];
        snprintf(t, sizeof t, "%d VIVOS", grupo_.vivos(com_jogador_ ? &jogador_.lutador() : nullptr));
        fonte::desenhar_adesivo(l, 624 - fonte::largura(t, 2), 240, 2, BRANCO, TINTA, 1, 1, t);
        for (int i = 0; i < grupo_.n_feed(); i++) {
            const Eliminacao& f = grupo_.feed(i);
            if (passos_ - f.tick > 30 * 8) continue;
            char linha[80];
            snprintf(linha, sizeof linha, "%s > %s", f.quem, f.vitima);
            fonte::desenhar_adesivo(l, 624 - fonte::largura(linha, 1), 262 + i * 12, 1, BRANCO, TINTA, 1, 0, linha);
        }
    }
    if (bots_ativos_ && com_jogador_ && !jogador_.lutador().vivo) {
        char t[64];
        snprintf(t, sizeof t, "ELIMINADO POR %s", grupo_.nome_quem_eliminou);
        fonte::desenhar_adesivo(l, 320 - fonte::largura(t, 3) * 0.5f, 180, 3, rgb(0xff6b6b), TINTA, 2, 2, t);
        if (grupo_.reaparecer) {
            snprintf(t, sizeof t, "VOLTA EM %u S", static_cast<unsigned>((jogador_reviver_ > passos_ ? jogador_reviver_ - passos_ : 0) / 30 + 1));
            fonte::desenhar_adesivo(l, 320 - fonte::largura(t, 2) * 0.5f, 214, 2, BRANCO, TINTA, 1, 1, t);
        }
    }
}

// Minimapa (norte para cima): a textura de 600 m recortada em volta do jogador.
void Ilha::minimapa(ListaDesenho2D& l, f32 x, f32 y, f32 tam, f32 raio) const {
    const Personagem& p = jogador_.personagem();
    f32 o = mapa_.origem();
    // Centro da janela preso para ela não sair da textura.
    f32 cx = fminf(fmaxf(p.pos.x, o + raio), o + 600 - raio), cz = fminf(fmaxf(p.pos.z, o + raio), o + 600 - raio);
    f32 u0 = (cx - raio - o) / 600.0f, v0 = (cz - raio - o) / 600.0f, u1 = (cx + raio - o) / 600.0f, v1 = (cz + raio - o) / 600.0f;
    if (tex_minimapa_ >= 0) l.sprite_uv(x, y, tam, tam, tex_minimapa_, u0, v0, u1, v1);
    f32 px = x + (p.pos.x - (cx - raio)) / (2 * raio) * tam, py = y + (p.pos.z - (cz - raio)) / (2 * raio) * tam;
    seta(l, px, py, jogador_.camera().yaw_rad(), tam > 200 ? 9.0f : 7.0f, TINTA);
    seta(l, px, py, jogador_.camera().yaw_rad(), tam > 200 ? 6.0f : 4.5f, AMARELO);
}

void Ilha::hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d, const InfoPlataforma& info) const {
    char linha[128], a[16], b[16], c[16], d[16], v[16];
    const Personagem& p = jogador_.personagem();
    card(l, 16, 14, 608, 70);
    const char* reg = regiao();
    snprintf(linha, sizeof linha, "ILHA VENTANIA · %s · %s%s%s", caminho_ == Caminho3D::VU1 ? "VU1" : "EE", info.nome,
             reg ? " · " : "", reg ? reg : "");
    fonte::desenhar(l, 26, 20, 2, AMARELO, linha);
    formato::decimal(a, sizeof a, static_cast<u32>(fabsf(p.pos.x) * 10), 10, 1);
    formato::decimal(b, sizeof b, static_cast<u32>(fabsf(p.pos.y) * 10), 10, 1);
    formato::decimal(c, sizeof c, static_cast<u32>(fabsf(p.pos.z) * 10), 10, 1);
    formato::decimal(v, sizeof v, static_cast<u32>(jogador_.animador().velocidade() * 10), 10, 1);
    snprintf(linha, sizeof linha, "X %s%s Y %s%s Z %s%s · %s M/S · %s", p.pos.x < 0 ? "-" : "", a, p.pos.y < 0 ? "-" : "", b,
             p.pos.z < 0 ? "-" : "", c, v, jogador_.animador().estado());
    fonte::desenhar(l, 26, 42, 2, BRANCO, linha);
    formato::decimal(a, sizeof a, e3d.tempo_3d_us, 1000, 1);
    formato::decimal(d, sizeof d, est.fps_x10(), 10, 1);
    snprintf(linha, sizeof linha, "%u OBJ · %u TRI · 3D %s MS · %s FPS", static_cast<unsigned>(est_.desenhadas),
             static_cast<unsigned>(e3d.triangulos_enviados), a, d);
    fonte::desenhar(l, 26, 64, 2, rgb(0xf6f3ec, 190), linha);
    if (!mapa_cheio_) {
        card(l, 484, 92, 140, 140);
        minimapa(l, 486, 94, 136, 110.0f);
    }
}
