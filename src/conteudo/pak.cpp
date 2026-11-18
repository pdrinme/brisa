#include "pak.h"

#include <stdio.h>
#include <string.h>

#include "nucleo/crc32.h"

bool Pacote::abrir(const u8* d, u32 t, char* erro, int ne) {
    dados_ = nullptr;
    n_ = 0;
    if (!d || t < 64 || memcmp(d, "BPAK", 4) != 0) { snprintf(erro, ne, "pacote sem assinatura BPAK"); return false; }
    if (reinterpret_cast<uintptr_t>(d) & 63) { snprintf(erro, ne, "pacote fora do alinhamento de 64 bytes"); return false; }
    u32 cab[7];
    memcpy(cab, d + 4, sizeof cab);
    if (cab[0] != VERSAO) { snprintf(erro, ne, "versão do pacote %u (o jogo lê a %u)", static_cast<unsigned>(cab[0]), static_cast<unsigned>(VERSAO)); return false; }
    u32 n = cab[2], off_ind = cab[3], total = cab[4], crc_ind = cab[5];
    if (total > t || off_ind + n * 64 > t) { snprintf(erro, ne, "pacote truncado (%u de %u bytes)", static_cast<unsigned>(t), static_cast<unsigned>(total)); return false; }
    if (crc32(d + off_ind, n * 64) != crc_ind) { snprintf(erro, ne, "índice do pacote corrompido (CRC)"); return false; }
    const Entrada* e = reinterpret_cast<const Entrada*>(d + off_ind);
    for (u32 i = 0; i < n; i++)
        if (e[i].offset + e[i].tamanho > total || (e[i].offset & 63)) { snprintf(erro, ne, "entrada %u fora do pacote", static_cast<unsigned>(i)); return false; }
    dados_ = d;
    entradas_ = e;
    n_ = n;
    tamanho_ = total;
    versao_conteudo_ = cab[1];
    return true;
}

const u8* Pacote::arquivo(const char* nome, u32* tamanho) const {
    // Índice em ordem alfabética (o empacotador ordena): busca binária.
    int lo = 0, hi = static_cast<int>(n_) - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        int c = strncmp(nome, entradas_[m].nome, sizeof entradas_[m].nome);
        if (c == 0) {
            if (tamanho) *tamanho = entradas_[m].tamanho;
            return dados_ + entradas_[m].offset;
        }
        if (c < 0) hi = m - 1; else lo = m + 1;
    }
    return nullptr;
}

const char* Pacote::nome(u32 i) const { return i < n_ ? entradas_[i].nome : ""; }

int Pacote::verificar(char* erro, int ne) const {
    int falhas = 0;
    for (u32 i = 0; i < n_; i++)
        if (crc32(dados_ + entradas_[i].offset, entradas_[i].tamanho) != entradas_[i].crc) {
            if (falhas == 0 && erro) snprintf(erro, ne, "CRC errado em %s", entradas_[i].nome);
            falhas++;
        }
    return falhas;
}
