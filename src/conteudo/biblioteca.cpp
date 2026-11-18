#include "biblioteca.h"

#include <stdio.h>
#include <string.h>

int BibliotecaModelos::carregar(const char* id, char* erro, int ne) {
    for (int i = 0; i < n_; i++)
        if (strcmp(id_[i], id) == 0) return i;
    if (!pak_ || n_ >= MAX) { snprintf(erro, ne, "biblioteca cheia ou sem pacote"); return -1; }
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
