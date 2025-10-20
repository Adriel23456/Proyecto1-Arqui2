//l1_snoop.cpp
#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_utils.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h"
#include <iostream>
#define SNOOPLOG(MSG) do{ std::cout << "[L1Snoop] " << MSG << std::endl; }while(0)


SnoopResp L1Cache::onSnoop(const SnoopReq & s) {
    SnoopResp r{};  // has_shared=false, has_mod=false, inv_ack=false, rvalid=false

    // OJO: s.addr_line viene alineada a línea (bits [OFFSET_BITS-1:0] = 0)
    const auto a = splitAddress(s.addr_line);

    // Buscar la línea en mi caché
    auto& set = sets_[a.set];
    int way = findWay(a.set, a.tag);   // -1 si no está
    if (way < 0) return r;

    auto& line = set.ways[way];
    if (!line.valid || line.tag != a.tag) return r;

    // Presencia/dirty para el bus
    const bool isS = (line.state == Mesi::S);
    const bool isE = (line.state == Mesi::E);
    const bool isM = (line.state == Mesi::M);
    r.has_shared = (isS || isE || isM);   // cualquier copia marca shared
    r.has_mod = isM;                   // solo M marca HITM

    switch (s.cmd) {
    case BusCmd::BusRd: {
        // Lectura compartible: si yo tengo M y me piden datos -> servir si grant_data
        if (isM && s.grant_data) {
            r.rdata = line.data;
            r.rvalid = true;
        }
        // Degradaciones: M/E -> S
        if (isM || isE) line.state = Mesi::S;
    } break;

    case BusCmd::BusRdX: {
        // Read-for-ownership (RFO): invalidar a terceros; si yo tenía M y me piden datos, los doy
        if (isM && s.grant_data) {
            r.rdata = line.data;
            r.rvalid = true;
        }
        if (line.state != Mesi::I) {
            line.state = Mesi::I;
            line.valid = false;   // si manejás 'valid' aparte del estado
            r.inv_ack = true;    // confirmo invalidación
        }
    } break;

    case BusCmd::BusUpgr: {
        // Upgrade del solicitante (tenía S y quiere M): yo invalido si compartía
        if (isS || isE) {
            line.state = Mesi::I;
            line.valid = false;
            r.inv_ack = true;
        }
    } break;

    case BusCmd::WriteBack:
    default:
        // No acción necesaria en pares
        break;
    }

    return r;
}
