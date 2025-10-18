#include "programs/cpu_tlp_shared_cache/components/cash/l1_utils.h"
#include <cstring>

uint64_t read64_in_line(const CacheLine& line, uint8_t dw) {
  uint64_t v;
  std::memcpy(&v, &line.data[dw * 8], 8);
  return v;
}

uint8_t read8_in_line(const CacheLine& line, uint8_t b) {
  return line.data[b];
}

void write64_in_line(CacheLine& line, uint8_t dw, uint64_t v) {
  std::memcpy(&line.data[dw * 8], &v, 8);
}

void write8_in_line(CacheLine& line, uint8_t b, uint8_t v) {
  line.data[b] = v;
}

AddrParts splitAddress(uint64_t addr) {
  constexpr uint64_t OFF_MASK = (1ULL << OFFSET_BITS) - 1ULL;
  constexpr uint64_t IDX_MASK = ((1ULL << INDEX_BITS) - 1ULL) << OFFSET_BITS;
  const uint64_t off = addr & OFF_MASK;
  const uint64_t set = (addr & IDX_MASK) >> OFFSET_BITS;
  const uint64_t tag = addr >> (OFFSET_BITS + INDEX_BITS);
  return AddrParts{
    tag,
    static_cast<uint32_t>(set),
    static_cast<uint32_t>(off),
    static_cast<uint8_t>(off >> 3),
    static_cast<uint8_t>(off & 0x1F)
  };
}
