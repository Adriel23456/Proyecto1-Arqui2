#include "programs/cpu_tlp_shared_cache/components/cash/L1Component.h"
#include <chrono>
#include <iostream>

namespace cpu_tlp {

    bool L1Component::initialize(std::shared_ptr<CPUSystemSharedData> sharedData, Interconnect* ic) {
        if (m_running.load()) {
            std::cerr << "[L1Cache" << m_id << "] Already running, cannot initialize again\n";
            return false;
        }
        if (!sharedData || !ic) {
            std::cerr << "[L1Cache" << m_id << "] Invalid parameters (sharedData or interconnect is null)\n";
            return false;
        }

        m_shared = std::move(sharedData);

        // 1) Conectar puertos del bus
        m_portOut = ic->portM2B(m_id);
        m_portIn = ic->portB2M(m_id);
        if (!m_portOut || !m_portIn) {
            std::cerr << "[L1Cache" << m_id << "] Failed to get bus ports\n";
            return false;
        }

        // 2) Adjuntar a la L1 y registrar snoop
        m_l1.attachBus(m_portOut, m_portIn, m_id);
        ic->attachSnoopCallback(m_id, [this](const SnoopReq& s) {
            return m_l1.onSnoop(s);
            });

        // 3) Reset local
        m_l1.reset();

        // 4) Lanzar hilo
        m_running.store(true, std::memory_order_release);
        m_thr = std::make_unique<std::thread>(&L1Component::threadMain, this);

        // ===== LOG DE INICIALIZACIÓN =====
        std::cout << "[L1Cache" << m_id << "] Initialized successfully\n";

        return true;
    }

    void L1Component::shutdown() {
        if (!m_running.exchange(false)) {
            return; // Ya estaba apagado
        }

        // ===== LOG DE INICIO DE SHUTDOWN =====
        std::cout << "[L1Cache" << m_id << "] Shutting down...\n";

        if (m_thr && m_thr->joinable()) {
            m_thr->join();
        }
        m_thr.reset();

        // ===== LOG DE SHUTDOWN COMPLETO =====
        std::cout << "[L1Cache" << m_id << "] Shutdown complete\n";
    }

    CpuReq L1Component::readCpuReq() {
        CpuReq r{};
        auto& c = m_shared->cache_connections[m_id];

        // Lectura con acquire: visibilidad de la petición del CPU
        r.ALUOut_M = c.ALUOut_M.load(std::memory_order_acquire);
        r.RD_Rm_Special_M = c.RD_Rm_Special_M.load(std::memory_order_acquire);
        r.C_WE_M = c.C_WE_M.load(std::memory_order_acquire);
        r.C_ISB_M = c.C_ISB_M.load(std::memory_order_acquire);
        r.C_REQUEST_M = c.C_REQUEST_M.load(std::memory_order_acquire);
        r.C_READY_ACK = c.C_READY_ACK.load(std::memory_order_acquire);
        return r;
    }

    void L1Component::writeCpuResp(const CpuResp& r) {
        auto& c = m_shared->cache_connections[m_id];
        // Publicación con release: el CPU verá dato listo + ready de forma atómica
        c.RD_C_out.store(r.RD_C_out, std::memory_order_release);
        c.C_READY.store(r.C_READY, std::memory_order_release);
    }

    void L1Component::threadMain() {
        using namespace std::chrono_literals;

        while (m_running.load(std::memory_order_acquire) &&
            !m_shared->system_should_stop.load(std::memory_order_acquire))
        {
            // 1) samplear petición de CPU
            const CpuReq req = readCpuReq();

            // 2) levantar/bajar FSM (tu L1 ya maneja el handshake C_READY/ACK inside)
            m_l1.beginAccess(req);
            m_l1.tick();

            // 3) publicar salida
            writeCpuResp(m_l1.output());

            // Evitar busy-wait extremo, sin perder reactividad
            std::this_thread::sleep_for(100us);
        }
    }

} // namespace cpu_tlp