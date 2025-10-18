#pragma once
#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"
#include <cstdint>

// Lectura/escritura dentro de línea
uint64_t read64_in_line(const CacheLine& line, uint8_t dw);
uint8_t  read8_in_line(const CacheLine& line, uint8_t b);
void     write64_in_line(CacheLine& line, uint8_t dw, uint64_t v);
void     write8_in_line(CacheLine& line, uint8_t b, uint8_t v);

// División de dirección en tag/set/offset
AddrParts splitAddress(uint64_t addr);
