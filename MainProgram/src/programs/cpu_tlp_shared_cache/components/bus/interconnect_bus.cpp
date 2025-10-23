#include "programs/cpu_tlp_shared_cache/components/bus/interconnect_bus.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h"
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"

#include <iostream>
#include <mutex>

namespace {
    constexpr bool DBG_BUS = true;
    std::mutex g_dbg_mtx;

#define BUSLOG(MSG_EXPR) do { \
        if (DBG_BUS) { \
            std::lock_guard<std::mutex> _lk(g_dbg_mtx); \
            std::cout << "[Bus] " << MSG_EXPR << std::endl; \
        } \
    } while(0)
}

static inline uint64_t align_to_line(uint64_t a) {
    constexpr uint64_t MASK = ~((1ULL << OFFSET_BITS) - 1ULL);
    return a & MASK;
}

uint16_t Interconnect::idx64(uint64_t addr_line) {
    return static_cast<uint16_t>(addr_line & 0x0FFFu);
}

void Interconnect::clearOutputs() {
    const bool can_accept_wb = !(tx_.busy && tx_.mem != ActiveTx::MemPhase::None);
    const int owner = (tx_.busy ? tx_.owner : -1);

    for (size_t i = 0; i < b2m_.size(); ++i) {
        if ((int)i == owner) {
            b2m_[i].B_GRANT.store(true, std::memory_order_relaxed);
        }
        else {
            b2m_[i].B_GRANT.store(false, std::memory_order_relaxed);
        }

        b2m_[i].B_WREADY.store(can_accept_wb, std::memory_order_relaxed);

        if ((int)i != owner) {
            b2m_[i].B_SHARED_SEEN.store(false, std::memory_order_relaxed);
            b2m_[i].B_HITM_SEEN.store(false, std::memory_order_relaxed);
        }
    }

    if (owner >= 0) {
        b2m_[owner].B_SHARED_SEEN.store(tx_.seen_shared, std::memory_order_relaxed);
        b2m_[owner].B_HITM_SEEN.store(tx_.seen_hitm, std::memory_order_relaxed);
    }
}

int Interconnect::pickOwnerRR() {
    const int N = (int)m2b_.size();
    for (int i = 0; i < N; ++i) {
        int k = (rr_ptr_ + i) % N;
        if (!req_edge_block_[k] && m2b_[k].B_REQ.load(std::memory_order_acquire)) {
            rr_ptr_ = (k + 1) % N;
            return k;
        }
    }
    return -1;
}

void Interconnect::startMemReadLine() {
    tx_.mem = ActiveTx::MemPhase::ReadReq;
    tx_.seg = 0;
}

bool Interconnect::stepMemReadLine() {
    if (!ram_) {
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

            const int off = tx_.seg * 8;
            for (int i = 0; i < 8; ++i)
                tx_.rdata[off + i] = static_cast<uint8_t>((w >> (i * 8)) & 0xFF);

            R.response_ready.store(false, std::memory_order_release);
            R.request_active.store(false, std::memory_order_release);

            tx_.seg++;
            tx_.mem = (tx_.seg < 4) ? ActiveTx::MemPhase::ReadReq
                : ActiveTx::MemPhase::None;

            return (tx_.seg >= 4);
        }
        return false;
    }

    return (tx_.seg >= 4);
}

void Interconnect::startMemWriteLine() {
    tx_.mem = ActiveTx::MemPhase::WriteReq;
    tx_.seg = 0;
}

bool Interconnect::stepMemWriteLine() {
    if (!ram_) {
        tx_.seg = 4;
        tx_.mem = ActiveTx::MemPhase::None;
        return true;
    }

    if (tx_.mem == ActiveTx::MemPhase::WriteReq) {
        auto& R = *ram_;
        if (!R.request_active.load(std::memory_order_acquire)) {
            uint64_t w = 0;
            const int off = tx_.seg * 8;
            for (int i = 0; i < 8; ++i)
                w |= (static_cast<uint64_t>(tx_.wb_line[off + i]) << (i * 8));

            R.write_data.store(w, std::memory_order_release);
            R.write_enable.store(true, std::memory_order_release);
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
            R.response_ready.store(false, std::memory_order_release);
            R.request_active.store(false, std::memory_order_release);

            tx_.seg++;
            tx_.mem = (tx_.seg < 4) ? ActiveTx::MemPhase::WriteReq
                : ActiveTx::MemPhase::None;

            return (tx_.seg >= 4);
        }
        return false;
    }

    return (tx_.seg >= 4);
}

void Interconnect::tick() {
    clearOutputs();

    if (tx_.busy && tx_.owner >= 0)
        b2m_[tx_.owner].B_GRANT.store(true, std::memory_order_relaxed);

    // Desbloquear masters que ya bajaron B_REQ (edge)
    for (size_t i = 0; i < m2b_.size(); ++i) {
        if (req_edge_block_[i] && !m2b_[i].B_REQ.load(std::memory_order_acquire)) {
            req_edge_block_[i] = false;
        }
    }

    // 0) Avanzar DRAM si está en curso
    if (tx_.busy && tx_.mem != ActiveTx::MemPhase::None) {
        bool mem_done = false;

        switch (tx_.mem) {
        case ActiveTx::MemPhase::ReadReq:
        case ActiveTx::MemPhase::ReadWait: {
            mem_done = stepMemReadLine();
            if (mem_done) {
                BUSLOG("DRAM READ done owner=" << tx_.owner
                    << " addr_line=0x" << std::hex << tx_.addr_line << std::dec);

                if (!tx_.signaled_rvalid) {
                    b2m_[tx_.owner].B_RDATA = tx_.rdata;
                    b2m_[tx_.owner].B_RVALID.store(true, std::memory_order_release);
                    tx_.signaled_rvalid = true;
                }
                if (!tx_.signaled_done) {
                    b2m_[tx_.owner].B_DONE.store(true, std::memory_order_release);
                    tx_.signaled_done = true;
                }

                tx_.mem = ActiveTx::MemPhase::None;
            }
            break;
        }
        case ActiveTx::MemPhase::WriteReq:
        case ActiveTx::MemPhase::WriteWait: {
            mem_done = stepMemWriteLine();
            if (mem_done) {
                BUSLOG("DRAM WRITE done owner=" << tx_.owner
                    << " addr_line=0x" << std::hex << tx_.addr_line << std::dec);
                if (!tx_.signaled_done) {
                    b2m_[tx_.owner].B_DONE.store(true, std::memory_order_release);
                    tx_.signaled_done = true;
                }
                tx_.mem = ActiveTx::MemPhase::None;
            }
            break;
        }
        default: break;
        }

        if (tx_.busy && tx_.owner >= 0)
            b2m_[tx_.owner].B_GRANT.store(true, std::memory_order_relaxed);
        return;
    }

    // ====================================================================
    // CRITICAL FIX: ESPERAR A QUE B_REQ BAJE ANTES DE LIBERAR TRANSACCIÓN
    // ====================================================================
    // 0.5) HOLD-UNTIL-CONSUMED
    if (tx_.busy && tx_.owner >= 0 && tx_.mem == ActiveTx::MemPhase::None) {
        bool need_hold = false;

        switch (tx_.cmd) {
        case BusCmd::BusRd:
        case BusCmd::BusRdX: {
            bool rv = b2m_[tx_.owner].B_RVALID.load(std::memory_order_acquire);
            bool dn = b2m_[tx_.owner].B_DONE.load(std::memory_order_acquire);

            if (rv || dn) {
                need_hold = true;
                break;
            }

            // CRITICAL: También verificar que B_REQ haya bajado
            // Esto garantiza que la caché completó FILL y actualizó su estado
            bool req = m2b_[tx_.owner].B_REQ.load(std::memory_order_acquire);
            if (req) {
                need_hold = true;
            }
            break;
        }
        case BusCmd::WriteBack:
        case BusCmd::BusUpgr: {
            bool dn = b2m_[tx_.owner].B_DONE.load(std::memory_order_acquire);
            if (dn) {
                need_hold = true;
                break;
            }

            bool req = m2b_[tx_.owner].B_REQ.load(std::memory_order_acquire);
            if (req) {
                need_hold = true;
            }
            break;
        }
        default: break;
        }

        if (need_hold) {
            b2m_[tx_.owner].B_GRANT.store(true, std::memory_order_relaxed);
            return;
        }
        else {
            if (tx_.cmd != BusCmd::WriteBack) {
                req_edge_block_[tx_.owner] = true;
            }
            tx_.busy = false;
        }
    }

    // 1) Si no hay transacción activa, buscar una nueva
    if (!tx_.busy) {
        int owner = pickOwnerRR();
        if (owner < 0) return;

        tx_.busy = true;
        tx_.owner = owner;
        tx_.signaled_rvalid = false;
        tx_.signaled_done = false;

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
        tx_.rdata.fill(0);

        BUSLOG("NEW owner=" << owner
            << " cmd=" << (int)tx_.cmd
            << " addr_line=0x" << std::hex << tx_.addr_line << std::dec);

        b2m_[owner].B_GRANT.store(true, std::memory_order_release);

        // ====== MÉTRICAS (ADDED): miss y transición del solicitante ======
        if (shared_) {
            if (tx_.cmd == BusCmd::BusRd || tx_.cmd == BusCmd::BusRdX) {
                shared_->analysis.cache_misses.fetch_add(1, std::memory_order_relaxed);
                shared_->analysis.transactions_mesi.fetch_add(1, std::memory_order_relaxed); // I->E/S o I->M
            }
            else if (tx_.cmd == BusCmd::BusUpgr) {
                shared_->analysis.transactions_mesi.fetch_add(1, std::memory_order_relaxed); // S->M
            }
        }

        // Difusión Snoop
        for (int id = 0; id < (int)m2b_.size(); ++id) {
            if (id == owner) continue;

            std::function<SnoopResp(const SnoopReq&)> cb;
            {
                std::lock_guard<std::mutex> lk(cb_mtx_);
                cb = (id >= 0 && id < (int)sn_cb_.size()) ? sn_cb_[id] : nullptr;
            }
            if (!cb) continue;

            SnoopReq s;
            s.cmd = tx_.cmd;
            s.addr_line = tx_.addr_line;
            s.grant_data = false;
            s.from_self = false;

            BUSLOG("SNP → L1" << id);
            SnoopResp r = cb(s);
            BUSLOG("SNP ← L1" << id
                << " shared=" << r.has_shared
                << " mod=" << r.has_mod
                << " inv_ack=" << r.inv_ack);

            tx_.seen_shared |= r.has_shared;
            if (r.has_mod) {
                tx_.seen_hitm = true;
                if (tx_.m_owner < 0) tx_.m_owner = id;
            }

            // ====== MÉTRICAS (ADDED): invalidaciones por BusRdX ======
            if (tx_.cmd == BusCmd::BusRdX && r.inv_ack && shared_) {
                shared_->analysis.invalidations.fetch_add(1, std::memory_order_relaxed);
                shared_->analysis.transactions_mesi.fetch_add(1, std::memory_order_relaxed); // S/E->I o M->I
            }

            if (tx_.cmd == BusCmd::BusUpgr) {
                if (r.has_shared && !r.has_mod) tx_.inv_acks_needed++;
                if (r.inv_ack)                  tx_.inv_acks_got++;
            }
        }

        BUSLOG("SNP summary: shared_seen=" << tx_.seen_shared
            << " hitm_seen=" << tx_.seen_hitm
            << " m_owner=" << tx_.m_owner
            << " inv_needed=" << tx_.inv_acks_needed
            << " inv_got=" << tx_.inv_acks_got);

        b2m_[owner].B_SHARED_SEEN.store(tx_.seen_shared, std::memory_order_relaxed);
        b2m_[owner].B_HITM_SEEN.store(tx_.seen_hitm, std::memory_order_relaxed);

        // ====== MÉTRICAS (ADDED): invalidaciones por BusUpgr (en total) ======
        if (shared_ && tx_.cmd == BusCmd::BusUpgr && tx_.inv_acks_got > 0) {
            shared_->analysis.invalidations.fetch_add(tx_.inv_acks_got, std::memory_order_relaxed);
            shared_->analysis.transactions_mesi.fetch_add(tx_.inv_acks_got, std::memory_order_relaxed);
        }

        // Ejecución según comando
        switch (tx_.cmd) {

        case BusCmd::BusRd:
        case BusCmd::BusRdX: {
            if (tx_.seen_hitm && tx_.m_owner >= 0) {
                std::function<SnoopResp(const SnoopReq&)> cb2;
                {
                    std::lock_guard<std::mutex> lk(cb_mtx_);
                    cb2 = (tx_.m_owner >= 0 && tx_.m_owner < (int)sn_cb_.size())
                        ? sn_cb_[tx_.m_owner] : nullptr;
                }

                if (cb2) {
                    SnoopReq s2;
                    s2.cmd = tx_.cmd;
                    s2.addr_line = tx_.addr_line;
                    s2.grant_data = true;
                    s2.from_self = false;

                    BUSLOG("C2C grant_data → L1" << tx_.m_owner);
                    SnoopResp r2 = cb2(s2);
                    BUSLOG("C2C grant_data ← L1" << tx_.m_owner
                        << " rvalid=" << r2.rvalid);

                    if (r2.rvalid) {
                        tx_.rdata = r2.rdata;
                        tx_.have_rdata = true;

                        if (!tx_.signaled_rvalid) {
                            b2m_[owner].B_RDATA = tx_.rdata;
                            b2m_[owner].B_RVALID.store(true, std::memory_order_release);
                            tx_.signaled_rvalid = true;
                        }
                        if (!tx_.signaled_done) {
                            b2m_[owner].B_DONE.store(true, std::memory_order_release);
                            tx_.signaled_done = true;
                        }

                        // ====== MÉTRICAS (ADDED): downgrade del dueño M ======
                        if (shared_) {
                            // M->S en BusRd, M->I en BusRdX
                            shared_->analysis.transactions_mesi.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                }
            }

            if (!tx_.have_rdata) {
                BUSLOG("DRAM READ start base_idx=0x" << std::hex << tx_.addr_line << std::dec);
                startMemReadLine();
            }
            break;
        }

        case BusCmd::WriteBack: {
            if (!m2b_[owner].B_WVALID.load(std::memory_order_acquire)) {
                if (!tx_.signaled_done) {
                    b2m_[owner].B_DONE.store(true, std::memory_order_release);
                    tx_.signaled_done = true;
                }
                break;
            }

            tx_.wb_line = m2b_[owner].B_WDATA;
            BUSLOG("DRAM WRITE start base_idx=0x" << std::hex << tx_.addr_line << std::dec);
            startMemWriteLine();
            break;
        }

        case BusCmd::BusUpgr: {
            if (tx_.inv_acks_needed > 0 && tx_.inv_acks_got < tx_.inv_acks_needed) {
                BUSLOG("WARN: BusUpgr incomplete invalidations");
            }
            if (!tx_.signaled_done) {
                b2m_[owner].B_DONE.store(true, std::memory_order_release);
                tx_.signaled_done = true;
            }
            break;
        }

        default:
            break;
        }
    }
}