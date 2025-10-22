#pragma once
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include "programs/cpu_tlp_shared_cache/components/SharedMemory.h"
#include <memory>
#include <thread>
#include <atomic>

namespace cpu_tlp {

    // Backend DRAM determinístico con handshake por flanco:
    // - Latch de request en rising edge de request_active
    // - Servicio único por solicitud
    // - Publica response_ready (release)
    // - Espera a que Interconnect limpie response_ready y baje request_active
    class SharedMemoryComponent {
    public:
        SharedMemoryComponent();
        ~SharedMemoryComponent();

        // Lifecycle
        bool initialize(std::shared_ptr<CPUSystemSharedData> sharedData);
        void shutdown();
        bool isRunning() const;

        // Acceso a la memoria (thread-safe)
        SharedMemory& getMemory() { return m_memory; }
        const SharedMemory& getMemory() const { return m_memory; }

    private:
        void threadMain();

        std::shared_ptr<CPUSystemSharedData> m_sharedData;
        std::unique_ptr<std::thread> m_executionThread;

        SharedMemory m_memory;
        std::atomic<bool> m_isRunning;
    };

} // namespace cpu_tlp
