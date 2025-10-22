#include "programs/cpu_tlp_shared_cache/components/SharedMemoryComponent.h"
#include "programs/cpu_tlp_shared_cache/widgets/Log.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace cpu_tlp {

    using std::memory_order_acquire;
    using std::memory_order_release;
    using std::memory_order_relaxed;

    static inline void dbg_ram(const char* msg, uint16_t addr, bool we, uint64_t w, uint64_t r) {
#if 1
        std::cout << "[DRAM] " << msg
            << " addr=0x" << std::hex << (unsigned)addr
            << " we=" << std::dec << (we ? 1 : 0)
            << " w=0x" << std::hex << w
            << " r=0x" << std::hex << r
            << std::dec << "\n";
#endif
    }

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

        // Inicialización determinista recomendada (útil para tus tests)
        m_memory.reset();
        m_memory.initTestPattern_0_1_2_3();

        // Poner el canal RAMConnection en estado conocido
        if (m_sharedData) {
            auto& R = m_sharedData->ram_connection;
            for (int pe = 0; pe < 4; ++pe) {
                auto& C = m_sharedData->cache_connections[pe];
                C.RD_C_out.store(0, memory_order_release);
                C.C_READY.store(false, memory_order_release);
            }
            R.request_active.store(false, memory_order_release);
            R.response_ready.store(false, memory_order_release);
            R.write_enable.store(false, memory_order_release);
            R.request_address.store(0, memory_order_release);
            R.write_data.store(0, memory_order_release);
            R.read_data.store(0, memory_order_release);
        }

        m_isRunning = true;
        m_sharedData->system_should_stop.store(false, memory_order_release);

        m_executionThread = std::make_unique<std::thread>(&SharedMemoryComponent::threadMain, this);

        std::cout << "[SharedMemory] Component initialized successfully" << std::endl;
        return true;
    }

    void SharedMemoryComponent::shutdown() {
        if (!m_isRunning) return;
        std::cout << "[SharedMemory] Shutting down...\n";

        if (m_sharedData) {
            m_sharedData->system_should_stop.store(true, memory_order_release);
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

    // FSM del backend DRAM (determinístico):
    //  - Espera flanco de subida de request_active
    //  - Latch de address/write_enable/write_data UNA sola vez
    //  - Servicio (read/write) UNA sola vez
    //  - Publica response_ready=1 (release)
    //  - Espera a que Interconnect consuma (response_ready=0) y baje request_active=0
    void SharedMemoryComponent::threadMain() {
        using namespace std::chrono_literals;
        std::cout << "[SharedMemory] Thread started\n";

        auto& R = m_sharedData->ram_connection;

        while (!m_sharedData->system_should_stop.load(std::memory_order_acquire)) {
            // Handshake por NIVEL:
            // - Si hay request_active==1 y response_ready==0 -> SERVIR UNA VEZ
            // - Interconnect bajará response_ready=0 + request_active=0 tras consumir
            const bool req = R.request_active.load(std::memory_order_acquire);
            const bool rsp = R.response_ready.load(std::memory_order_acquire);

            if (req && !rsp) {
                const uint16_t addr = R.request_address.load(std::memory_order_acquire);
                const bool     we = R.write_enable.load(std::memory_order_acquire);
                const uint64_t w = R.write_data.load(std::memory_order_acquire);

                uint64_t rval = 0;
                if (we) {
                    m_memory.write(addr, w);
                    rval = w; // opcional: eco en lecturas de depuración
                }
                else {
                    rval = m_memory.read(addr);
                }

                // Publicar payload y luego el flag (release)
                R.read_data.store(rval, std::memory_order_release);
                R.response_ready.store(true, std::memory_order_release);

                dbg_ram("SERVE", addr, we, w, rval);
            }
            else {
                std::this_thread::sleep_for(3us);
            }
        }

        std::cout << "[SharedMemory] Thread ending\n";
    }

} // namespace cpu_tlp
