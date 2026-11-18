#include "controle.h"

f32 Controle::eixo(u8 v, f32 zona_morta) {
    f32 x = (static_cast<f32>(v) - 127.5f) / 127.5f;
    f32 a = x < 0 ? -x : x;
    if (a <= zona_morta) return 0.0f;
    f32 s = (a - zona_morta) / (1.0f - zona_morta);
    if (s > 1.0f) s = 1.0f;
    return x < 0 ? -s : s;
}
