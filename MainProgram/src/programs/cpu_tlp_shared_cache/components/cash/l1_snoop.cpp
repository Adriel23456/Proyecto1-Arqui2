#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_utils.h"
#include "programs/cpu_tlp_shared_cache/components/cash/l1_snoop.h"

SnoopResp L1Cache::onSnoop(const SnoopReq& s) {
  SnoopResp r{};
  const uint64_t LINE_MASK = ~((1ULL << OFFSET_BITS) - 1ULL);
  const uint64_t aligned   = s.addr_line & LINE_MASK;
  const auto p             = splitAddress(aligned);
  if (s.from_self) return r;

  int way = findWay(p.set, p.tag);
  if (way < 0) return r;

  auto& line = sets_[p.set].ways[way];
  r.has_shared = (line.state == Mesi::S || line.state == Mesi::E || line.state == Mesi::M);
  r.has_mod    = (line.state == Mesi::M);

  switch (s.cmd) {
    case BusCmd::BusRd:
      if (line.state == Mesi::M && s.grant_data) {
        r.rdata = line.data; r.rvalid = true;
      }
      if (line.state == Mesi::M || line.state == Mesi::E) line.state = Mesi::S;
      break;

    case BusCmd::BusRdX:
      if ((line.state == Mesi::M || line.state == Mesi::S || line.state == Mesi::E) && s.grant_data) {
        r.rdata = line.data; r.rvalid = true;
      }
      line.state = Mesi::I; line.valid = false; r.inv_ack = true;
      break;

    case BusCmd::BusUpgr:
      if (line.state == Mesi::S || line.state == Mesi::E) {
        line.state = Mesi::I; line.valid = false; r.inv_ack = true;
      }
      break;

    case BusCmd::WriteBack:
    default:
      break;
  }
  return r;
}
