#include "itens.h"

#include <stdio.h>
#include <string.h>

namespace {
struct CabecalhoDados {
    char magia[4];
    u32 versao, n_rar, n_mun, n_mat, n_armas, n_cons, n_fer;
    u32 off_rar, off_mun, off_mat, off_armas, off_cons, off_fer, off_mods, off_bau;
};
static_assert(sizeof(CabecalhoDados) == 64, "CabecalhoDados");

template <typename T> int escolher_peso(Aleatorio& r, const T* v, int n, f32 (*peso)(const T&)) {
    f32 total = 0;
    for (int i = 0; i < n; i++) total += peso(v[i]);
    f32 x = r.unitario() * total;
    int ultimo = 0;
    for (int i = 0; i < n; i++) {
        f32 p = peso(v[i]);
        if (p <= 0) continue;
        ultimo = i;
        x -= p;
        if (x <= 0) return i;
    }
    return ultimo;
}
}  // namespace

bool BancoItens::carregar(const u8* d, u32 t, char* erro, int ne) {
    if (!d || t < 64 || memcmp(d, "BDAT", 4) != 0) { snprintf(erro, ne, "dados sem assinatura BDAT"); return false; }
    const CabecalhoDados* c = reinterpret_cast<const CabecalhoDados*>(d);
    if (c->versao != VERSAO) { snprintf(erro, ne, "versão dos dados %u (o jogo lê a %u)", static_cast<unsigned>(c->versao), static_cast<unsigned>(VERSAO)); return false; }
    struct { u32 off, bytes; } b[] = {{c->off_rar, c->n_rar * 52}, {c->off_mun, c->n_mun * 48}, {c->off_mat, c->n_mat * 32},
                                      {c->off_armas, c->n_armas * 132}, {c->off_cons, c->n_cons * 100},
                                      {c->off_fer, c->n_fer * 76}, {c->off_mods, 16}, {c->off_bau, 16}};
    for (auto& k : b)
        if (k.off + k.bytes > t || (k.off & 3)) { snprintf(erro, ne, "dados truncados"); return false; }
    if (c->n_rar == 0 || c->n_armas == 0 || c->n_mun > 8 || c->n_mat > 4) { snprintf(erro, ne, "dados inválidos"); return false; }
    n_rar_ = static_cast<int>(c->n_rar);
    n_mun_ = static_cast<int>(c->n_mun);
    n_mat_ = static_cast<int>(c->n_mat);
    n_armas_ = static_cast<int>(c->n_armas);
    n_cons_ = static_cast<int>(c->n_cons);
    n_fer_ = static_cast<int>(c->n_fer);
    rar_ = reinterpret_cast<const RaridadeDado*>(d + c->off_rar);
    mun_ = reinterpret_cast<const MunicaoDado*>(d + c->off_mun);
    mat_ = reinterpret_cast<const MaterialDado*>(d + c->off_mat);
    armas_ = reinterpret_cast<const ArmaDado*>(d + c->off_armas);
    cons_ = reinterpret_cast<const ConsumivelDado*>(d + c->off_cons);
    fer_ = reinterpret_cast<const FerramentaDado*>(d + c->off_fer);
    mods_ = reinterpret_cast<const ModificadoresDispersao*>(d + c->off_mods);
    bau_ = reinterpret_cast<const BauDado*>(d + c->off_bau);
    for (int i = 0; i < n_armas_; i++)
        if (armas_[i].municao >= static_cast<u32>(n_mun_)) { snprintf(erro, ne, "arma %d com munição inválida", i); return false; }
    return true;
}

const char* BancoItens::id(int i) const {
    if (eh_arma(i)) return arma(i).id;
    if (eh_consumivel(i)) return consumivel(i).id;
    if (eh_ferramenta(i)) return ferramenta(i).id;
    return "";
}

int BancoItens::indice(const char* nome) const {
    for (int i = 1; i < n_itens(); i++)
        if (strcmp(id(i), nome) == 0) return i;
    return 0;
}

const char* BancoItens::nome(int i) const {
    if (eh_arma(i)) return arma(i).nome;
    if (eh_consumivel(i)) return consumivel(i).nome;
    if (eh_ferramenta(i)) return ferramenta(i).nome;
    return "";
}

void BancoItens::nome_completo(int i, int r, char* buf, int n) const {
    if (!eh_arma(i)) { snprintf(buf, n, "%s", nome(i)); return; }
    if (r < 0) snprintf(buf, n, "%s %s", arma(i).tipo, arma(i).nome);
    else snprintf(buf, n, "%s %s (%s)", arma(i).tipo, arma(i).nome, raridade(r).nome);
}

const char* BancoItens::modelo(int i) const {
    if (eh_arma(i)) return arma(i).modelo;
    if (eh_consumivel(i)) return consumivel(i).modelo;
    if (eh_ferramenta(i)) return ferramenta(i).modelo;
    return "";
}

PoseItem BancoItens::pose(int i) const {
    if (eh_arma(i)) return static_cast<PoseItem>(arma(i).pose);
    if (eh_consumivel(i)) return POSE_CONSUMIVEL;
    if (eh_ferramenta(i)) return POSE_PICARETA;
    return POSE_NENHUMA;
}

int BancoItens::pilha(int i) const { return eh_consumivel(i) ? static_cast<int>(consumivel(i).pilha) : 1; }

int BancoItens::sortear_raridade(Aleatorio& r, bool de_bau) const {
    return de_bau ? escolher_peso<RaridadeDado>(r, rar_, n_rar_, [](const RaridadeDado& x) { return x.peso_bau; })
                  : escolher_peso<RaridadeDado>(r, rar_, n_rar_, [](const RaridadeDado& x) { return x.peso_chao; });
}
int BancoItens::sortear_arma(Aleatorio& r) const {
    return 1 + escolher_peso<ArmaDado>(r, armas_, n_armas_, [](const ArmaDado& x) { return x.peso; });
}
int BancoItens::sortear_consumivel(Aleatorio& r) const {
    return 1 + n_armas_ + escolher_peso<ConsumivelDado>(r, cons_, n_cons_, [](const ConsumivelDado& x) { return x.peso; });
}
