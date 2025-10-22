#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_utils.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h"
#include <iostream>
#define SNOOPLOG(MSG) do{ std::cout << "[L1Snoop] " << MSG << std::endl; }while(0)

SnoopResp L1Cache::onSnoop(const SnoopReq& s) {
    std::lock_guard<std::mutex> lk(mtx_);

    SnoopResp r{};
    const auto a = splitAddress(s.addr_line);

    // ====================================================================
    // CRITICAL FIX: Verificar transacciones pendientes PRIMERO
    // ====================================================================
    // Si tenemos una transacción activa para esta línea, responder basado
    // en el estado que TENDRÁ la línea cuando complete, no en su estado actual.
    // Esto previene que múltiples cachés adquieran la misma línea en M.

    if (pend_.is_active) {
        // Extraer tag de la dirección pendiente
        uint64_t pend_tag = pend_.req_addr_line >> (OFFSET_BITS + INDEX_BITS);

        if (pend_.set == a.set && pend_tag == a.tag) {
            // Esta transacción es para la misma línea que están preguntando

            // Determinar qué estado tendrá cuando complete
            Mesi future_state = pend_.target_state;

            // Reportar basado en el estado futuro
            const bool will_be_S = (future_state == Mesi::S);
            const bool will_be_E = (future_state == Mesi::E);
            const bool will_be_M = (future_state == Mesi::M);

            r.has_shared = (will_be_S || will_be_E || will_be_M);
            r.has_mod = will_be_M;

            // Si el snoop es BusRdX y vamos a tener la línea, invalidarnos
            if (s.cmd == BusCmd::BusRdX) {
                if (will_be_S || will_be_E) {
                    // Cancelar la transacción pendiente - otra caché la está tomando exclusiva
                    pend_.is_active = false;
                    pend_.target_state = Mesi::I;
                    r.inv_ack = true;
                }
                else if (will_be_M) {
                    // Si íbamos a estar en M, downgrade a I
                    pend_.target_state = Mesi::I;
                    pend_.is_active = false;
                    r.inv_ack = true;
                }
            }
            else if (s.cmd == BusCmd::BusRd) {
                if (will_be_E) {
                    // Downgrade E -> S
                    pend_.target_state = Mesi::S;
                }
                else if (will_be_M) {
                    // Downgrade M -> S
                    pend_.target_state = Mesi::S;
                }
            }
            else if (s.cmd == BusCmd::BusUpgr) {
                if (will_be_S || will_be_E) {
                    pend_.is_active = false;
                    pend_.target_state = Mesi::I;
                    r.inv_ack = true;
                }
            }

            return r;
        }
    }

    // Si no hay transacción pendiente, verificar línea existente
    auto& set = sets_[a.set];
    int way = findWay(a.set, a.tag);
    if (way < 0) return r;

    auto& line = set.ways[way];
    if (!line.valid || line.tag != a.tag) return r;

    const bool isS = (line.state == Mesi::S);
    const bool isE = (line.state == Mesi::E);
    const bool isM = (line.state == Mesi::M);

    r.has_shared = (isS || isE || isM);
    r.has_mod = isM;

    switch (s.cmd) {
    case BusCmd::BusRd: {
        if (isM) {
            if (s.grant_data) {
                r.rdata = line.data;
                r.rvalid = true;
                line.state = Mesi::S; // downgrade después de dar data
            }
        }
        else if (isE) {
            line.state = Mesi::S;
        }
    } break;

    case BusCmd::BusRdX: {
        if (isM) {
            if (s.grant_data) {
                r.rdata = line.data;
                r.rvalid = true;
                line.state = Mesi::I;   // invalidarse DESPUÉS de dar data
                line.valid = false;
                r.inv_ack = true;
            }
            else {
                // 1ª pasada: no me invalido aún
            }
        }
        else {
            // S/E invalidan en la 1ª pasada
            if (line.state != Mesi::I) {
                line.state = Mesi::I;
                line.valid = false;
                r.inv_ack = true;
            }
        }
    } break;

    case BusCmd::BusUpgr: {
        if (isS || isE) {
            line.state = Mesi::I;
            line.valid = false;
            r.inv_ack = true;
        }
    } break;

    default: break;
    }

    return r;
}