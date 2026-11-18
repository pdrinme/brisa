#pragma once
#include "tipos.h"
// CRC-32 (IEEE 802.3, o mesmo do zlib.crc32 usado pelas ferramentas).
u32 crc32(const void* dados, size_t n, u32 inicial = 0);
