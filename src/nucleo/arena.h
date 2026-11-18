// Alocador linear (arena). Sem alocação no laço: cada fase (boot, menu,
// partida) reserva o que precisa e libera tudo de uma vez (BRISA_ARCHITECTURE §3).
#pragma once
#include "tipos.h"

class Arena {
public:
    Arena() = default;
    Arena(void* memoria, size_t tamanho) { iniciar(memoria, tamanho); }
    void iniciar(void* memoria, size_t tamanho);
    // Devolve nullptr se não couber (nunca aborta). alinhamento: potência de 2.
    void* alocar(size_t tamanho, size_t alinhamento = 16);
    template <typename T> T* alocar_n(size_t n) { return static_cast<T*>(alocar(sizeof(T) * n, alignof(T) < 16 ? 16 : alignof(T))); }
    void limpar() { usado_ = 0; }
    size_t usado() const { return usado_; }
    size_t pico() const { return pico_; }
    size_t tamanho() const { return tamanho_; }
    size_t livre() const { return tamanho_ - usado_; }
    // Marca/volta: libera tudo o que foi alocado depois da marca.
    size_t marca() const { return usado_; }
    void voltar(size_t m) { if (m <= usado_) usado_ = m; }
private:
    u8* base_ = nullptr;
    size_t tamanho_ = 0;
    size_t usado_ = 0;
    size_t pico_ = 0;
};
