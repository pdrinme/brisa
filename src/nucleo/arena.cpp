#include "arena.h"

void Arena::iniciar(void* memoria, size_t tamanho) {
    base_ = static_cast<u8*>(memoria);
    tamanho_ = memoria ? tamanho : 0;
    usado_ = 0;
    pico_ = 0;
}

void* Arena::alocar(size_t tamanho, size_t alinhamento) {
    if (!base_ || alinhamento == 0 || (alinhamento & (alinhamento - 1)) != 0) return nullptr;
    uintptr_t atual = reinterpret_cast<uintptr_t>(base_) + usado_;
    uintptr_t alinhado = (atual + (alinhamento - 1)) & ~static_cast<uintptr_t>(alinhamento - 1);
    size_t inicio = static_cast<size_t>(alinhado - reinterpret_cast<uintptr_t>(base_));
    if (inicio > tamanho_ || tamanho > tamanho_ - inicio) return nullptr;
    usado_ = inicio + tamanho;
    if (usado_ > pico_) pico_ = usado_;
    return base_ + inicio;
}
