// Etapa 5: a Ilha Ventania (world/ilha/ilha_ventania.gd do Ventania) no PS2 —
// relevo, mar, construções da grade, props, baús, estradas, regiões e minimapa —
// com o personagem da Etapa 4. Corte por setor de 30 m, distância por categoria
// e LOD1 (BRISA_ARCHITECTURE §18).
#pragma once
#include "conteudo/biblioteca.h"
#include "jogador.h"
#include "mundo/mapa.h"
#include "mundo/terreno.h"
#include "nucleo/estatisticas.h"
#include "plataforma/plataforma.h"

struct EstatisticasIlha {
    u32 setores = 0;            // setores dentro do alcance e do frustum
    u32 instancias = 0;         // instâncias olhadas
    u32 desenhadas = 0;         // instâncias postas na cena
    u32 em_lod1 = 0;
    u32 triangulos_instancias = 0;
    u32 tris_categoria[N_CATEGORIAS] = {};  // peça, árvore, pequeno, móvel, grande, baú, estrada
    EstatisticasTerreno terreno;
    u32 cpu_corte_us = 0;       // montar a cena (corte + matrizes), sem desenhar
};

class Ilha {
public:
    static const int MAX_MODELOS = 192;
    f32 dist_visao = 240.0f;    // neblina termina aqui
    bool iniciar(const Pacote& pak, Arena& arena, char* erro, int n_erro);
    void passo(const Controle& c);   // 30 Hz
    void montar(Cena3D& cena, ListaDesenho2D& hud, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                const InfoPlataforma& info);
    // Põe o personagem no chão em (x, z), com a câmera olhando para `yaw` graus.
    void ir_para(f32 x, f32 z, f32 yaw_graus, f32 pitch_graus = -10.0f);
    Caminho3D caminho() const { return caminho_; }
    void definir_caminho(Caminho3D c) { caminho_ = c; }
    void definir_traje(int t) { jogador_.definir_traje(t); }
    void mostrar_hud(bool h) { hud_ = h; }
    void mostrar_mapa(bool m) { mapa_cheio_ = m; }
    const Jogador& jogador() const { return jogador_; }
    Jogador& jogador_mut() { return jogador_; }
    const MundoColisao& mundo() const { return mundo_; }
    const MapaBrisa& mapa() const { return mapa_; }
    const Terreno& terreno() const { return terreno_; }
    const EstatisticasIlha& estatisticas() const { return est_; }
    const BibliotecaModelos& biblioteca() const { return bib_; }
    u32 passos() const { return passos_; }
    // Região (POI) onde o personagem está, ou nullptr.
    const char* regiao() const;
private:
    void montar_mapa(Cena3D& cena);
    void hud(ListaDesenho2D& l, const EstatisticasQuadro& est, const Estatisticas3D& e3d, const InfoPlataforma& info) const;
    void minimapa(ListaDesenho2D& l, f32 x, f32 y, f32 tam, f32 raio_m) const;
    MapaBrisa mapa_;
    BibliotecaModelos bib_;
    i16 bib_de_[MAX_MODELOS];   // modelo do mapa → índice na biblioteca
    MundoColisao mundo_;
    Terreno terreno_;
    Jogador jogador_;
    EstatisticasIlha est_;
    int tex_minimapa_ = -1, tex_sombra_ = -1;
    Caminho3D caminho_ = Caminho3D::VU1;
    bool hud_ = true, mapa_cheio_ = false;
    u32 passos_ = 0;
};
