#pragma once
#include <thread>
#include <memory>
#include <functional>
#include <atomic>
#include <iostream>
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include "programs/cpu_tlp_shared_cache/components/bus/interconnect_bus.h"

namespace cpu_tlp {

    class InterconnectComponent {
    public:
        InterconnectComponent() = default;
        ~InterconnectComponent() { shutdown(); }

        // Crea el bus con `masters` y lo conecta a la RAM compartida
        bool initialize(std::shared_ptr<CPUSystemSharedData> sharedData, int masters);
        void shutdown();

        // Pasarelas para conectar L1s (nombres “compatibles” con tu código)
        MasterToBus* portOut(int id);  // L1 -> Bus
        BusToMaster* portIn(int id);  // Bus -> L1
        void setSnoopCallback(int id, std::function<SnoopResp(const SnoopReq&)> cb);

        // Acceso directo al bus si lo necesitás
        Interconnect* raw() { return m_bus.get(); }

    private:
        void threadMain();

    private:
        std::shared_ptr<CPUSystemSharedData> m_shared{};
        std::unique_ptr<Interconnect>        m_bus{};
        std::unique_ptr<std::thread>         m_thr{};
        std::atomic<bool>                    m_running{ false };
    };

} // namespace cpu_tlp
