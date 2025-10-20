//l1_fsm.cpp
#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_utils.h"
#include "programs/cpu_tlp_shared_cache/components/bus/interconnect_bus.h" // MasterToBus / BusToMaster
#include <iostream>

// --------- util de logging: imprime sólo cuando cambia el valor ----------
#define CONCAT_IMPL(x,y) x##y
#define CONCAT(x,y) CONCAT_IMPL(x,y)
#define LOG_ON_CHANGE(TAG, EXPR) do {                                \
  static auto CONCAT(_last_, __LINE__) = (EXPR);                     \
  auto        CONCAT(_now_,  __LINE__) = (EXPR);                     \
  if (CONCAT(_now_, __LINE__) != CONCAT(_last_, __LINE__)) {         \
    std::cout << TAG << (CONCAT(_now_, __LINE__)) << "\n";           \
    CONCAT(_last_, __LINE__) = CONCAT(_now_, __LINE__);              \
  }                                                                  \
} while(0)

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

L1Cache::L1Cache() { reset(); }

void L1Cache::reset() {
    std::lock_guard<std::mutex> lk(mtx_);
    for (auto& s : sets_) {
        s.lru = 0;
        for (auto& w : s.ways) {
            w.tag = 0; w.state = Mesi::I; w.valid = false; w.data.fill(0);
        }
    }
    out_.C_READY = false; out_.RD_C_out = 0;
    fsm_ = L1State::IDLE;
    in_ = {};
    pend_ = {};
    prev_req_ = false;
    temp_fill_.fill(0);
}

void L1Cache::beginAccess(const CpuReq& req) {
    std::lock_guard<std::mutex> lk(mtx_);
    in_ = req;

    // Fast path: soltar READY si llega ACK en WAIT_ACK
    if (fsm_ == L1State::WAIT_ACK && in_.C_READY_ACK) {
        out_.C_READY = false;
        fsm_ = L1State::IDLE;
        // prev_req_ se actualiza abajo con el nivel actual
    }

    // *** edge detect de C_REQUEST_M ***
    bool rising_req = (in_.C_REQUEST_M && !prev_req_);
    prev_req_ = in_.C_REQUEST_M;

    // Nuevo acceso sólo en flanco
    if (fsm_ == L1State::IDLE && rising_req && !out_.C_READY) {
        fsm_ = L1State::LOOKUP;
    }
}

void L1Cache::tick() {
    std::lock_guard<std::mutex> lk(mtx_);
    using std::memory_order_acquire;
    using std::memory_order_release;
    using std::memory_order_relaxed;
    using std::memory_order_acq_rel;

    // --------- LOG COMPACTO: sólo cambios de estado/señales ----------
    {
        static L1State last = L1State::IDLE;
        if (fsm_ != last) {
            std::cout << "[L1" << l1_id_ << "] FSM: " << stateName(last)
                << " -> " << stateName(fsm_) << "\n";
            last = fsm_;
        }
        if (pm_out_) LOG_ON_CHANGE("[L1] B_REQ=", pm_out_->B_REQ.load(memory_order_acquire));
        if (pm_in_) {
            LOG_ON_CHANGE("[L1] B_GRANT=", pm_in_->B_GRANT.load(memory_order_acquire));
            LOG_ON_CHANGE("[L1] B_RVALID=", pm_in_->B_RVALID.load(memory_order_acquire));
            LOG_ON_CHANGE("[L1] B_DONE=", pm_in_->B_DONE.load(memory_order_acquire));
        }
    }
    // ----------------------------------------------------------------

    switch (fsm_) {

    case L1State::LOOKUP: {
        const auto p = splitAddress(in_.ALUOut_M);
        auto& set = sets_[p.set];

        int hit_way = findWay(p.set, p.tag);
        if (hit_way >= 0) {
            auto& line = set.ways[hit_way];

            if (!in_.C_WE_M) {
                // READ HIT (LDR / LDRB)
                if (in_.C_ISB_M)
                    out_.RD_C_out = static_cast<uint64_t>(read8_in_line(line, p.byte_in_line));
                else if ((p.off & 0x7) == 0)
                    out_.RD_C_out = read64_in_line(line, p.dw_in_line);

                // LRU (MRU = hit_way)
                set.lru = (hit_way == 0) ? 0 : 1;

                out_.C_READY = true;
                fsm_ = L1State::WAIT_ACK;

            }
            else {
                // WRITE
                if (line.state == Mesi::S) {
                    // Write-hit en S → necesita exclusividad: BusUpgr
                    pend_ = {};
                    pend_.cmd = BusCmd::BusUpgr;
                    pend_.req_addr_line = (in_.ALUOut_M & ~((1ULL << OFFSET_BITS) - 1ULL));
                    pend_.set = p.set;
                    pend_.victim = hit_way;

                    if (pm_out_) {
                        pm_out_->B_CMD = BusCmd::BusUpgr;
                        pm_out_->B_ADDR = pend_.req_addr_line;
                        pm_out_->B_WVALID.store(false, memory_order_relaxed);
                        pm_out_->B_REQ.store(true, memory_order_release);
                    }
                    fsm_ = L1State::REQ_BUS;
                    break;
                }

                // Estados E o M: escribir localmente
                if (in_.C_ISB_M) {
                    write8_in_line(line, p.byte_in_line,
                        static_cast<uint8_t>(in_.RD_Rm_Special_M & 0xFF));
                }
                else {
                    if ((p.off & 0x7) != 0) break; // misaligned: ignora en MVP
                    write64_in_line(line, p.dw_in_line, in_.RD_Rm_Special_M);
                }
                if (line.state == Mesi::E) line.state = Mesi::M;

                // LRU
                set.lru = (hit_way == 0) ? 0 : 1;

                out_.C_READY = true;
                fsm_ = L1State::WAIT_ACK;
            }
        }
        else {
            // MISS
            pend_ = {};
            pend_.req_addr_line = (in_.ALUOut_M & ~((1ULL << OFFSET_BITS) - 1ULL));
            pend_.set = p.set;

            // Víctima LRU simple
            pend_.victim = (set.lru == 0 ? 1 : 0);
            auto& vict = set.ways[pend_.victim];

            // Si la víctima está en M → WriteBack antes de llenar
            pend_.need_wb = (vict.valid && vict.state == Mesi::M);
            if (pend_.need_wb) {
                // reconstruir dirección de la línea víctima
                pend_.victim_addr_line =
                    ((vict.tag << (INDEX_BITS + OFFSET_BITS)) |
                        (static_cast<uint64_t>(pend_.set) << OFFSET_BITS));
                pend_.wb_line = vict.data;
            }

            // Decide comando según acceso: read→BusRd, write→BusRdX (RFO)
            pend_.cmd = (!in_.C_WE_M) ? BusCmd::BusRd : BusCmd::BusRdX;

            // Solicitar bus (WB si aplica, si no la solicitud real)
            if (pm_out_) {
                pm_out_->B_CMD = pend_.need_wb ? BusCmd::WriteBack : pend_.cmd;
                pm_out_->B_ADDR = pend_.need_wb ? pend_.victim_addr_line : pend_.req_addr_line;
                pm_out_->B_WDATA = pend_.wb_line; // payload no atómico
                pm_out_->B_WVALID.store(pend_.need_wb, memory_order_relaxed);
                pm_out_->B_REQ.store(true, memory_order_release);
            }
            fsm_ = L1State::REQ_BUS;
        }
    } break;

    case L1State::REQ_BUS: {
        // esperar arbitraje
        fsm_ = L1State::WAIT_GRANT;
    } break;

    case L1State::WAIT_GRANT: {
        if (pm_in_ && pm_in_->B_GRANT.load(memory_order_acquire)) {
            // Flags efímeros (relaxed)
            pend_.saw_shared |= pm_in_->B_SHARED_SEEN.load(memory_order_relaxed);
            pend_.saw_hitm |= pm_in_->B_HITM_SEEN.load(memory_order_relaxed);
            fsm_ = L1State::WAIT_DATA;
        }
    } break;

    case L1State::WAIT_DATA: {
        if (!pm_in_) break;

        // Mantener copia de flags efímeros
        pend_.saw_shared |= pm_in_->B_SHARED_SEEN.load(memory_order_relaxed);
        pend_.saw_hitm |= pm_in_->B_HITM_SEEN.load(memory_order_relaxed);

        // Si es lectura (BusRd/BusRdX) y hay datos válidos, consumir el flag y quedarnos con la línea
        if (pm_in_->B_RVALID.exchange(false, memory_order_acq_rel)) {
            temp_fill_ = pm_in_->B_RDATA; // payload ya publicado por el bus antes del RVALID
        }

        // Cierre de la transacción actual (consumir DONE una sola vez)
        if (pm_in_->B_DONE.exchange(false, memory_order_acq_rel)) {

            // ¿Era un WriteBack previo? → reemitir la tx real
            if (pm_out_ && pm_out_->B_CMD == BusCmd::WriteBack) {
                pm_out_->B_REQ.store(false, memory_order_release);     // edge de baja
                pm_out_->B_CMD = pend_.cmd;
                pm_out_->B_ADDR = pend_.req_addr_line;
                pm_out_->B_WVALID.store(false, memory_order_relaxed);
                pend_.saw_shared = false;
                pend_.saw_hitm = false;
                pm_out_->B_REQ.store(true, memory_order_release);      // edge de subida
                fsm_ = L1State::WAIT_GRANT; // re-entrar al grant de la tx real
                break;
            }

            // Transacción real terminada → ir a FILL (o upgrade)
            fsm_ = L1State::FILL;
        }
    } break;

    case L1State::FILL: {
        const auto p = splitAddress(in_.ALUOut_M);
        auto& set = sets_[pend_.set];

        if (pend_.cmd == BusCmd::BusUpgr) {
            // Upgrade no trae datos; solo cambiar S→M y aplicar store
            int way = findWay(pend_.set, p.tag);
            if (way >= 0) {
                auto& line = set.ways[way];
                line.state = Mesi::M;
                if (in_.C_ISB_M) {
                    write8_in_line(line, p.byte_in_line,
                        static_cast<uint8_t>(in_.RD_Rm_Special_M & 0xFF));
                }
                else if ((p.off & 0x7) == 0) {
                    write64_in_line(line, p.dw_in_line, in_.RD_Rm_Special_M);
                }
                set.lru = (way == 0) ? 0 : 1;
            }
            out_.C_READY = true;
            if (pm_out_) {
                pm_out_->B_WVALID.store(false, memory_order_release);
                pm_out_->B_REQ.store(false, memory_order_release);
            }
            fsm_ = L1State::WAIT_ACK;
            break;
        }

        // FILL de línea para BusRd / BusRdX
        auto& vict = set.ways[pend_.victim];
        vict.data = temp_fill_;
        vict.tag = (pend_.req_addr_line >> (OFFSET_BITS + INDEX_BITS));
        vict.valid = true;

        if (pend_.cmd == BusCmd::BusRd) {
            // E si nadie más la tiene; S si hubo shared/hitm
            vict.state = (pend_.saw_shared || pend_.saw_hitm) ? Mesi::S : Mesi::E;

            // Entregar dato de lectura al CPU
            if (in_.C_ISB_M)
                out_.RD_C_out = static_cast<uint64_t>(read8_in_line(vict, p.byte_in_line));
            else if ((p.off & 0x7) == 0)
                out_.RD_C_out = read64_in_line(vict, p.dw_in_line);

        }
        else { // BusRdX (RFO) => terminar en M y aplicar el store
            vict.state = Mesi::M;
            if (in_.C_ISB_M)
                write8_in_line(vict, p.byte_in_line,
                    static_cast<uint8_t>(in_.RD_Rm_Special_M & 0xFF));
            else if ((p.off & 0x7) == 0)
                write64_in_line(vict, p.dw_in_line, in_.RD_Rm_Special_M);
        }

        // LRU: la víctima pasa a MRU
        set.lru = (pend_.victim == 0) ? 0 : 1;

        // Handshake al CPU
        out_.C_READY = true;

        // Soltar el bus
        if (pm_out_) {
            pm_out_->B_WVALID.store(false, memory_order_release);
            pm_out_->B_REQ.store(false, memory_order_release);
        }

        fsm_ = L1State::WAIT_ACK;
    } break;

    case L1State::WAIT_ACK: {
        if (in_.C_READY_ACK) {
            out_.C_READY = false;
            fsm_ = L1State::IDLE;
        }
    } break;

    default: break;
    }
}

int L1Cache::findWay(uint32_t set, uint64_t tag) const {
    const auto& cs = sets_[set];
    for (int w = 0; w < (int)WAYS; ++w) {
        const auto& line = cs.ways[w];
        if (line.valid && line.state != Mesi::I && line.tag == tag) return w;
    }
    return -1;
}
