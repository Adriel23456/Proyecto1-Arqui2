#ifndef L1_CACHE_H
#define L1_CACHE_H

#include <array>
#include <cstdint>
#include <cstddef>

// ============================================================
// Parámetros del MVP (como ya tenías)
// ============================================================
constexpr std::size_t LINE_BYTES = 32;  // 32 bytes por línea
constexpr std::size_t WAYS       = 2;   // 2-way set associative
constexpr std::size_t SETS       = 8;   // número de conjuntos

static_assert((SETS & (SETS - 1)) == 0, "SETS debe ser potencia de 2");

constexpr int ilog2(std::size_t n) { return (n <= 1) ? 0 : 1 + ilog2(n >> 1); }
constexpr int OFFSET_BITS = ilog2(LINE_BYTES);             // =5
constexpr int INDEX_BITS  = ilog2(SETS);                   // =3 si SETS=8
constexpr int TAG_BITS    = 64 - OFFSET_BITS - INDEX_BITS; // =56

// ============================================================
// MESI y Bus
// ============================================================
enum class Mesi   : uint8_t { I=0, S=1, E=2, M=3 };

// Canal snoop (definido en snoop_if.h)
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h" // BusCmd, SnoopReq, SnoopResp, LineData

// Forward-decl de puertos del bus (definidos en interconnect_bus.h)
struct MasterToBus;
struct BusToMaster;

// ============================================================
// Estructuras básicas de la caché
// ============================================================
using LineDataAlias = LineData; // alias local si lo prefieres

struct CacheLine {
  uint64_t  tag   = 0;
  Mesi      state = Mesi::I;
  LineData  data  {};
  bool      valid = false;
};

struct CacheSet {
  std::array<CacheLine, WAYS> ways{};
  uint8_t lru = 0; // 0 = way0 fue la más reciente
};

struct AddrParts {
  uint64_t tag;
  uint32_t set;
  uint32_t off;
  uint8_t  dw_in_line;   // palabra de 64 bits dentro de la línea (0..3)
  uint8_t  byte_in_line; // byte dentro de la línea (0..31)
};

// ============================================================
// Interfaz CPU ↔ L1
// ============================================================
struct CpuReq {
  bool     C_REQUEST_M{false};   // solicitud activa
  bool     C_WE_M{false};        // 0=read, 1=write
  bool     C_ISB_M{false};       // 1=byte, 0=64-bit
  uint64_t ALUOut_M{0};          // dirección
  uint64_t RD_Rm_Special_M{0};   // dato a escribir (si write)
  bool     C_READY_ACK{false};   // handshake del CPU
};

struct CpuResp {
  bool     C_READY{false};       // respuesta lista
  uint64_t RD_C_out{0};          // dato leído (si read)
};

// ============================================================
// FSM de la L1
// ============================================================
enum class L1State : uint8_t {
  IDLE, LOOKUP, MISS, WAIT_ACK,
  REQ_BUS, WAIT_GRANT, WAIT_DATA, FILL
};

// ============================================================
// Clase principal L1Cache
// ============================================================
class L1Cache {
public:
  L1Cache();

  // --- Interfaz principal ---
  void reset();
  void beginAccess(const CpuReq& req);
  void tick();
  CpuResp output() const { return out_; }

  // --- Cableado al bus ---
  void attachBus(MasterToBus* out_port, BusToMaster* in_port, int my_id) {
    pm_out_ = out_port; pm_in_ = in_port; l1_id_ = my_id;
  }

  // --- Coherencia (MESI) ---
  SnoopResp onSnoop(const SnoopReq& s);

  // --- Utilidades ---
  static constexpr int offsetBits() { return OFFSET_BITS; }
  static constexpr int indexBits()  { return INDEX_BITS;  }
  static constexpr int tagBits()    { return TAG_BITS;    }

private:
  // ---- Helpers públicos en .cpp ----
  friend AddrParts splitAddress(uint64_t addr);
  int findWay(uint32_t set, uint64_t tag) const;

  // ---- Estado interno ----
  std::array<CacheSet, SETS> sets_{};
  L1State  fsm_{L1State::IDLE};
  CpuReq   in_{};
  CpuResp  out_{};

  // ---- Puertos de bus ----
  MasterToBus* pm_out_{nullptr};  // L1 → Bus
  BusToMaster* pm_in_{nullptr};   // Bus → L1
  int          l1_id_{-1};

  // ---- Contexto de transacción pendiente (para MISS/Upgr/WB) ----
  struct PendingTx {
    BusCmd   cmd{BusCmd::BusRd};
    uint64_t req_addr_line{0};     // línea solicitada (alineada)
    uint32_t set{0};
    int      victim{-1};

    // WriteBack de víctima M (previo al fill)
    bool     need_wb{false};
    uint64_t victim_addr_line{0};
    LineData wb_line{};

    // Flags observados del bus
    bool     saw_shared{false};
    bool     saw_hitm{false};
  } pend_;

  LineData temp_fill_{}; // buffer de línea recibida para FILL
};

#endif // L1_CACHE_H
