// Itens de partida (data/itens.json do Brisa, compilado em dados/itens.bdat; os
// valores são os do Ventania). Índices de item como o ItemDB do Ventania:
// 0 = vazio, depois as armas, os consumíveis e as ferramentas, na ordem do JSON.
#pragma once
#include "nucleo/tipos.h"

struct RaridadeDado {
    char id[12];
    char nome[16];
    u32 cor, cor2;            // 0xRRGGBB
    f32 dano, recarga, peso_bau, peso_chao;
};
struct MunicaoDado {
    char id[12];
    char nome[24];
    u32 max, caixa, _;
};
struct MaterialDado {
    char id[12];
    char nome[16];
    u32 max;
};
enum PoseItem : u32 { POSE_NENHUMA = 0, POSE_RIFLE = 1, POSE_PRECISAO = 2, POSE_PISTOLA = 3, POSE_CONSUMIVEL = 4, POSE_PICARETA = 5 };
struct ArmaDado {
    char id[12];
    char nome[16];
    char tipo[24];
    char modelo[24];
    u32 municao;              // índice do tipo de munição
    f32 dano, cadencia;       // dano por projétil; disparos por segundo
    u32 automatica, pente;
    f32 recarga_s, alcance, dispersao;
    u32 projeteis;
    f32 cabeca, peso;
    f32 dispersao_mirando;    // < 0: usa o modificador "mirando"
    f32 _;
    u32 pose;                 // PoseItem
};
struct ConsumivelDado {
    char id[12];
    char nome[32];
    char modelo[24];
    f32 vida, escudo, limite, tempo_s;
    u32 pilha, quantidade;
    f32 peso;
    u32 _;
};
struct FerramentaDado {
    char id[12];
    char nome[16];
    char modelo[24];
    f32 cadencia, impacto, alcance;
    u32 dano_jogador, dano_construcao, _;
};
struct ModificadoresDispersao { f32 mirando, movendo, agachado, no_ar; };
struct BauDado { u32 armas, consumiveis, caixas_municao, _; };
static_assert(sizeof(RaridadeDado) == 52 && sizeof(MunicaoDado) == 48 && sizeof(MaterialDado) == 32, "BDAT");
static_assert(sizeof(ArmaDado) == 132 && sizeof(ConsumivelDado) == 100 && sizeof(FerramentaDado) == 76, "BDAT");

// Gerador determinístico (PCG32): o mesmo sorteio no PC e no PS2.
struct Aleatorio {
    u64 estado = 0x853c49e6748fea9bULL;
    void semear(u32 s) { estado = 0; proximo(); estado += 0xda3e39cb94b95bdbULL ^ s; proximo(); }
    u32 proximo() {
        u64 v = estado;
        estado = v * 6364136223846793005ULL + 1442695040888963407ULL;
        u32 x = static_cast<u32>(((v >> 18u) ^ v) >> 27u), r = static_cast<u32>(v >> 59u);
        return (x >> r) | (x << ((32 - r) & 31));
    }
    f32 unitario() { return (proximo() >> 8) * (1.0f / 16777216.0f); }  // [0, 1)
    int ate(int n) { return n > 0 ? static_cast<int>(proximo() % static_cast<u32>(n)) : 0; }
};

class BancoItens {
public:
    static const u32 VERSAO = 1;
    bool carregar(const u8* dados, u32 tamanho, char* erro, int n_erro);
    int n_itens() const { return 1 + n_armas_ + n_cons_ + n_fer_; }
    int n_raridades() const { return n_rar_; }
    int n_municoes() const { return n_mun_; }
    int n_materiais() const { return n_mat_; }
    int n_armas() const { return n_armas_; }
    int n_consumiveis() const { return n_cons_; }
    bool eh_arma(int i) const { return i >= 1 && i <= n_armas_; }
    bool eh_consumivel(int i) const { return i > n_armas_ && i <= n_armas_ + n_cons_; }
    bool eh_ferramenta(int i) const { return i > n_armas_ + n_cons_ && i < n_itens(); }
    bool vai_no_espaco(int i) const { return eh_arma(i) || eh_consumivel(i); }
    const ArmaDado& arma(int i) const { return armas_[i - 1]; }
    const ConsumivelDado& consumivel(int i) const { return cons_[i - 1 - n_armas_]; }
    const FerramentaDado& ferramenta(int i) const { return fer_[i - 1 - n_armas_ - n_cons_]; }
    const RaridadeDado& raridade(int r) const { return rar_[r < 0 ? 0 : (r >= n_rar_ ? n_rar_ - 1 : r)]; }
    const MunicaoDado& municao(int t) const { return mun_[t]; }
    const MaterialDado& material(int t) const { return mat_[t]; }
    const ModificadoresDispersao& modificadores() const { return *mods_; }
    const BauDado& bau() const { return *bau_; }
    int indice(const char* id) const;          // 0 se não existe
    int picareta() const { return indice("picareta"); }
    const char* id(int i) const;
    const char* nome(int i) const;             // nome curto ("Rajada", "Curativo")
    // "Rifle de Assalto Rajada (Raro)" (raridade < 0: sem a raridade).
    void nome_completo(int i, int raridade, char* buf, int n) const;
    const char* modelo(int i) const;
    PoseItem pose(int i) const;
    int pilha(int i) const;                    // máximo por espaço
    f32 dano(int arma_i, int rar) const { return arma(arma_i).dano * raridade(rar).dano; }
    f32 recarga(int arma_i, int rar) const { return arma(arma_i).recarga_s * raridade(rar).recarga; }
    // Sorteios (itens.gd do Ventania): peso 0 nunca sai (ex.: Mítico).
    int sortear_raridade(Aleatorio& r, bool de_bau) const;
    int sortear_arma(Aleatorio& r) const;
    int sortear_consumivel(Aleatorio& r) const;
private:
    int n_rar_ = 0, n_mun_ = 0, n_mat_ = 0, n_armas_ = 0, n_cons_ = 0, n_fer_ = 0;
    const RaridadeDado* rar_ = nullptr;
    const MunicaoDado* mun_ = nullptr;
    const MaterialDado* mat_ = nullptr;
    const ArmaDado* armas_ = nullptr;
    const ConsumivelDado* cons_ = nullptr;
    const FerramentaDado* fer_ = nullptr;
    const ModificadoresDispersao* mods_ = nullptr;
    const BauDado* bau_ = nullptr;
};
