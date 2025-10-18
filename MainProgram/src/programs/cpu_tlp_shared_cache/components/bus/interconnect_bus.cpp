#include "programs/cpu_tlp_shared_cache/components/bus/interconnect_bus.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h"

static inline uint64_t align_to_line(uint64_t a) {
  constexpr uint64_t MASK = ~((1ULL << OFFSET_BITS) - 1ULL);
  return a & MASK;
}

void Interconnect::clearOutputs() {
  for (auto& o : b2m_) {
    o.B_SHARED_SEEN = false;
    o.B_HITM_SEEN   = false;
    o.B_RVALID      = false;
    o.B_DONE        = false;
    o.B_WREADY      = true;
    // B_GRANT se mantiene alto si tx_.busy (lo hacemos aparte)
  }
}

int Interconnect::pickOwnerRR() {
  const int N = (int)m2b_.size();
  for (int i = 0; i < N; ++i) {
    int k = (rr_ptr_ + i) % N;
    if (m2b_[k].B_REQ) {
      rr_ptr_ = (k + 1) % N;
      return k;
    }
  }
  return -1;
}

void Interconnect::tick() {
  clearOutputs();

  // Mantener GRANT alto mientras haya transacción activa
  if (tx_.busy && tx_.owner >= 0)
    b2m_[tx_.owner].B_GRANT = true;

  // Si no hay transacción activa, buscar nueva
  if (!tx_.busy) {
    int owner = pickOwnerRR();
    if (owner < 0) return; // nadie pidió

    // Iniciar transacción
    tx_.busy      = true;
    tx_.owner     = owner;
    tx_.cmd       = m2b_[owner].B_CMD;
    tx_.addr_line = align_to_line(m2b_[owner].B_ADDR);
    tx_.seen_shared = false;
    tx_.seen_hitm   = false;
    tx_.m_owner     = -1;
    tx_.have_rdata  = false;
    tx_.inv_acks_needed = 0;
    tx_.inv_acks_got    = 0;

    b2m_[owner].B_GRANT = true; // primer GRANT inmediato

    // ===== Fase 1: difusión Snoop =====
    for (int id = 0; id < (int)m2b_.size(); ++id) {
      if (id == owner) continue;
      if (!sn_cb_[id]) continue;
        SnoopReq s;
        s.cmd        = tx_.cmd;
        s.addr_line  = tx_.addr_line;
        s.grant_data = false;
        s.from_self  = false;
      SnoopResp r = sn_cb_[id](s);

      tx_.seen_shared |= r.has_shared;
      if (r.has_mod) {
        tx_.seen_hitm = true;
        if (tx_.m_owner < 0) tx_.m_owner = id;
      }

      if (tx_.cmd == BusCmd::BusUpgr) {
        if (r.has_shared && !r.has_mod) tx_.inv_acks_needed++;
        if (r.inv_ack)                  tx_.inv_acks_got++;
      }
    }

    // Publicar flags al solicitante
    b2m_[owner].B_SHARED_SEEN = tx_.seen_shared;
    b2m_[owner].B_HITM_SEEN   = tx_.seen_hitm;

    // ===== Fase 2: ejecución según comando =====
    switch (tx_.cmd) {

      // ----------------------------
      case BusCmd::BusRd:
      case BusCmd::BusRdX: {
        if (tx_.seen_hitm && tx_.m_owner >= 0) {
          // Cache-to-cache: pedir datos al dueño M
        SnoopReq s2;
        s2.cmd        = tx_.cmd;
        s2.addr_line  = tx_.addr_line;
        s2.grant_data = true;
        s2.from_self  = false;

          SnoopResp r2 = sn_cb_[tx_.m_owner](s2);

          if (r2.rvalid) {
            tx_.have_rdata = true;
            tx_.rdata      = r2.rdata;
            b2m_[owner].B_RVALID = true;
            b2m_[owner].B_RDATA  = r2.rdata;
          }

          // Cierre inmediato (no DRAM)
          b2m_[owner].B_DONE = true;
          tx_.busy = false;
          return;
        } else {
          // No hay M → espera DRAM (no B_DONE)
          return;
        }
      }

      // ----------------------------
      case BusCmd::BusUpgr: {
        // Esperar a que todos los S/E invaliden
        if (tx_.inv_acks_got >= tx_.inv_acks_needed) {
          b2m_[owner].B_DONE = true;
          tx_.busy = false;
        } else {
          // Si se quisiera modelar varios ticks: mantener busy
          b2m_[owner].B_DONE = true; // en esta versión, todos ackean en fase 1
          tx_.busy = false;
        }
        return;
      }

      // ----------------------------
      case BusCmd::WriteBack: {
        // Aceptar y descartar (sin DRAM)
        b2m_[owner].B_DONE = true;
        tx_.busy = false;
        return;
      }
    }
  }
}
