#include "biblioteca.h"

#include <new>
#include <stdio.h>
#include <string.h>

bool BibliotecaModelos::iniciar(const Pacote* pak, Arena& arena, int cap) {
    pak_ = pak;
    n_ = 0;
    bytes_ = 0;
    m_ = arena.alocar_n<ModeloBrisa>(static_cast<size_t>(cap));
    id_ = reinterpret_cast<char (*)[32]>(arena.alocar_n<char>(static_cast<size_t>(cap) * 32));
    if (!m_ || !id_) { max_ = 0; return false; }
    for (int i = 0; i < cap; i++) new (&m_[i]) ModeloBrisa();
    max_ = cap;
    return true;
}

int BibliotecaModelos::carregar(const char* id, char* erro, int ne) {
    for (int i = 0; i < n_; i++)
        if (strcmp(id_[i], id) == 0) return i;
    if (!pak_ || n_ >= max_) { snprintf(erro, ne, "biblioteca cheia ou sem pacote"); return -1; }
    char nome[64];
    snprintf(nome, sizeof nome, "modelos/%s.bmod", id);
    u32 t = 0;
    const u8* d = pak_->arquivo(nome, &t);
    if (!d) { snprintf(erro, ne, "%s não está no pacote", nome); return -1; }
    char e[96];
    if (!m_[n_].carregar(d, t, e, sizeof e)) { snprintf(erro, ne, "%s: %s", nome, e); return -1; }
    snprintf(id_[n_], sizeof id_[n_], "%s", id);
    bytes_ += t;
    return n_++;
}
