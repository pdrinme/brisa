// Etapa 5: a Ilha Ventania (world/ilha/ilha_ventania.gd do Ventania) no PS2 —
// relevo, mar, construções da grade, props, baús, estradas, regiões e minimapa —
// com o personagem da Etapa 4. Corte por setor de 30 m, distância por categoria
// e LOD1 (BRISA_ARCHITECTURE §18).
#pragma once
#include "conteudo/biblioteca.h"
#include "combate/loot.h"
#include "grupo_bots.h"
#include "hud_combate.h"
#include "mundo/navegacao.h"
#include "jogador.h"
#include "partida_br.h"
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
    static const int MAX_INSTANCIAS = 2048;
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
    const Loot& loot() const { return loot_; }
    // Etapa 7: os bots (15, nos pontos de início 1–15). Desligados até ativar (as vistas de medida não têm bots).
    void ativar_bots(int n, u32 semente, bool reaparecer = true, bool com_jogador = true);
    bool bots_ativos() const { return bots_ativos_; }
    GrupoBots& bots() { return grupo_; }
    const GrupoBots& bots() const { return grupo_; }
    Navegacao& navegacao() { return nav_; }
    // Etapa 8: partida Battle Royale (o jogador + `n_bots`): contagem, ônibus, Tormenta, fim.
    void comecar_partida(u32 semente, bool com_jogador = true, int n_bots = GrupoBots::MAX_BOTS);
    PartidaBR& partida() { return partida_; }
    const PartidaBR& partida() const { return partida_; }
    bool resultado_visivel() const { return resultado_; }
    u32 semente_partida() const { return semente_partida_; }
    // Teste de aceite: passo só dos bots (sem desenhar), o jogador de fora.
    void passo_sem_jogador();
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
    Loot loot_;
    HudCombate hc_;
    EventosCombate ev_;
    Navegacao nav_;
    GrupoBots grupo_;
    bool bots_ativos_ = false, com_jogador_ = true;
    u32 jogador_reviver_ = 0;
    void passo_bots(bool com_jogador);
    void controlar_partida(const Controle& c);
    void hud_partida(ListaDesenho2D& l) const;
    void tela_resultado(ListaDesenho2D& l) const;
    void montar_partida(Cena3D& cena);
    PartidaBR partida_;
    bool resultado_ = false, resultado_fim_ = false;
    u32 eliminado_em_ = 0, semente_partida_ = 1;
    void montar_bots(Cena3D& cena);
    void hud_bots(ListaDesenho2D& l, const Camera3D& cam) const;
    i16 bau_da_inst_[MAX_INSTANCIAS];
    EstatisticasIlha est_;
    int tex_minimapa_ = -1, tex_sombra_ = -1, modelo_onibus_ = -1;
    Caminho3D caminho_ = Caminho3D::VU1;
    bool hud_ = true, mapa_cheio_ = false;
    u32 passos_ = 0;
};
