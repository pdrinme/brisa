#include "fonte.h"

namespace fonte {
namespace {

struct Base {
    char c;
    const char* l[7];
};

// Desenho original do Brisa (grade 5×7).
const Base BASES[] = {
    {' ', {".....", ".....", ".....", ".....", ".....", ".....", "....."}},
    {'A', {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'B', {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}},
    {'C', {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}},
    {'D', {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}},
    {'E', {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}},
    {'F', {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}},
    {'G', {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}},
    {'H', {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'I', {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {'J', {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}},
    {'K', {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}},
    {'L', {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}},
    {'M', {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}},
    {'N', {"#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#"}},
    {'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'P', {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}},
    {'Q', {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}},
    {'R', {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}},
    {'S', {".####", "#....", "#....", ".###.", "....#", "....#", "####."}},
    {'T', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}},
    {'U', {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'V', {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}},
    {'W', {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."}},
    {'X', {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}},
    {'Y', {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}},
    {'Z', {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}},
    {'0', {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}},
    {'1', {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {'2', {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}},
    {'3', {"#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###."}},
    {'4', {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}},
    {'5', {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}},
    {'6', {"..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."}},
    {'7', {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}},
    {'8', {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}},
    {'9', {".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."}},
    {'!', {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}},
    {'?', {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."}},
    {'.', {".....", ".....", ".....", ".....", ".....", ".##..", ".##.."}},
    {',', {".....", ".....", ".....", ".....", ".##..", "..#..", ".#..."}},
    {':', {".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."}},
    {'-', {".....", ".....", ".....", "#####", ".....", ".....", "....."}},
    {'+', {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}},
    {'/', {".....", "....#", "...#.", "..#..", ".#...", "#....", "....."}},
    {'(', {"...#.", "..#..", ".#...", ".#...", ".#...", "..#..", "...#."}},
    {')', {".#...", "..#..", "...#.", "...#.", "...#.", "..#..", ".#..."}},
    {'%', {"##...", "##..#", "...#.", "..#..", ".#...", "#..##", "...##"}},
    {'=', {".....", ".....", "#####", ".....", "#####", ".....", "....."}},
    {'\'', {"..#..", "..#..", ".#...", ".....", ".....", ".....", "....."}},
    {'<', {"...#.", "..#..", ".#...", "#....", ".#...", "..#..", "...#."}},
    {'>', {".#...", "..#..", "...#.", "....#", "...#.", "..#..", ".#..."}},
    {'_', {".....", ".....", ".....", ".....", ".....", ".....", "#####"}},
    {'#', {".#.#.", ".#.#.", "#####", ".#.#.", "#####", ".#.#.", ".#.#."}},  // colocação (Etapa 8)
};

enum Acento { NENHUM, AGUDO, GRAVE, CIRCUNFLEXO, TIL, CEDILHA };

const char* const ACENTOS[][2] = {
    {".....", "....."},  // nenhum
    {"...#.", "..#.."},  // agudo
    {".#...", "..#.."},  // grave
    {"..#..", ".#.#."},  // circunflexo
    {".##.#", "#..#."},  // til
    {".....", "....."},  // cedilha (marca embaixo)
};

struct Composto {
    u32 codigo;
    char base;
    Acento acento;
};

const Composto COMPOSTOS[] = {
    {0xC1, 'A', AGUDO}, {0xC0, 'A', GRAVE}, {0xC2, 'A', CIRCUNFLEXO}, {0xC3, 'A', TIL},
    {0xC9, 'E', AGUDO}, {0xCA, 'E', CIRCUNFLEXO}, {0xCD, 'I', AGUDO},
    {0xD3, 'O', AGUDO}, {0xD4, 'O', CIRCUNFLEXO}, {0xD5, 'O', TIL},
    {0xDA, 'U', AGUDO}, {0xDC, 'U', NENHUM}, {0xC7, 'C', CEDILHA},
    {0xE1, 'A', AGUDO}, {0xE0, 'A', GRAVE}, {0xE2, 'A', CIRCUNFLEXO}, {0xE3, 'A', TIL},
    {0xE9, 'E', AGUDO}, {0xEA, 'E', CIRCUNFLEXO}, {0xED, 'I', AGUDO},
    {0xF3, 'O', AGUDO}, {0xF4, 'O', CIRCUNFLEXO}, {0xF5, 'O', TIL},
    {0xFA, 'U', AGUDO}, {0xFC, 'U', NENHUM}, {0xE7, 'C', CEDILHA},
};

u8 bits(const char* s) {
    u8 v = 0;
    for (int i = 0; i < LARGURA; i++) v = static_cast<u8>((v << 1) | (s[i] == '#' ? 1 : 0));
    return v;
}

const Base* achar_base(char c) {
    for (const Base& b : BASES)
        if (b.c == c) return &b;
    return nullptr;
}

}  // namespace

u32 proximo_codigo(const char** s) {
    const u8* p = reinterpret_cast<const u8*>(*s);
    if (!*p) return 0;
    u32 c;
    int n;
    if (p[0] < 0x80) { c = p[0]; n = 1; }
    else if ((p[0] & 0xE0) == 0xC0 && p[1]) { c = ((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu); n = 2; }
    else if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2]) { c = ((p[0] & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu); n = 3; }
    else { c = '?'; n = 1; }
    *s += n;
    return c;
}

bool glifo(u32 codigo, u8 linhas[ALTURA]) {
    for (int i = 0; i < ALTURA; i++) linhas[i] = 0;
    if (codigo >= 'a' && codigo <= 'z') codigo -= 32;
    if (codigo == 0xB7) {  // ponto médio "·"
        linhas[2 + 3] = bits("..#..");
        return true;
    }
    if (codigo == 0xD7) codigo = 'X';  // "×"
    char base = 0;
    Acento acento = NENHUM;
    if (codigo < 0x80) {
        base = static_cast<char>(codigo);
    } else {
        for (const Composto& k : COMPOSTOS)
            if (k.codigo == codigo) { base = k.base; acento = k.acento; }
    }
    const Base* b = base ? achar_base(base) : nullptr;
    bool achou = b != nullptr;
    if (!b) b = achar_base('?');
    linhas[0] = bits(ACENTOS[acento][0]);
    linhas[1] = bits(ACENTOS[acento][1]);
    for (int i = 0; i < 7; i++) linhas[2 + i] = bits(b->l[i]);
    if (acento == CEDILHA) linhas[9] = bits("..#..");
    if (acento == AGUDO || acento == GRAVE || acento == CIRCUNFLEXO || acento == TIL) {
        // A letra fica igual; o acento ocupa as 2 linhas de cima.
    }
    return achou;
}

f32 largura(const char* utf8, f32 escala) {
    int n = 0;
    while (proximo_codigo(&utf8)) n++;
    return n ? (n * AVANCO - 1) * escala : 0.0f;
}

// expandir > 0 cresce cada retângulo para todos os lados: é a dilatação do
// texto por um quadrado, igual a desenhá-lo em 8 direções, com 1/8 das primitivas.
static f32 desenhar_ex(ListaDesenho2D& lista, f32 x, f32 y, f32 escala, Cor cor, const char* utf8, f32 expandir) {
    u8 linhas[ALTURA];
    u32 c;
    while ((c = proximo_codigo(&utf8)) != 0) {
        if (c != ' ') {
            glifo(c, linhas);
            for (int r = 0; r < ALTURA; r++) {
                int col = 0;
                while (col < LARGURA) {
                    if (linhas[r] & (0x10 >> col)) {
                        int ini = col;
                        while (col < LARGURA && (linhas[r] & (0x10 >> col))) col++;
                        lista.retangulo(x + ini * escala - expandir, y + r * escala - expandir,
                                        (col - ini) * escala + 2 * expandir, escala + 2 * expandir, cor);
                    } else {
                        col++;
                    }
                }
            }
        }
        x += AVANCO * escala;
    }
    return x;
}

f32 desenhar(ListaDesenho2D& lista, f32 x, f32 y, f32 escala, Cor cor, const char* utf8) {
    return desenhar_ex(lista, x, y, escala, cor, utf8, 0);
}

void desenhar_adesivo(ListaDesenho2D& lista, f32 x, f32 y, f32 escala, Cor cor, Cor contorno,
                      f32 espessura, f32 sombra, const char* utf8) {
    // A sombra também leva o contorno, para ficar um bloco só (como um adesivo).
    if (sombra > 0) desenhar_ex(lista, x + sombra, y + sombra, escala, contorno, utf8, espessura);
    desenhar_ex(lista, x, y, escala, contorno, utf8, espessura);
    desenhar_ex(lista, x, y, escala, cor, utf8, 0);
}

}  // namespace fonte
