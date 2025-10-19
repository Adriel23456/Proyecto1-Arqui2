#include "../include/programs/cpu_tlp_shared_cache/components/bus/InterconnectComponent.h"
#include <chrono>
#include <iostream>

namespace cpu_tlp {

    bool InterconnectComponent::initialize(std::shared_ptr<CPUSystemSharedData> sharedData, int masters) {
        if (m_running.load()) return false;
        if (!sharedData) return false;

        m_shared = std::move(sharedData);

        // Construir el bus con N masters (NO existe ic_.init(...))
        m_bus = std::make_unique<Interconnect>(masters);

        // Conectar RAM (RAMConnection del SharedMemoryComponent)
        m_bus->bindRAM(&m_shared->ram_connection);

        m_running.store(true, std::memory_order_release);
        m_thr = std::make_unique<std::thread>(&InterconnectComponent::threadMain, this);
        return true;
    }

    void InterconnectComponent::shutdown() {
        if (!m_running.exchange(false)) return;
        if (m_thr && m_thr->joinable()) m_thr->join();
        m_thr.reset();
        m_bus.reset();
    }

    MasterToBus* InterconnectComponent::portOut(int id) {
        return m_bus ? m_bus->portM2B(id) : nullptr;
    }
    BusToMaster* InterconnectComponent::portIn(int id) {
        return m_bus ? m_bus->portB2M(id) : nullptr;
    }
    void InterconnectComponent::setSnoopCallback(int id, std::function<SnoopResp(const SnoopReq&)> cb) {
        if (m_bus) m_bus->attachSnoopCallback(id, std::move(cb));
    }

    void InterconnectComponent::threadMain() {
        using namespace std::chrono_literals;
        while (m_running.load(std::memory_order_acquire) &&
            !m_shared->system_should_stop.load(std::memory_order_acquire)) {
            if (m_bus) m_bus->tick();   // arbitra, difunde snoops, C2C/DRAM, etc.
            std::this_thread::sleep_for(50us); // suaviza CPU
        }
    }

} // namespace cpu_tlp
