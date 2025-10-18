#include "programs/cpu_tlp_shared_cache/components/SharedMemoryComponent.h"
#include "programs/cpu_tlp_shared_cache/widgets/Log.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace cpu_tlp {

    SharedMemoryComponent::SharedMemoryComponent()
        : m_sharedData(nullptr)
        , m_executionThread(nullptr)
        , m_isRunning(false)
    {
    }

    SharedMemoryComponent::~SharedMemoryComponent() {
        shutdown();
    }

    bool SharedMemoryComponent::initialize(std::shared_ptr<CPUSystemSharedData> sharedData) {
        if (m_isRunning) {
            std::cerr << "[SharedMemory] Component already running!" << std::endl;
            return false;
        }

        m_sharedData = sharedData;

        // Inicializar la memoria RAM (4096 bytes)
        log_build_and_print([&](std::ostringstream& oss) {
            oss << "[SharedMemory] Initializing RAM with "
                << SharedMemory::MEM_SIZE_BYTES << " bytes ("
                << SharedMemory::MEM_SIZE_WORDS << " words of 64 bits)\n";
            });

        m_isRunning = true;
        m_sharedData->system_should_stop = false;

        // Lanzar hilo de ejecución
        m_executionThread = std::make_unique<std::thread>(&SharedMemoryComponent::threadMain, this);

        std::cout << "[SharedMemory] Component initialized successfully" << std::endl;
        return true;
    }

    void SharedMemoryComponent::shutdown() {
        if (!m_isRunning) return;

        std::cout << "[SharedMemory] Shutting down..." << std::endl;

        if (m_sharedData) {
            m_sharedData->system_should_stop = true;
        }

        if (m_executionThread && m_executionThread->joinable()) {
            m_executionThread->join();
        }

        m_isRunning = false;
        m_executionThread.reset();

        std::cout << "[SharedMemory] Shutdown complete" << std::endl;
    }

    bool SharedMemoryComponent::isRunning() const {
        return m_isRunning;
    }

    void SharedMemoryComponent::threadMain() {
        std::cout << "[SharedMemory] Thread started" << std::endl;

        // Este componente permanece activo respondiendo a solicitudes
        // de lectura/escritura de memoria de los PEs a través de la cache
        while (!m_sharedData->system_should_stop.load(std::memory_order_acquire)) {
            // El trabajo real se hace a través de los métodos read/write/get/set
            // que son llamados por otros componentes de forma thread-safe
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[SharedMemory] Thread ending" << std::endl;
    }

} // namespace cpu_tlp