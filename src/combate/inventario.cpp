#include "inventario.h"

const char* Inventario::mensagem(Erro e) {
    switch (e) {
    case OK: return "";
    case CHEIO: return "Inventário cheio: selecione o espaço que quer trocar.";
    case INVALIDO: return "Operação inválida.";
    case NAO_DESCARTAVEL: return "A picareta é permanente e não pode ser largada.";
    case SEM_ESPACO: return "Sem espaço livre para dividir.";
    case VAZIO: return "Esse espaço está vazio.";
    case LONGE: return "Longe demais para pegar.";
    case SUMIU: return "O item não está mais aí.";
    }
    return "";
}

void Inventario::iniciar(const BancoItens* b) {
    b_ = b;
    for (Espaco& e : espacos) e = Espaco();
    for (int i = 0; i < MAX_TIPOS; i++) municao[i] = materiais[i] = 0;
    selecionado = PICARETA;
    picareta_ = Espaco();
    picareta_.item = static_cast<u8>(b->picareta());
    picareta_.qtd = 1;
}

int Inventario::proximo_no_ciclo(int atual, int s) {
    // posições do ciclo: 0 = picareta, 1..5 = espaços
    int p = (atual == PICARETA ? 0 : atual + 1) + (s > 0 ? 1 : -1);
    p = (p + ESPACOS + 1) % (ESPACOS + 1);
    return p == 0 ? PICARETA : p - 1;
}

bool Inventario::selecionar(int i) {
    if ((i != PICARETA && !espaco_valido(i)) || i == selecionado) return false;
    selecionado = i;
    return true;
}

int Inventario::primeiro_livre() const {
    for (int i = 0; i < ESPACOS; i++)
        if (espacos[i].vazio()) return i;
    return -1;
}

int Inventario::livres() const {
    int n = 0;
    for (const Espaco& e : espacos) n += e.vazio();
    return n;
}

int Inventario::por_municao(int t, int n) {
    if (t < 0 || t >= b_->n_municoes() || n <= 0) return 0;
    int max = static_cast<int>(b_->municao(t).max), cabe = max - municao[t];
    int k = n < cabe ? n : cabe;
    municao[t] = static_cast<u16>(municao[t] + k);
    return k;
}

int Inventario::por_material(int t, int n) {
    if (t < 0 || t >= b_->n_materiais() || n <= 0) return 0;
    int max = static_cast<int>(b_->material(t).max), cabe = max - materiais[t];
    int k = n < cabe ? n : cabe;
    materiais[t] = static_cast<u16>(materiais[t] + k);
    return k;
}

int Inventario::cabe(int item) const {
    if (b_->eh_arma(item)) return primeiro_livre() >= 0 ? 1 : 0;
    if (!b_->eh_consumivel(item)) return 0;
    int p = b_->pilha(item), n = livres() * p;
    for (const Espaco& e : espacos)
        if (e.item == item) n += p - e.qtd;
    return n;
}

Espaco Inventario::novo(int item, int r, int qtd, int pente) const {
    Espaco e;
    e.item = static_cast<u8>(item);
    e.raridade = static_cast<u8>(r);
    if (b_->eh_arma(item)) {
        e.pente = static_cast<u16>(pente >= 0 ? pente : static_cast<int>(b_->arma(item).pente));
        e.qtd = 1;
    } else {
        e.qtd = static_cast<u16>(qtd);
    }
    return e;
}

Inventario::Adicao Inventario::adicionar(int item, int r, int qtd, int pente, int trocar) {
    Adicao a;
    if (!b_->vai_no_espaco(item) || qtd <= 0) { a.erro = INVALIDO; return a; }
    int resta = qtd;
    if (b_->eh_consumivel(item)) {
        int p = b_->pilha(item);
        for (int i = 0; i < ESPACOS && resta > 0; i++) {
            Espaco& e = espacos[i];
            if (e.item == item && e.qtd < p) {
                int k = p - e.qtd < resta ? p - e.qtd : resta;
                e.qtd = static_cast<u16>(e.qtd + k);
                resta -= k;
                a.espaco = i;
            }
        }
        while (resta > 0 && primeiro_livre() >= 0) {
            int l = primeiro_livre();
            espacos[l] = novo(item, r, resta < p ? resta : p, pente);
            resta -= espacos[l].qtd;
            a.espaco = l;
        }
        if (resta < qtd) { a.pegou = qtd - resta; return a; }
    } else {
        int l = primeiro_livre();
        if (l >= 0) {
            espacos[l] = novo(item, r, 1, pente);
            a.pegou = 1;
            a.espaco = l;
            return a;
        }
    }
    // Nada coube: troca só se pedida explicitamente.
    if (!espaco_valido(trocar) || espacos[trocar].vazio()) { a.erro = CHEIO; return a; }
    a.largado = espacos[trocar];
    a.tem_largado = true;
    int k = b_->eh_arma(item) ? 1 : (qtd < b_->pilha(item) ? qtd : b_->pilha(item));
    espacos[trocar] = novo(item, r, k, pente);
    a.pegou = k;
    a.espaco = trocar;
    return a;
}

Espaco Inventario::tirar_espaco(int i) {
    Espaco e = espacos[i];
    espacos[i] = Espaco();
    return e;
}

Inventario::Erro Inventario::trocar(int a, int b) {
    if (!espaco_valido(a) || !espaco_valido(b) || a == b) return INVALIDO;
    if (espacos[a].vazio() && espacos[b].vazio()) return VAZIO;
    Espaco t = espacos[a];
    espacos[a] = espacos[b];
    espacos[b] = t;
    if (selecionado == a) selecionado = b;
    else if (selecionado == b) selecionado = a;
    return OK;
}

Inventario::Erro Inventario::dividir(int a, int n) {
    if (a == PICARETA) return NAO_DESCARTAVEL;
    if (!espaco_valido(a)) return INVALIDO;
    Espaco& e = espacos[a];
    if (e.vazio()) return VAZIO;
    if (!b_->eh_consumivel(e.item) || n < 1 || n >= e.qtd) return INVALIDO;
    int l = primeiro_livre();
    if (l < 0) return SEM_ESPACO;
    espacos[l] = novo(e.item, e.raridade, n, -1);
    e.qtd = static_cast<u16>(e.qtd - n);
    return OK;
}

Inventario::Erro Inventario::tirar_de(int a, int n, Espaco* saiu) {
    if (a == PICARETA) return NAO_DESCARTAVEL;
    if (!espaco_valido(a)) return INVALIDO;
    Espaco& e = espacos[a];
    if (e.vazio()) return VAZIO;
    if (b_->eh_arma(e.item) || n >= e.qtd) { *saiu = tirar_espaco(a); return OK; }
    if (n < 1) return INVALIDO;
    *saiu = e;
    saiu->qtd = static_cast<u16>(n);
    e.qtd = static_cast<u16>(e.qtd - n);
    return OK;
}

int Inventario::tirar_municao(int t, int n) {
    if (t < 0 || t >= b_->n_municoes() || n < 1) return 0;
    int k = n < municao[t] ? n : municao[t];
    municao[t] = static_cast<u16>(municao[t] - k);
    return k;
}

int Inventario::tirar_material(int t, int n) {
    if (t < 0 || t >= b_->n_materiais() || n < 1) return 0;
    int k = n < materiais[t] ? n : materiais[t];
    materiais[t] = static_cast<u16>(materiais[t] - k);
    return k;
}

int Inventario::recarga_possivel() const {
    const Espaco& e = atual();
    if (!b_->eh_arma(e.item)) return 0;
    const ArmaDado& w = b_->arma(e.item);
    int falta = static_cast<int>(w.pente) - e.pente, tem = municao[w.municao];
    return falta < tem ? falta : tem;
}

int Inventario::concluir_recarga() {
    int n = recarga_possivel();
    if (n > 0) {
        Espaco& e = atual();
        e.pente = static_cast<u16>(e.pente + n);
        municao[b_->arma(e.item).municao] = static_cast<u16>(municao[b_->arma(e.item).municao] - n);
    }
    return n;
}

int Inventario::largar_tudo(Espaco* saida) {
    int n = 0;
    for (int i = 0; i < ESPACOS; i++)
        if (!espacos[i].vazio()) saida[n++] = tirar_espaco(i);
    selecionado = PICARETA;
    return n;
}

int Inventario::total(int item) const {
    int n = 0;
    for (const Espaco& e : espacos)
        if (e.item == item) n += e.qtd;
    return n;
}
