#pragma once
#include <thread>
#include <memory>
#include <atomic>
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"

// Cache y bus (ya existen en tu repo)
#include "programs/cpu_tlp_shared_cache/components/cash/l1_cash.h"
#include "programs/cpu_tlp_shared_cache/components/bus/interconnect_bus.h"

namespace cpu_tlp {

    class L1Component {
    public:
        explicit L1Component(int id) : m_id(id) {}
        ~L1Component() { shutdown(); }

        // interconnect debe estar YA creado; este método ata puertos y lanza el hilo
        bool initialize(std::shared_ptr<CPUSystemSharedData> sharedData,
            Interconnect* ic);
        void shutdown();

        // acceso (debug/tests)
        L1Cache* l1() { return &m_l1; }

    private:
        void threadMain();

        // Helpers: traducen atomics ⇄ structs
        CpuReq  readCpuReq();
        void    writeCpuResp(const CpuResp& r);

    private:
        int m_id{ -1 };
        std::shared_ptr<CPUSystemSharedData> m_shared{};
        std::unique_ptr<std::thread> m_thr{};
        std::atomic<bool> m_running{ false };

        // núcleo de la caché
        L1Cache m_l1;

        // puertos del bus (propiedad del interconnect)
        MasterToBus* m_portOut{ nullptr }; // L1 → bus
        BusToMaster* m_portIn{ nullptr }; // bus → L1
    };

} // namespace cpu_tlp
