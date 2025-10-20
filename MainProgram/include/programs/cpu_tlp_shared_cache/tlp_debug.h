#pragma once
#include <iostream>
#include <iomanip>

// Activa/desactiva prints en build (-DTLP_DEBUG en el compilador)
#ifndef TLP_DEBUG
#define TLP_DEBUG 1
#endif

#if TLP_DEBUG
#define DBG_PE(pe, msg)   do{ std::cout << "[PE"  << (pe) << "] " << msg << std::endl; }while(0)
#define DBG_L1(id, msg)   do{ std::cout << "[L1"  << (id) << "] " << msg << std::endl; }while(0)
#define DBG_BUS(msg)      do{ std::cout << "[BUS] "         << msg << std::endl; }while(0)
#define DBG_SM(msg)       do{ std::cout << "[DRAM] "        << msg << std::endl; }while(0)
#else
#define DBG_PE(pe, msg)   do{}while(0)
#define DBG_L1(id, msg)   do{}while(0)
#define DBG_BUS(msg)      do{}while(0)
#define DBG_SM(msg)       do{}while(0)
#endif

// helpers bonitos
#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"

inline const char* mesiName(Mesi s) {
  switch (s) { case Mesi::I: return "I"; case Mesi::S: return "S"; case Mesi::E: return "E"; case Mesi::M: return "M"; }
                           return "?";
}
inline const char* cmdName(BusCmd c) {
    switch (c) {
    case BusCmd::BusRd:    return "BusRd";
    case BusCmd::BusRdX:   return "BusRdX";
    case BusCmd::BusUpgr:  return "BusUpgr";
    case BusCmd::WriteBack:return "WriteBack";
    } return "?";
}
