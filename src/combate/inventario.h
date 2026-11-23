// Inventário de partida (inventory.gd do Ventania):
// - picareta: equipamento permanente, fora dos espaços (selecionado = PICARETA);
//   não pode ser largada, trocada nem perdida;
// - 5 espaços: armas (1 por espaço) ou pilhas de consumíveis (limite "pilha");
// - reservas fora dos espaços: munição por tipo e materiais (limite "max").
// Nenhuma operação perde ou duplica itens: o que sai volta para quem chamou.
#pragma once
#include "itens.h"

struct Espaco {
    u8 item = 0;       // índice do BancoItens (0 = vazio)
    u8 raridade = 0;
    u16 pente = 0;     // armas: balas no pente
    u16 qtd = 0;       // consumíveis: quantidade (armas: 1)
    bool vazio() const { return item == 0; }
};

class Inventario {
public:
    static const int ESPACOS = 5;
    static const int PICARETA = -1;
    static const int MAX_TIPOS = 8;
    enum Erro { OK, CHEIO, INVALIDO, NAO_DESCARTAVEL, SEM_ESPACO, VAZIO, LONGE, SUMIU };
    static const char* mensagem(Erro e);
    struct Adicao {
        int pegou = 0;       // quanto entrou
        Espaco largado;      // item trocado (vai para o chão), se tem_largado
        bool tem_largado = false;
        Erro erro = OK;
        int espaco = -1;
    };
    void iniciar(const BancoItens* banco);
    const BancoItens& banco() const { return *b_; }
    Espaco espacos[ESPACOS];
    u16 municao[MAX_TIPOS] = {};
    u16 materiais[MAX_TIPOS] = {};
    int selecionado = PICARETA;
    const Espaco& atual() const { return selecionado == PICARETA ? picareta_ : espacos[selecionado]; }
    Espaco& atual() { return selecionado == PICARETA ? picareta_ : espacos[selecionado]; }
    bool picareta_na_mao() const { return selecionado == PICARETA; }
    static bool espaco_valido(int i) { return i >= 0 && i < ESPACOS; }
    // Ciclo da barra: picareta → 1 → … → 5 → picareta (R1/L1).
    static int proximo_no_ciclo(int atual, int sentido);
    bool selecionar(int i);
    int primeiro_livre() const;
    int livres() const;
    int por_municao(int tipo, int n);     // devolve quanto coube
    int por_material(int tipo, int n);
    int cabe(int item) const;             // quanto deste item cabe sem trocar nada
    // Consumíveis completam pilhas e depois ocupam espaços livres. Sem lugar: só troca
    // se `trocar` for um espaço válido e ocupado (o item trocado volta em `largado`).
    Adicao adicionar(int item, int raridade, int qtd = 1, int pente = -1, int trocar = -1);
    Espaco tirar_espaco(int i);
    Erro trocar(int a, int b);            // a seleção acompanha o item da mão
    Erro dividir(int a, int n);           // n unidades do espaço a para o primeiro livre
    Erro tirar_de(int a, int n, Espaco* saiu);  // para largar no mundo (arma: inteira)
    int tirar_municao(int tipo, int n);
    int tirar_material(int tipo, int n);
    int recarga_possivel() const;         // balas que a recarga moveria agora
    int concluir_recarga();
    int largar_tudo(Espaco* saida);       // devolve quantos (a picareta fica)
    // Contagem total de um item (testes de conservação).
    int total(int item) const;
private:
    Espaco novo(int item, int raridade, int qtd, int pente) const;
    const BancoItens* b_ = nullptr;
    Espaco picareta_;
};

// Vida e escudo (vitals.gd): o dano consome o escudo primeiro.
struct Vitais {
    static const int VIDA_MAX = 100, ESCUDO_MAX = 100;
    int vida = VIDA_MAX, escudo = 0;
    struct Dano { int no_escudo = 0, na_vida = 0; bool eliminou = false; };
    bool vivo() const { return vida > 0; }
    Dano tomar_dano(int n) {
        Dano d;
        if (!vivo() || n <= 0) return d;
        d.no_escudo = escudo < n ? escudo : n;
        escudo -= d.no_escudo;
        int resto = n - d.no_escudo;
        d.na_vida = vida < resto ? vida : resto;
        vida -= d.na_vida;
        d.eliminou = vida <= 0;
        return d;
    }
    int curar(int n, int limite) {
        if (!vivo() || vida >= limite) return 0;
        int k = n < limite - vida ? n : limite - vida;
        vida += k;
        return k;
    }
    int por_escudo(int n, int limite) {
        if (!vivo() || escudo >= limite) return 0;
        int k = n < limite - escudo ? n : limite - escudo;
        escudo += k;
        return k;
    }
    void reiniciar() { vida = VIDA_MAX; escudo = 0; }
};
