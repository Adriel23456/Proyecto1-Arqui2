#pragma once
#include <vector>
#include <functional>
#include <cstdint>
#include "cash/l1_cash.h"   // BusCmd, LineData, SnoopReq/SnoopResp, OFFSET_BITS

// ============================================================
// Señales L1 → Interconnect
// ============================================================
struct MasterToBus {
  bool     B_REQ{false};           // solicita bus
  BusCmd   B_CMD{BusCmd::BusRd};   // 000/001/010/011
  uint64_t B_ADDR{0};              // dirección alineada (bits [OFFSET_BITS-1:0]=0)
  LineData B_WDATA{};              // datos (WriteBack)
  bool     B_WVALID{false};        // writeback válido
};

// ============================================================
// Señales Interconnect → L1
// ============================================================
struct BusToMaster {
  bool     B_GRANT{false};         // el bus le otorgó el turno
  bool     B_SHARED_SEEN{false};   // algún otro tiene copia (S/E/M)
  bool     B_HITM_SEEN{false};     // algún otro tenía M (dirty)
  bool     B_RVALID{false};        // datos válidos
  LineData B_RDATA{};              // línea leída
  bool     B_DONE{false};          // fin de transacción
  bool     B_WREADY{true};         // listo para aceptar WDATA (opcional)
};

// ============================================================
// Interconnect (sin DRAM, coherente con FSM MESI)
// ============================================================
class Interconnect {
public:
  explicit Interconnect(int num_l1)
    : m2b_(num_l1), b2m_(num_l1), rr_ptr_(0) {
      sn_cb_.resize(num_l1);
    }

  // Puertos por L1 (para conectar en attachBus)
  MasterToBus* portM2B(int id)  { return &m2b_[id]; }
  BusToMaster* portB2M(int id)  { return &b2m_[id]; }

  // Callback de snoop (bus → L1.onSnoop)
  void attachSnoopCallback(int id,
      std::function<SnoopResp(const SnoopReq&)> cb) {
    sn_cb_[id] = std::move(cb);
  }

  // Avanza un ciclo de bus
  void tick();

private:
  struct ActiveTx {
    bool     busy{false};
    int      owner{-1};
    BusCmd   cmd{BusCmd::BusRd};
    uint64_t addr_line{0};

    bool     seen_shared{false};
    bool     seen_hitm{false};
    int      m_owner{-1};

    int      inv_acks_needed{0};
    int      inv_acks_got{0};

    bool     have_rdata{false};
    LineData rdata{};

    void clear() { *this = ActiveTx{}; }
  };

  std::vector<MasterToBus>  m2b_;
  std::vector<BusToMaster>  b2m_;
  std::vector<std::function<SnoopResp(const SnoopReq&)>> sn_cb_;

  int rr_ptr_;
  ActiveTx tx_;

  int  pickOwnerRR();
  void clearOutputs();
};
