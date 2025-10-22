#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_utils.h"
#include "programs/cpu_tlp_shared_cache/components/bus/interconnect_bus.h"
#include <iostream>

static const char* stateName(L1State s) {
    switch (s) {
    case L1State::IDLE:       return "IDLE";
    case L1State::LOOKUP:     return "LOOKUP";
    case L1State::WAIT_ACK:   return "WAIT_ACK";
    case L1State::REQ_BUS:    return "REQ_BUS";
    case L1State::WAIT_GRANT: return "WAIT_GRANT";
    case L1State::WAIT_DATA:  return "WAIT_DATA";
    case L1State::FILL:       return "FILL";
    default:                  return "?";
    }
}

static int pickVictimWay(CacheSet& set, uint64_t target_tag) {
    for (int w = 0; w < WAYS; ++w) {
        if (set.ways[w].valid && set.ways[w].tag == target_tag) {
            return w;
        }
    }
    for (int w = 0; w < WAYS; ++w) {
        if (!set.ways[w].valid || set.ways[w].state == Mesi::I) {
            return w;
        }
    }
    return 0;
}

L1Cache::L1Cache() { reset(); }

void L1Cache::reset() {
    std::lock_guard<std::mutex> lk(mtx_);
    for (auto& s : sets_) {
        s.lru = 0;
        for (auto& w : s.ways) {
            w.tag = INVALID_TAG_SENTINEL;
            w.state = Mesi::I;
            w.valid = false;
            w.data.fill(0);
        }
    }
    out_.C_READY = false; out_.RD_C_out = 0;
    fsm_ = L1State::IDLE;
    in_ = {};
    pend_ = {};
    prev_req_ = false;
    temp_fill_.fill(0);

    dbg_prev_fsm_ = L1State::IDLE;
    dbg_prev_B_REQ_ = false;
    dbg_prev_B_GRANT_ = false;
    dbg_prev_B_RVALID_ = false;
    dbg_prev_B_DONE_ = false;
}

void L1Cache::beginAccess(const CpuReq& req) {
    std::lock_guard<std::mutex> lk(mtx_);
    in_ = req;

    // ====================================================================
    // CRITICAL FIX: Bajar B_REQ AQUÍ cuando recibimos ACK
    // ====================================================================
    if (fsm_ == L1State::WAIT_ACK && in_.C_READY_ACK) {
        out_.C_READY = false;

        // Bajar B_REQ antes de cambiar a IDLE
        if (pm_out_) {
            pm_out_->B_REQ.store(false, std::memory_order_release);
        }

        fsm_ = L1State::IDLE;
    }

    bool rising_req = (in_.C_REQUEST_M && !prev_req_);
    prev_req_ = in_.C_REQUEST_M;

    if (fsm_ == L1State::IDLE && rising_req && !out_.C_READY) {
        fsm_ = L1State::LOOKUP;
    }
}

void L1Cache::logSignals_() {
    if (fsm_ != dbg_prev_fsm_) {
        std::cout << "[L1" << l1_id_ << "] FSM: "
            << stateName(dbg_prev_fsm_) << " -> " << stateName(fsm_) << "\n";
        dbg_prev_fsm_ = fsm_;
    }

    if (pm_out_) {
        bool now = pm_out_->B_REQ.load(std::memory_order_acquire);
        if (now != dbg_prev_B_REQ_) {
            std::cout << "[L1" << l1_id_ << "] B_REQ=" << now << "\n";
            dbg_prev_B_REQ_ = now;
        }
    }
    if (pm_in_) {
        bool g = pm_in_->B_GRANT.load(std::memory_order_acquire);
        if (g != dbg_prev_B_GRANT_) {
            std::cout << "[L1" << l1_id_ << "] B_GRANT=" << g << "\n";
            dbg_prev_B_GRANT_ = g;
        }
        bool rv = pm_in_->B_RVALID.load(std::memory_order_acquire);
        if (rv != dbg_prev_B_RVALID_) {
            std::cout << "[L1" << l1_id_ << "] B_RVALID=" << rv << "\n";
            dbg_prev_B_RVALID_ = rv;
        }
        bool dn = pm_in_->B_DONE.load(std::memory_order_acquire);
        if (dn != dbg_prev_B_DONE_) {
            std::cout << "[L1" << l1_id_ << "] B_DONE=" << dn << "\n";
            dbg_prev_B_DONE_ = dn;
        }
    }
}

void L1Cache::tick() {
    std::lock_guard<std::mutex> lk(mtx_);
    using std::memory_order_acquire;
    using std::memory_order_release;
    using std::memory_order_relaxed;
    using std::memory_order_acq_rel;

    logSignals_();

    switch (fsm_) {

    case L1State::LOOKUP: {
        const auto p = splitAddress(in_.ALUOut_M);
        auto& set = sets_[p.set];

        int hit_way = findWay(p.set, p.tag);
        if (hit_way >= 0) {
            auto& line = set.ways[hit_way];

            if (line.state == Mesi::I) {
                pend_ = {};
                pend_.req_addr_line = (in_.ALUOut_M & ~((1ULL << OFFSET_BITS) - 1ULL));
                pend_.set = p.set;
                pend_.victim = hit_way;
                pend_.need_wb = false;
                pend_.byte_in_line = p.byte_in_line;
                pend_.dw_in_line = p.dw_in_line;
                pend_.cmd = (!in_.C_WE_M) ? BusCmd::BusRd : BusCmd::BusRdX;

                if (pm_out_) {
                    pm_out_->B_CMD = pend_.cmd;
                    pm_out_->B_ADDR = pend_.req_addr_line;
                    pm_out_->B_WVALID.store(false, memory_order_relaxed);
                    pm_out_->B_REQ.store(true, memory_order_release);
                }
                fsm_ = L1State::REQ_BUS;
            }
            else if (!in_.C_WE_M) {
                if (in_.C_ISB_M)
                    out_.RD_C_out = static_cast<uint64_t>(read8_in_line(line, p.byte_in_line));
                else if ((p.off & 0x7) == 0)
                    out_.RD_C_out = read64_in_line(line, p.dw_in_line);

                set.lru = (hit_way == 0) ? 0 : 1;
                out_.C_READY = true;
                fsm_ = L1State::WAIT_ACK;
            }
            else {
                if (line.state == Mesi::S) {
                    pend_ = {};
                    pend_.cmd = BusCmd::BusUpgr;
                    pend_.req_addr_line = (in_.ALUOut_M & ~((1ULL << OFFSET_BITS) - 1ULL));
                    pend_.set = p.set;
                    pend_.victim = hit_way;
                    pend_.byte_in_line = p.byte_in_line;
                    pend_.dw_in_line = p.dw_in_line;

                    if (pm_out_) {
                        pm_out_->B_CMD = BusCmd::BusUpgr;
                        pm_out_->B_ADDR = pend_.req_addr_line;
                        pm_out_->B_WVALID.store(false, memory_order_relaxed);
                        pm_out_->B_REQ.store(true, memory_order_release);
                    }
                    fsm_ = L1State::REQ_BUS;
                    break;
                }

                if (in_.C_ISB_M) {
                    write8_in_line(line, p.byte_in_line,
                        static_cast<uint8_t>(in_.RD_Rm_Special_M & 0xFF));
                }
                else {
                    if ((p.off & 0x7) != 0) break;
                    write64_in_line(line, p.dw_in_line, in_.RD_Rm_Special_M);
                }
                if (line.state == Mesi::E) line.state = Mesi::M;

                set.lru = (hit_way == 0) ? 0 : 1;
                out_.C_READY = true;
                fsm_ = L1State::WAIT_ACK;
            }
        }
        else {
            pend_ = {};
            pend_.req_addr_line = (in_.ALUOut_M & ~((1ULL << OFFSET_BITS) - 1ULL));
            pend_.set = p.set;

            pend_.victim = pickVictimWay(set, p.tag);
            auto& vict = set.ways[pend_.victim];

            pend_.need_wb = (vict.valid && vict.state == Mesi::M);
            if (pend_.need_wb) {
                pend_.victim_addr_line =
                    ((vict.tag << (INDEX_BITS + OFFSET_BITS)) |
                        (static_cast<uint64_t>(pend_.set) << OFFSET_BITS));
                pend_.wb_line = vict.data;
            }

            pend_.byte_in_line = p.byte_in_line;
            pend_.dw_in_line = p.dw_in_line;
            pend_.cmd = (!in_.C_WE_M) ? BusCmd::BusRd : BusCmd::BusRdX;

            if (pm_out_) {
                pm_out_->B_CMD = pend_.need_wb ? BusCmd::WriteBack : pend_.cmd;
                pm_out_->B_ADDR = pend_.need_wb ? pend_.victim_addr_line : pend_.req_addr_line;
                pm_out_->B_WDATA = pend_.wb_line;
                pm_out_->B_WVALID.store(pend_.need_wb, memory_order_relaxed);
                pm_out_->B_REQ.store(true, memory_order_release);
            }
            fsm_ = L1State::REQ_BUS;
        }
    } break;

    case L1State::REQ_BUS: {
        if (pm_out_) {
            pm_out_->B_REQ.store(true, memory_order_release);
        }
        fsm_ = L1State::WAIT_GRANT;
    } break;

    case L1State::WAIT_GRANT: {
        if (pm_in_ && pm_in_->B_GRANT.load(memory_order_acquire)) {
            pend_.saw_shared |= pm_in_->B_SHARED_SEEN.load(memory_order_relaxed);
            pend_.saw_hitm |= pm_in_->B_HITM_SEEN.load(memory_order_relaxed);
            fsm_ = L1State::WAIT_DATA;
        }
    } break;

    case L1State::WAIT_DATA: {
        if (!pm_in_) break;

        pend_.saw_shared |= pm_in_->B_SHARED_SEEN.load(memory_order_relaxed);
        pend_.saw_hitm |= pm_in_->B_HITM_SEEN.load(memory_order_relaxed);

        if (pm_in_->B_RVALID.exchange(false, memory_order_acq_rel)) {
            temp_fill_ = pm_in_->B_RDATA;
        }

        if (pm_in_->B_DONE.exchange(false, memory_order_acq_rel)) {
            if (pm_out_ && pm_out_->B_CMD == BusCmd::WriteBack) {
                // CRÍTICO: Bajar B_REQ primero para permitir edge-trigger
                pm_out_->B_REQ.store(false, memory_order_release);

                // Preparar el nuevo request
                pm_out_->B_CMD = pend_.cmd;
                pm_out_->B_ADDR = pend_.req_addr_line;
                pm_out_->B_WVALID.store(false, memory_order_relaxed);
                pend_.saw_shared = false;
                pend_.saw_hitm = false;

                // Volver a REQ_BUS para que suba B_REQ en el siguiente ciclo
                fsm_ = L1State::REQ_BUS;
                break;
            }
            fsm_ = L1State::FILL;
        }
    } break;

    case L1State::FILL: {
        const auto p_now = splitAddress(in_.ALUOut_M);
        auto& set = sets_[pend_.set];

        if (pend_.cmd == BusCmd::BusUpgr) {
            int way = findWay(pend_.set, (pend_.req_addr_line >> (OFFSET_BITS + INDEX_BITS)));
            if (way >= 0) {
                auto& line = set.ways[way];
                line.state = Mesi::M;
                if (in_.C_ISB_M) {
                    write8_in_line(line, pend_.byte_in_line,
                        static_cast<uint8_t>(in_.RD_Rm_Special_M & 0xFF));
                }
                else {
                    write64_in_line(line, pend_.dw_in_line, in_.RD_Rm_Special_M);
                }
                set.lru = (way == 0) ? 0 : 1;
            }
            out_.C_READY = true;
            if (pm_out_) {
                pm_out_->B_WVALID.store(false, memory_order_release);
                // NO bajar B_REQ aquí - se baja en beginAccess al recibir ACK
            }
            fsm_ = L1State::WAIT_ACK;
            break;
        }

        auto& vict = set.ways[pend_.victim];
        vict.data = temp_fill_;
        vict.tag = (pend_.req_addr_line >> (OFFSET_BITS + INDEX_BITS));
        vict.valid = true;

        if (pend_.cmd == BusCmd::BusRd) {
            vict.state = (pend_.saw_shared || pend_.saw_hitm) ? Mesi::S : Mesi::E;

            if (in_.C_ISB_M)
                out_.RD_C_out = static_cast<uint64_t>(read8_in_line(vict, pend_.byte_in_line));
            else
                out_.RD_C_out = read64_in_line(vict, pend_.dw_in_line);
        }
        else {
            vict.state = Mesi::M;
            if (in_.C_ISB_M)
                write8_in_line(vict, pend_.byte_in_line,
                    static_cast<uint8_t>(in_.RD_Rm_Special_M & 0xFF));
            else
                write64_in_line(vict, pend_.dw_in_line, in_.RD_Rm_Special_M);
        }

        set.lru = (pend_.victim == 0) ? 0 : 1;
        out_.C_READY = true;

        if (pm_out_) {
            pm_out_->B_WVALID.store(false, memory_order_release);
            // NO bajar B_REQ aquí - se baja en beginAccess al recibir ACK
        }

        fsm_ = L1State::WAIT_ACK;
    } break;

    case L1State::WAIT_ACK: {
        // Este estado ahora solo se maneja en beginAccess
        // No hacemos nada aquí, esperamos el ACK del CPU
    } break;

    default: break;
    }
}

int L1Cache::findWay(uint32_t set, uint64_t tag) const {
    const auto& cs = sets_[set];
    for (int w = 0; w < (int)WAYS; ++w) {
        const auto& line = cs.ways[w];
        if (line.valid && line.tag == tag) return w;
    }
    return -1;
}