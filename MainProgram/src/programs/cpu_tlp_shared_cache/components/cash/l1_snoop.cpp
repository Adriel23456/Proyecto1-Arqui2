#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_utils.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h"
#include <iostream>
#define SNOOPLOG(MSG) do{ std::cout << "[L1Snoop] " << MSG << std::endl; }while(0)

SnoopResp L1Cache::onSnoop(const SnoopReq & s) {
    std::lock_guard<std::mutex> lk(mtx_);

    SnoopResp r{};
    const auto a = splitAddress(s.addr_line);

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
