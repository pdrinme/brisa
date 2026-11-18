// Animações (BANI): clipes amostrados a 30 Hz, um registro por nó por quadro
// (quaternion int16 e translação em milímetros). O banco do esqueleto padrão é
// compartilhado por todos os trajes (a assinatura dos nós precisa bater).
#pragma once
#include "modelo.h"

class BancoAnimacoes {
public:
    static const u32 VERSAO = 1;
    bool carregar(const u8* dados, u32 tamanho, char* erro, int n_erro);
    int n_clipes() const { return n_clipes_; }
    int achar(const char* nome) const;
    const char* nome(int clipe) const;
    bool loop(int clipe) const;
    f32 duracao(int clipe) const;  // segundos
    u32 assinatura() const { return assinatura_; }
    u32 n_nos() const { return n_nos_; }
    u32 fps() const { return fps_; }
    // Monta a pose do clipe no tempo t (s): interpola entre quadros (nlerp na
    // rotação), usa a escala de repouso do modelo e compõe a hierarquia.
    // Fora de um clipe em laço, t é preso ao último quadro.
    void amostrar(int clipe, f32 t, const ModeloBrisa& m, Pose& p) const;
private:
    struct Clipe { char nome[24]; u32 n_quadros, loop, offset, n_nos; u32 _[2]; };
    struct Amostra { i16 q[4]; i16 t[3]; i16 _; };
    const u8* dados_ = nullptr;
    const Clipe* clipes_ = nullptr;
    int n_clipes_ = 0;
    u32 n_nos_ = 0, fps_ = 30, assinatura_ = 0, tamanho_ = 0;
};
