#include "passo_fixo.h"

u32 PassoFixo::avancar(u32 dt_us) {
    acumulado_ += dt_us;
    u32 n = static_cast<u32>(acumulado_ / passo_us_);
    if (n > max_) {
        u64 excesso = acumulado_ - static_cast<u64>(max_) * passo_us_;
        // Guarda só a fração do próximo passo.
        u64 resto = excesso % passo_us_;
        descartado_ += excesso - resto;
        acumulado_ = static_cast<u64>(max_) * passo_us_ + resto;
        n = max_;
    }
    acumulado_ -= static_cast<u64>(n) * passo_us_;
    total_ += n;
    return n;
}
