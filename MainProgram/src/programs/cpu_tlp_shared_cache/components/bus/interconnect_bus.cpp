#include "programs/cpu_tlp_shared_cache/components/bus/interconnect_bus.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h"
#include "programs/cpu_tlp_shared_cache/components/SharedData.h" // cpu_tlp::RAMConnection
#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"

// --- helpers de dirección ---
static inline uint64_t align_to_line(uint64_t a) {
    constexpr uint64_t MASK = ~((1ULL << OFFSET_BITS) - 1ULL);
    return a & MASK;
}

uint16_t Interconnect::idx64(uint64_t addr_line) {
    // 4 KiB de ventana => devolver base EN BYTES (no /8)
    return static_cast<uint16_t>(addr_line & 0xFFFULL);
}

// --- limpieza de salidas por ciclo ---
void Interconnect::clearOutputs() {
    const bool can_accept_wb = !(tx_.busy && tx_.mem != ActiveTx::MemPhase::None);
    for (auto& o : b2m_) {
        o.B_GRANT.store(false, std::memory_order_relaxed);
        o.B_SHARED_SEEN.store(false, std::memory_order_relaxed);
        o.B_HITM_SEEN.store(false, std::memory_order_relaxed);
        // NO limpiar aquí: RVALID/DONE las baja la L1 con exchange(false)
        o.B_WREADY.store(can_accept_wb, std::memory_order_relaxed);
    }
}

// --- RR arbiter ---
int Interconnect::pickOwnerRR() {
    const int N = (int)m2b_.size();
    for (int i = 0; i < N; ++i) {
        int k = (rr_ptr_ + i) % N;
        if (m2b_[k].B_REQ.load(std::memory_order_acquire)) { // ver payloads previos
            rr_ptr_ = (k + 1) % N;
            return k;
        }
    }
    return -1;
}

// --- DRAM: iniciar lectura de línea (4 beats de 64b) ---
void Interconnect::startMemReadLine() {
    tx_.mem = ActiveTx::MemPhase::ReadReq;
    tx_.seg = 0;
}

// Devuelve true cuando ya está completa en tx_.rdata
bool Interconnect::stepMemReadLine() {
    if (!ram_) { // fallback: responde ceros para no colgarse
        if (tx_.seg == 0) {
            tx_.rdata.fill(0);
            tx_.seg = 4;
        }
        tx_.mem = ActiveTx::MemPhase::None;
        return true;
    }

    if (tx_.mem == ActiveTx::MemPhase::ReadReq) {
        auto& R = *ram_;
        if (!R.request_active.load(std::memory_order_acquire)) {
            R.write_enable.store(false, std::memory_order_release);
            // EN BYTES: base + seg*8
            R.request_address.store(static_cast<uint16_t>(idx64(tx_.addr_line) + tx_.seg * 8),
                std::memory_order_release);
            R.request_active.store(true, std::memory_order_release);
            tx_.mem = ActiveTx::MemPhase::ReadWait;
        }
        return false;
    }

    if (tx_.mem == ActiveTx::MemPhase::ReadWait) {
        auto& R = *ram_;
        if (R.response_ready.load(std::memory_order_acquire)) {
            uint64_t w = R.read_data.load(std::memory_order_acquire);

            // Copiar 8 bytes little-endian al buffer de línea
            const int off = tx_.seg * 8;
            for (int i = 0; i < 8; ++i)
                tx_.rdata[off + i] = static_cast<uint8_t>((w >> (i * 8)) & 0xFF);

            // limpiar flags del canal
            R.response_ready.store(false, std::memory_order_release);
            R.request_active.store(false, std::memory_order_release);

            // siguiente beat
            tx_.seg++;
            tx_.mem = (tx_.seg < 4) ? ActiveTx::MemPhase::ReadReq
                : ActiveTx::MemPhase::None;

            return (tx_.seg >= 4); // true = línea completa
        }
        return false;
    }

    return (tx_.seg >= 4);
}

// --- DRAM: iniciar escritura de línea (4 beats de 64b) ---
void Interconnect::startMemWriteLine() {
    tx_.mem = ActiveTx::MemPhase::WriteReq;
    tx_.seg = 0;
}

bool Interconnect::stepMemWriteLine() {
    if (!ram_) { // fallback: “aceptar y olvidar”
        tx_.seg = 4;
        tx_.mem = ActiveTx::MemPhase::None;
        return true;
    }

    if (tx_.mem == ActiveTx::MemPhase::WriteReq) {
        auto& R = *ram_;
        if (!R.request_active.load(std::memory_order_acquire)) {
            // Empacar 8 bytes desde wb_line (little-endian)
            uint64_t w = 0;
            const int off = tx_.seg * 8;
            for (int i = 0; i < 8; ++i)
                w |= (static_cast<uint64_t>(tx_.wb_line[off + i]) << (i * 8));

            R.write_data.store(w, std::memory_order_release);
            R.write_enable.store(true, std::memory_order_release);
            // EN BYTES: base + seg*8
            R.request_address.store(static_cast<uint16_t>(idx64(tx_.addr_line) + tx_.seg * 8),
                std::memory_order_release);
            R.request_active.store(true, std::memory_order_release);

            tx_.mem = ActiveTx::MemPhase::WriteWait;
        }
        return false;
    }

    if (tx_.mem == ActiveTx::MemPhase::WriteWait) {
        auto& R = *ram_;
        if (R.response_ready.load(std::memory_order_acquire)) {
            // limpiar flags del canal
            R.response_ready.store(false, std::memory_order_release);
            R.request_active.store(false, std::memory_order_release);

            // siguiente beat
            tx_.seg++;
            tx_.mem = (tx_.seg < 4) ? ActiveTx::MemPhase::WriteReq
                : ActiveTx::MemPhase::None;

            return (tx_.seg >= 4); // true = línea escrita completa
        }
        return false;
    }

    return (tx_.seg >= 4);
}

// --- ciclo principal ---
void Interconnect::tick() {
    clearOutputs();

    // Mantener GRANT alto mientras haya transacción activa
    if (tx_.busy && tx_.owner >= 0)
        b2m_[tx_.owner].B_GRANT.store(true, std::memory_order_relaxed);

    // 0) Avanzar DRAM si está en curso (cede el tick)
    if (tx_.busy && tx_.mem != ActiveTx::MemPhase::None) {
        bool mem_done = false;

        switch (tx_.mem) {
        case ActiveTx::MemPhase::ReadReq:
        case ActiveTx::MemPhase::ReadWait: {
            mem_done = stepMemReadLine();
            if (mem_done) {
                // Publicar datos al solicitante y cerrar (payload → flags)
                b2m_[tx_.owner].B_RDATA = tx_.rdata;
                b2m_[tx_.owner].B_RVALID.store(true, std::memory_order_release);
                b2m_[tx_.owner].B_DONE.store(true, std::memory_order_release);
                tx_.busy = false;
            }
            break;
        }
        case ActiveTx::MemPhase::WriteReq:
        case ActiveTx::MemPhase::WriteWait: {
            mem_done = stepMemWriteLine();
            if (mem_done) {
                b2m_[tx_.owner].B_DONE.store(true, std::memory_order_release);
                tx_.busy = false;
            }
            break;
        }
        default: break;
        }

        if (tx_.busy && tx_.owner >= 0)
            b2m_[tx_.owner].B_GRANT.store(true, std::memory_order_relaxed);
        return; // este tick fue para DRAM
    }

    // 1) Si no hay transacción activa, buscar una nueva
    if (!tx_.busy) {
        int owner = pickOwnerRR();
        if (owner < 0) return; // nadie pidió

        // Iniciar transacción
        tx_.busy = true;
        tx_.owner = owner;
        tx_.cmd = m2b_[owner].B_CMD;
        tx_.addr_line = align_to_line(m2b_[owner].B_ADDR);
        tx_.seen_shared = false;
        tx_.seen_hitm = false;
        tx_.m_owner = -1;
        tx_.have_rdata = false;
        tx_.inv_acks_needed = 0;
        tx_.inv_acks_got = 0;
        tx_.mem = ActiveTx::MemPhase::None;
        tx_.seg = 0;

        // primer GRANT inmediato
        b2m_[owner].B_GRANT.store(true, std::memory_order_release);

        // ===== Fase 1: difusión Snoop =====
        for (int id = 0; id < (int)m2b_.size(); ++id) {
            if (id == owner) continue;
            if (!sn_cb_[id]) continue;

            SnoopReq s;
            s.cmd = tx_.cmd;
            s.addr_line = tx_.addr_line;
            s.grant_data = false;
            s.from_self = false;

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

        // Publicar flags al solicitante (efímeros)
        b2m_[owner].B_SHARED_SEEN.store(tx_.seen_shared, std::memory_order_relaxed);
        b2m_[owner].B_HITM_SEEN.store(tx_.seen_hitm, std::memory_order_relaxed);

        // ===== Fase 2: ejecución según comando =====
        switch (tx_.cmd) {

        case BusCmd::BusRd:
        case BusCmd::BusRdX: {
            if (tx_.seen_hitm && tx_.m_owner >= 0) {
                // ---- Cache-to-cache: pedir datos al dueño en M ----
                SnoopReq s2;
                s2.cmd = tx_.cmd;
                s2.addr_line = tx_.addr_line;
                s2.grant_data = true;
                s2.from_self = false;

                SnoopResp r2 = sn_cb_[tx_.m_owner](s2);
                if (r2.rvalid) {
                    tx_.have_rdata = true;
                    tx_.rdata = r2.rdata;
                    // payload → flags
                    b2m_[owner].B_RDATA = r2.rdata;
                    b2m_[owner].B_RVALID.store(true, std::memory_order_release);
                }

                // Cierre inmediato (no DRAM)
                b2m_[owner].B_DONE.store(true, std::memory_order_release);
                tx_.busy = false;
                return;

            }
            else {
                // ---- No hay M: ir a DRAM (lectura en 4 beats) ----
                startMemReadLine();
                // Mantener GRANT hasta cerrar
                b2m_[owner].B_GRANT.store(true, std::memory_order_relaxed);
                return;
            }
        }

        case BusCmd::BusUpgr: {
            // Invalidaciones ackean en la misma fase
            b2m_[owner].B_DONE.store(true, std::memory_order_release);
            tx_.busy = false;
            return;
        }

        case BusCmd::WriteBack: {
            // Escribir línea sucia a DRAM (4 beats)
            tx_.wb_line = m2b_[owner].B_WDATA; // línea completa de 32B
            startMemWriteLine();
            b2m_[owner].B_GRANT.store(true, std::memory_order_relaxed);
            return;
        }
        } // switch
    }
}
