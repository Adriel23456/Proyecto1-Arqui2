//SharedMemoryComponent.cpp
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

        m_sharedData = std::move(sharedData);

        // Log de tamaño
        log_build_and_print([&](std::ostringstream& oss) {
            oss << "[SharedMemory] Initializing RAM with "
                << SharedMemory::MEM_SIZE_BYTES << " bytes ("
                << SharedMemory::MEM_SIZE_WORDS << " words of 64 bits)\n";
            });

        // Poner el canal RAMConnection en estado conocido
        if (m_sharedData) {
            auto& R = m_sharedData->ram_connection;
            for (int pe = 0; pe < 4; ++pe) {
                auto& C = m_sharedData->cache_connections[pe];
                C.RD_C_out.store(0, std::memory_order_release);
                C.C_READY.store(false, std::memory_order_release);
            }
            R.request_active.store(false, std::memory_order_release);
            R.response_ready.store(false, std::memory_order_release);
            R.write_enable.store(false, std::memory_order_release);
            R.request_address.store(0, std::memory_order_release);
            R.write_data.store(0, std::memory_order_release);
            R.read_data.store(0, std::memory_order_release);
        }

        m_isRunning = true;
        m_sharedData->system_should_stop.store(false, std::memory_order_release);

        m_executionThread = std::make_unique<std::thread>(&SharedMemoryComponent::threadMain, this);

        std::cout << "[SharedMemory] Component initialized successfully" << std::endl;
        return true;
    }

    void SharedMemoryComponent::shutdown() {
        if (!m_isRunning) return;
        std::cout << "[SharedMemory] Shutting down...\n";

        if (m_sharedData) {
            m_sharedData->system_should_stop.store(true, std::memory_order_release); // ← en vez de '='
        }

        if (m_executionThread && m_executionThread->joinable()) {
            m_executionThread->join();
        }
        m_isRunning = false;
        m_executionThread.reset();
        std::cout << "[SharedMemory] Shutdown complete\n";
    }


    bool SharedMemoryComponent::isRunning() const {
        return m_isRunning;
    }

    void SharedMemoryComponent::threadMain() {
        using namespace std::chrono_literals;
        std::cout << "[SharedMemory] Thread started\n";

        auto& R = m_sharedData->ram_connection;

        while (!m_sharedData->system_should_stop.load(std::memory_order_acquire)) {

            // Únicamente backend DRAM vía RAMConnection (llamado por el Interconnect)
            if (R.request_active.load(std::memory_order_acquire)) {
                if (!R.response_ready.load(std::memory_order_acquire)) {
                    const uint16_t addr = R.request_address.load(std::memory_order_acquire);
                    if (R.write_enable.load(std::memory_order_acquire)) {
                        const uint64_t w = R.write_data.load(std::memory_order_acquire);
                        m_memory.write(addr, w);
                        R.response_ready.store(true, std::memory_order_release);
                    }
                    else {
                        const uint64_t val = m_memory.read(addr);
                        R.read_data.store(val, std::memory_order_release);
                        R.response_ready.store(true, std::memory_order_release);
                    }
                    // request_active lo limpia el Interconnect al consumir response_ready
                }
            }
            else {
                std::this_thread::sleep_for(5us);
            }
        }

        std::cout << "[SharedMemory] Thread ending\n";
    }
}