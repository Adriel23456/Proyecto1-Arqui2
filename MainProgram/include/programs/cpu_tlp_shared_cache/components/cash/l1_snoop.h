#pragma once
#include "l1_cash.h"
#include <array>
#include <cstdint>

// ============================================================
// Configuración base del sistema (debe coincidir con la de L1)
// ============================================================
using LineData = std::array<uint8_t, LINE_BYTES>;

// ============================================================
// Enumeraciones (se replican aquí para evitar include circular)
// ============================================================
enum class BusCmd : uint8_t {
  BusRd   = 0b000,  // lectura compartible
  BusRdX  = 0b001,  // lectura exclusiva (RFO)
  BusUpgr = 0b010,  // upgrade S→M
  WriteBack = 0b011 // writeback (volcado a memoria)
};

// ============================================================
// Estructuras de snoop (bus ↔ L1)
// ============================================================

// Broadcast desde el bus hacia cada L1
struct SnoopReq {
  BusCmd   cmd;           // comando difundido
  uint64_t addr_line;     // dirección alineada (bits [OFFSET_BITS-1:0]=0)
  bool     grant_data;    // esta L1 fue elegida para poner datos
  bool     from_self;     // true si proviene de la misma L1 (ignorar)
};

// Respuesta de la L1 hacia el bus
struct SnoopResp {
  bool      has_shared{false}; // L1 tiene copia (S/E/M)
  bool      has_mod{false};    // L1 tiene M (dirty)
  bool      inv_ack{false};    // invalidó (ack)
  bool      rvalid{false};     // hay datos válidos en rdata
  LineData  rdata{};           // línea de datos (si grant_data)
};

// ============================================================
// Nota de uso:
//
// - El interconnect combina (OR) los flags de todos los SnoopResp:
//   * OR(has_shared) → B_SHARED_SEEN
//   * OR(has_mod)    → B_HITM_SEEN
//
// - Si existe un has_mod=1, el bus identifica a esa L1 como “dueño sucio”
//   y la vuelve a llamar con grant_data=1 para obtener SNP_RDATA.
//
// - Los inv_ack se usan para cerrar BusUpgr (invalida S/E en otros PE).
// ============================================================
