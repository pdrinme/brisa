// Etapa 3: a vitrine do kit visual do Ventania (game/tests/vitrine_kit.gd),
// remontada no PS2 com os assets convertidos pelo pipeline: as 14 peças de
// construção, árvores, pinheiros, pedra, arbustos, os dois baús (um abrindo) e
// dois personagens (idle e aceno). Mesmas posições, rotações, câmera e sol.
#pragma once
#include "conteudo/animacao.h"
#include "conteudo/modelo.h"
#include "conteudo/pak.h"
#include "nucleo/arena.h"
#include "nucleo/controle.h"
#include "nucleo/estatisticas.h"
#include "plataforma/plataforma.h"

class Vitrine {
public:
    static const int N_TRAJES = 11;
    enum ModoLod { LOD_AUTO, LOD_0, LOD_1, LOD_2, N_MODOS_LOD };
    // Carrega tudo do pacote. Em falha, `erro` explica.
    bool iniciar(const Pacote& pak, Arena& arena, char* erro, int n_erro);
    void passo(const Controle& c);
    void montar(Cena3D& cena, ListaDesenho2D& hud, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                const InfoPlataforma& info);

    Caminho3D caminho() const { return caminho_; }
    void definir_caminho(Caminho3D c) { caminho_ = c; }
    void definir_traje(int t) { traje_ = ((t % N_TRAJES) + N_TRAJES) % N_TRAJES; }
    void definir_lod(int m) { modo_lod_ = m % N_MODOS_LOD; }
    void definir_clipe(int c);
    int traje() const { return traje_; }
    int clipe() const { return clipe_; }
    int n_clipes() const { return anims_.n_clipes(); }
    void mostrar_hud(bool v) { hud_ = v; }
    void pausar(bool p) { pausado_ = p; }
    u32 bytes_modelos() const { return bytes_modelos_; }
    u32 n_modelos() const { return n_modelos_; }
    u32 triangulos_ultimo() const { return tris_; }

private:
    struct Peca { int modelo; f32 x, y, z, rot; };
    int carregar(const Pacote& pak, const char* id, char* erro, int ne);
    int lod_para(f32 distancia) const;
    void hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d, const InfoPlataforma& info) const;

    static const int MAX_MODELOS = 64;
    ModeloBrisa modelos_[MAX_MODELOS];
    char ids_[MAX_MODELOS][32];
    int n_ = 0;
    BancoAnimacoes anims_, anim_bau_;
    int trajes_[N_TRAJES][3];  // LOD0, LOD1, LOD2
    int bau_ = -1, chao_ = -1;
    Malha malha_chao_;
    Peca pecas_[40];
    int n_pecas_ = 0;
    int icones_[11];
    int n_icones_ = 0;
    int textura_sombra_ = -1;
    u32 bytes_modelos_ = 0, n_modelos_ = 0;
    u32 bytes_pak_ = 0, arquivos_pak_ = 0;
    int crc_falhas_ = 0;
    Caminho3D caminho_ = Caminho3D::VU1;
    int traje_ = 0, modo_lod_ = LOD_AUTO, clipe_ = 0, clipe_aceno_ = -1, clipe_abrir_ = -1;
    f32 t_ = 0;  // segundos
    f32 angulo_ = 1.5707963f, distancia_ = 16.56f, altura_ = 3.6f;
    bool hud_ = true, pausado_ = false;
    u32 tris_ = 0;
    int lod_usado_ = 0;
};
