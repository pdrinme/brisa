// Acumulador de passo fixo (BRISA_ARCHITECTURE §4): a simulação roda a 30 Hz,
// no máximo 2 passos por quadro; se ficar para trás, descarta o excesso
// (o jogo desacelera em vez de entrar em "espiral da morte").
#pragma once
#include "tipos.h"

class PassoFixo {
public:
    explicit PassoFixo(u32 hz = 30, u32 max_passos = 2) : passo_us_(1000000u / hz), max_(max_passos) {}
    // Soma o tempo do quadro e devolve quantos passos simular agora.
    u32 avancar(u32 dt_us);
    u32 passo_us() const { return passo_us_; }
    u64 descartado_us() const { return descartado_; }
    u64 passos_total() const { return total_; }
private:
    u32 passo_us_;
    u32 max_;
    u64 acumulado_ = 0;
    u64 descartado_ = 0;
    u64 total_ = 0;
};
