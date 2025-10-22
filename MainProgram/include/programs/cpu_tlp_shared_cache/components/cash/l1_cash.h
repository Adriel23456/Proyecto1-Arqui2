#ifndef L1_CACHE_H
#define L1_CACHE_H

#include <array>
#include <cstdint>
#include <cstddef>
#include <mutex>

// ============================================================
// Parámetros del MVP
// ============================================================
constexpr std::size_t LINE_BYTES = 32;
constexpr std::size_t WAYS = 2;
constexpr std::size_t SETS = 8;

static_assert((SETS& (SETS - 1)) == 0, "SETS debe ser potencia de 2");

constexpr int ilog2(std::size_t n) { return (n <= 1) ? 0 : 1 + ilog2(n >> 1); }
constexpr int OFFSET_BITS = ilog2(LINE_BYTES);
constexpr int INDEX_BITS = ilog2(SETS);
constexpr int TAG_BITS = 64 - OFFSET_BITS - INDEX_BITS;

// Sentinela para "tag inválido" (56 bits en 1)
static constexpr uint64_t INVALID_TAG_SENTINEL = 0x00FFFFFFFFFFFFFFULL;

// ============================================================
// MESI y Bus
// ============================================================
enum class Mesi : uint8_t { I = 0, S = 1, E = 2, M = 3 };

// Canal snoop
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h"

// Forward-decl de puertos del bus
struct MasterToBus;
struct BusToMaster;

// ============================================================
// Estructuras básicas de la caché
// ============================================================
struct CacheLine {
    uint64_t  tag = INVALID_TAG_SENTINEL;
    Mesi      state = Mesi::I;
    LineData  data{};
    bool      valid = false;
};

struct CacheSet {
    std::array<CacheLine, WAYS> ways{};
    uint8_t lru = 0;  // 0 => way0 MRU, 1 => way1 MRU (política simple)
};

struct AddrParts {
    uint64_t tag;
    uint32_t set;
    uint32_t off;
    uint8_t  dw_in_line;
    uint8_t  byte_in_line;
};

// ============================================================
// Interfaz CPU ↔ L1
// ============================================================
struct CpuReq {
    bool     C_REQUEST_M{ false };
    bool     C_WE_M{ false };
    bool     C_ISB_M{ false };
    uint64_t ALUOut_M{ 0 };
    uint64_t RD_Rm_Special_M{ 0 };
    bool     C_READY_ACK{ false };
};

struct CpuResp {
    bool     C_READY{ false };
    uint64_t RD_C_out{ 0 };
};

// ============================================================
// FSM de la L1
// ============================================================
enum class L1State : uint8_t {
    IDLE, LOOKUP, MISS, WAIT_ACK,
    REQ_BUS, WAIT_GRANT, WAIT_DATA, FILL
};

// ============================================================
// Estructura para exportar info de línea (thread-safe)
// ============================================================
struct CacheLineInfo {
    uint64_t tag;
    Mesi state;
    std::array<uint8_t, 32> data;
    bool valid;
};

// ============================================================
// Clase principal L1Cache
// ============================================================
class L1Cache {
public:
    L1Cache();

    mutable std::mutex mtx_;

    void reset();
    void beginAccess(const CpuReq& req);
    void tick();
    CpuResp output() const { return out_; }

    void attachBus(MasterToBus* out_port, BusToMaster* in_port, int my_id) {
        pm_out_ = out_port; pm_in_ = in_port; l1_id_ = my_id;
    }

    SnoopResp onSnoop(const SnoopReq& s);

    // ===== Método thread-safe ya existente =====
    CacheLineInfo getLineInfo(uint32_t set, uint32_t way) const {
        std::lock_guard<std::mutex> lk(mtx_);
        if (set >= SETS || way >= WAYS) {
            CacheLineInfo info;
            info.tag = 0;
            info.state = Mesi::I;
            info.data.fill(0);
            info.valid = false;
            return info;
        }
        const auto& line = sets_[set].ways[way];
        CacheLineInfo info;
        info.tag = line.tag;
        info.state = line.state;
        info.data = line.data;
        info.valid = line.valid;
        return info;
    }

    static constexpr int offsetBits() { return OFFSET_BITS; }
    static constexpr int indexBits() { return INDEX_BITS; }
    static constexpr int tagBits() { return TAG_BITS; }

private:
    friend AddrParts splitAddress(uint64_t addr);
    int  findWay(uint32_t set, uint64_t tag) const;
    void logSignals_();

    bool pe_req_block_ = false;

    std::array<CacheSet, SETS> sets_{};
    L1State  fsm_{ L1State::IDLE };
    CpuReq   in_{};
    CpuResp  out_{};

    MasterToBus* pm_out_{ nullptr };
    BusToMaster* pm_in_{ nullptr };
    int          l1_id_{ -1 };

    bool prev_req_{ false };

    // --- Debug log snapshots (por instancia) ---
    L1State dbg_prev_fsm_{ L1State::IDLE };
    bool    dbg_prev_B_REQ_{ false };
    bool    dbg_prev_B_GRANT_{ false };
    bool    dbg_prev_B_RVALID_{ false };
    bool    dbg_prev_B_DONE_{ false };

    struct PendingTx {
        BusCmd   cmd{ BusCmd::BusRd };
        uint64_t req_addr_line{ 0 };
        uint32_t set{ 0 };
        int      victim{ -1 };

        bool     need_wb{ false };
        uint64_t victim_addr_line{ 0 };
        LineData wb_line{};

        bool     saw_shared{ false };
        bool     saw_hitm{ false };

        // Congelamos offsets del acceso que provocó el miss
        uint8_t  byte_in_line{ 0 };
        uint8_t  dw_in_line{ 0 };

        // ====================================================================
        // CRITICAL FIX: Track si esta transacción está activa
        // ====================================================================
        // Esto permite que onSnoop responda correctamente incluso cuando
        // la línea todavía no es válida (en estados WAIT_DATA/FILL)
        bool     is_active{ false };

        // Estado que tendrá la línea cuando complete (para responder snoops)
        Mesi     target_state{ Mesi::I };
    } pend_;

    LineData temp_fill_{};
};

#endif // L1_CACHE_H