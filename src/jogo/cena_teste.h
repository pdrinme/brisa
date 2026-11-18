// Etapa 2: cena de teste do motor gráfico e benchmark dos caminhos EE e VU1.
// Objetos procedurais na paleta do Ventania (casas de madeira, torre de pedra,
// galpão de metal, pinheiros, árvores redondas, baús azul-petróleo e amarelo,
// pedras) sobre um chão de grama com relevo suave. Não são os assets do
// Ventania: eles chegam pelo conversor na Etapa 3.
#pragma once
#include "nucleo/arena.h"
#include "nucleo/controle.h"
#include "nucleo/desenho2d.h"
#include "nucleo/estatisticas.h"
#include "plataforma/plataforma.h"
#include "render3d/cena3d.h"

struct ResultadoBench {
    Caminho3D caminho;
    bool carga;
    int nivel;
    u32 objetos, triangulos;
    u32 quadro_us, cpu_us, t3d_us;
};

class CenaTeste {
public:
    static const int NIVEIS = 7;       // densidade: 24 · 2^nivel instâncias
    static const int NIVEIS_CARGA = 5;  // esferas de 1.280 triângulos: 4, 8, 16, 32, 48
    bool iniciar(Arena& arena);
    void passo(const Controle& c);
    // Monta a cena 3D e o HUD do quadro.
    void montar(Cena3D& cena, ListaDesenho2D& hud, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                const InfoPlataforma& info);
    // Chamado depois de cada quadro com as medições (alimenta o benchmark).
    void medir(u32 quadro_us, u32 cpu_us, const Estatisticas3D& e3d);

    Caminho3D caminho() const { return caminho_; }
    void definir_caminho(Caminho3D c) { caminho_ = c; }
    int nivel() const { return nivel_; }
    bool carga() const { return carga_; }
    void definir_nivel(int nivel, bool carga);
    bool bench_ativo() const { return bench_ativo_; }
    void iniciar_bench();
    int n_resultados() const { return n_res_; }
    const ResultadoBench& resultado(int i) const { return res_[i]; }
    u32 instancias() const;
    f32 angulo() const { return angulo_; }
    void pausar(bool p) { pausado_ = p; }

private:
    void montar_instancias(Cena3D& cena) const;
    void montar_hud(ListaDesenho2D& hud, const EstatisticasQuadro& est, const Estatisticas3D& e3d,
                    const InfoPlataforma& info) const;
    void aplicar_config_bench();

    enum { CHAO, CASA, TORRE, GALPAO, PINHEIRO, ARVORE, BAU, PEDRA, CARGA, N_MALHAS };
    Malha malhas_[N_MALHAS];
    Caminho3D caminho_ = Caminho3D::VU1;
    int nivel_ = 3;
    bool carga_ = false;
    bool pausado_ = false;
    f32 angulo_ = 0.6f, distancia_ = 70.0f, altura_ = 26.0f;
    u32 passos_ = 0;
    // Benchmark automático.
    bool bench_ativo_ = false;
    int bench_cfg_ = 0;
    u32 bench_quadro_ = 0;
    u64 soma_q_ = 0, soma_c_ = 0, soma_3d_ = 0;
    u32 amostras_ = 0;
    Estatisticas3D ultimo_e3d_;
    static const int MAX_RES = 2 * (NIVEIS + NIVEIS_CARGA);
    ResultadoBench res_[MAX_RES];
    int n_res_ = 0;
};
