// Tipos básicos do núcleo do Brisa (portátil: PS2 e PC).
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef _EE
// No EE, usa os tipos do PS2SDK para não conflitar com os cabeçalhos dele.
#include <tamtypes.h>
typedef s8 i8;
typedef s16 i16;
typedef s32 i32;
typedef s64 i64;
#else
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
#endif
typedef float f32;

static_assert(sizeof(u8) == 1 && sizeof(u16) == 2 && sizeof(u32) == 4 && sizeof(u64) == 8, "tamanhos dos tipos");
