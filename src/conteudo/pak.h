// Pacote de conteúdo (BPAK): índice de arquivos por nome, com CRC32. Formato em
// docs/FORMATOS.md. Os dados ficam onde estão (sem cópia): o pacote precisa
// estar alinhado a 64 bytes, e cada arquivo começa alinhado a 64.
#pragma once
#include "nucleo/tipos.h"

class Pacote {
public:
    static const u32 VERSAO = 1;
    // Devolve false e preenche `erro` se o pacote for inválido.
    bool abrir(const u8* dados, u32 tamanho, char* erro, int n_erro);
    const u8* arquivo(const char* nome, u32* tamanho = nullptr) const;
    // Confere o CRC32 de todos os arquivos. Devolve quantos falharam.
    int verificar(char* primeiro_erro, int n_erro) const;
    u32 n() const { return n_; }
    u32 versao_conteudo() const { return versao_conteudo_; }
    u32 tamanho() const { return tamanho_; }
    const char* nome(u32 i) const;
private:
    struct Entrada { char nome[44]; u32 offset, tamanho, crc; char tipo[4]; u32 _; };
    const u8* dados_ = nullptr;
    const Entrada* entradas_ = nullptr;
    u32 n_ = 0, tamanho_ = 0, versao_conteudo_ = 0;
};
