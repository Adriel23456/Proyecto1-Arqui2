#include "programs/cpu_tlp_shared_cache/components/InstructionMemoryComponent.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <array>

namespace cpu_tlp {

    InstructionMemoryComponent::InstructionMemoryComponent()
        : m_sharedData(nullptr)
        , m_executionThread(nullptr)
        , m_memorySize(0)
        , m_isRunning(false)
        , m_processingPaused(false) {
    }

    InstructionMemoryComponent::~InstructionMemoryComponent() {
        shutdown();
    }

    bool InstructionMemoryComponent::initialize(std::shared_ptr<CPUSystemSharedData> sharedData) {
        if (m_isRunning) {
            std::cerr << "[InstructionMemory] Component already running!" << std::endl;
            return false;
        }

        m_sharedData = sharedData;

        if (!loadInstructionMemory()) {
            std::cerr << "[InstructionMemory] Failed to load instruction memory file" << std::endl;
            return false;
        }

        m_isRunning = true;
        m_sharedData->system_should_stop = false;
        m_executionThread = std::make_unique<std::thread>(&InstructionMemoryComponent::threadMain, this);

        std::cout << "[InstructionMemory] Component initialized successfully" << std::endl;
        return true;
    }

    void InstructionMemoryComponent::shutdown() {
        if (!m_isRunning) return;

        std::cout << "[InstructionMemory] Shutting down..." << std::endl;

        if (m_sharedData) {
            m_sharedData->system_should_stop = true;
        }

        if (m_executionThread && m_executionThread->joinable()) {
            m_executionThread->join();
        }

        m_isRunning = false;
        m_executionThread.reset();

        std::cout << "[InstructionMemory] Shutdown complete" << std::endl;
    }

    bool InstructionMemoryComponent::isRunning() const {
        return m_isRunning;
    }

    bool InstructionMemoryComponent::loadInstructionMemory() {
        std::string fullPath = std::string(RESOURCES_PATH) + INSTRUCTION_FILE_PATH;
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            std::cerr << "[InstructionMemory] Could not open file: " << fullPath << std::endl;
            m_instructionMemory.clear();
            m_memorySize = 0;
            return true;
        }

        m_memorySize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        m_instructionMemory.resize(m_memorySize);
        file.read(reinterpret_cast<char*>(m_instructionMemory.data()), m_memorySize);
        file.close();

        std::cout << "[InstructionMemory] Loaded " << m_memorySize << " bytes from " << INSTRUCTION_FILE_PATH << std::endl;
        return true;
    }

    bool InstructionMemoryComponent::reloadInstructionMemory() {
        std::cout << "[InstructionMemory] Reloading instruction memory..." << std::endl;

        pauseProcessing();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        bool success = loadInstructionMemory();

        if (success) {
            for (int i = 0; i < 4; ++i) {
                auto& connection = m_sharedData->instruction_connections[i];
                connection.PC_F.store(0x0000000000000000ULL, std::memory_order_release);
                connection.INS_READY.store(false, std::memory_order_release);
            }
            std::cout << "[InstructionMemory] Reload successful" << std::endl;
        }
        else {
            std::cerr << "[InstructionMemory] Reload failed" << std::endl;
        }

        resumeProcessing();
        return success;
    }

    void InstructionMemoryComponent::pauseProcessing() {
        m_processingPaused.store(true, std::memory_order_release);
    }

    void InstructionMemoryComponent::resumeProcessing() {
        m_processingPaused.store(false, std::memory_order_release);
    }

    uint64_t InstructionMemoryComponent::bytesToUint64LittleEndian(const uint8_t* bytes) {
        uint64_t result = 0;
        for (int i = 0; i < 8; ++i) {
            result |= (static_cast<uint64_t>(bytes[i]) << (i * 8));
        }
        return result;
    }

    uint64_t InstructionMemoryComponent::readInstructionFromFile(uint64_t address) {
        if (address + 8 > m_memorySize) {
            return ERROR_INSTRUCTION;
        }

        const uint8_t* instructionBytes = &m_instructionMemory[address];
        return bytesToUint64LittleEndian(instructionBytes);
    }

    void InstructionMemoryComponent::processPERequest(int peIndex) {
        auto& connection = m_sharedData->instruction_connections[peIndex];
        uint64_t currentPC = connection.PC_F.load(std::memory_order_acquire);

        static std::array<uint64_t, 4> lastPC = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
                                                  0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL };

        if (currentPC != lastPC[peIndex]) {
            std::cout << "  [InstMem] PE" << peIndex << " DETECTED PC change: "
                << std::hex << lastPC[peIndex] << " -> " << currentPC << std::dec << "\n";

            connection.INS_READY.store(false, std::memory_order_release);

            // Leer instrucción
            uint64_t instruction = readInstructionFromFile(currentPC);

            // Escribir instrucción
            connection.InstrF.store(instruction, std::memory_order_release);

            connection.INS_READY.store(true, std::memory_order_release);

            lastPC[peIndex] = currentPC;

            std::cout << "[InstructionMemory] PE" << peIndex
                << " - PC: 0x" << std::hex << currentPC
                << " - Instruction: 0x" << instruction << std::dec << std::endl;
        }
    }

    void InstructionMemoryComponent::threadMain() {
        std::cout << "[InstructionMemory] Thread started" << std::endl;

        // Procesar inicialmente todas las direcciones 0x0
        for (int i = 0; i < 4; ++i) {
            processPERequest(i);
        }

        while (!m_sharedData->system_should_stop.load(std::memory_order_acquire)) {
            if (m_processingPaused.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Procesar todos los PEs
            for (int i = 0; i < 4; ++i) {
                processPERequest(i);
            }

            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        std::cout << "[InstructionMemory] Thread ending" << std::endl;
    }

} // namespace cpu_tlp