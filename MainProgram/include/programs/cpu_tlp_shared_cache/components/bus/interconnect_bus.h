#pragma once
#include <vector>
#include <functional>
#include <cstdint>
#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h" // BusCmd, LineData, SnoopReq/SnoopResp, OFFSET_BITS

namespace cpu_tlp { struct RAMConnection; }

// ============================================================
// Señales L1 → Interconnect
// ============================================================
struct MasterToBus {
    bool     B_REQ{ false };           // solicita bus
    BusCmd   B_CMD{ BusCmd::BusRd };   // 000/001/010/011
    uint64_t B_ADDR{ 0 };              // dirección alineada (bits [OFFSET_BITS-1:0]=0)
    LineData B_WDATA{};              // datos (WriteBack)
    bool     B_WVALID{ false };        // writeback válido
};

// ============================================================
// Señales Interconnect → L1
// ============================================================
struct BusToMaster {
    bool     B_GRANT{ false };         // el bus le otorgó el turno
    bool     B_SHARED_SEEN{ false };   // algún otro tiene copia (S/E/M)
    bool     B_HITM_SEEN{ false };     // algún otro tenía M (dirty)
    bool     B_RVALID{ false };        // datos válidos
    LineData B_RDATA{};              // línea leída
    bool     B_DONE{ false };          // fin de transacción
    bool     B_WREADY{ true };         // listo para aceptar WDATA (opcional)
};

// ============================================================
// Interconnect con soporte de DRAM (RAMConnection) y coherencia MESI
// ============================================================
class Interconnect {
public:
    explicit Interconnect(int num_l1)
        : m2b_(num_l1), b2m_(num_l1), rr_ptr_(0) {
        sn_cb_.resize(num_l1);
    }

    // Puertos por L1 (para conectar en attachBus)
    MasterToBus* portM2B(int id) { return &m2b_[id]; }
    BusToMaster* portB2M(int id) { return &b2m_[id]; }

    // Callback de snoop (bus → L1.onSnoop)
    void attachSnoopCallback(int id,
        std::function<SnoopResp(const SnoopReq&)> cb) {
        sn_cb_[id] = std::move(cb);
    }

    // Enlazar el backend de DRAM (canal atómico del SharedMemoryComponent)
    void bindRAM(cpu_tlp::RAMConnection* ram) { ram_ = ram; }

    // Avanza un ciclo de bus
    void tick();

private:
    struct ActiveTx {
        bool     busy{ false };
        int      owner{ -1 };
        BusCmd   cmd{ BusCmd::BusRd };
        uint64_t addr_line{ 0 };

        bool     seen_shared{ false };
        bool     seen_hitm{ false };
        int      m_owner{ -1 };

        int      inv_acks_needed{ 0 };
        int      inv_acks_got{ 0 };

        bool     have_rdata{ false };
        LineData rdata{};      // buffer línea leída (C2C o DRAM)
        LineData wb_line{};    // buffer para WriteBack → DRAM

        // FSM de memoria (DRAM)
        enum class MemPhase { None, ReadReq, ReadWait, WriteReq, WriteWait } mem{ MemPhase::None };
        int      seg{ 0 };       // beat 0..3 (cada beat = 8 bytes)

        void clear() { *this = ActiveTx{}; }
    };

    std::vector<MasterToBus>  m2b_;
    std::vector<BusToMaster>  b2m_;
    std::vector<std::function<SnoopResp(const SnoopReq&)>> sn_cb_;

    int rr_ptr_{ 0 };
    ActiveTx tx_{};

    // Backend DRAM (atomics del SharedMemory)
    cpu_tlp::RAMConnection* ram_{ nullptr };

    // --- Utilidades internas ---
    int  pickOwnerRR();
    void clearOutputs();

    // Mapear dirección de línea (alineada) a índice de palabra de 64b (4KiB → 512 words)
    static uint16_t idx64(uint64_t addr_line);

    // DRAM adapters (línea de 32B en 4 beats de 64b)
    void startMemReadLine();
    bool stepMemReadLine();   // true cuando tx_.rdata está completa

    void startMemWriteLine();
    bool stepMemWriteLine();  // true cuando la línea se escribió completa
};
