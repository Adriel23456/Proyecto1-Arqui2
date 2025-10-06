#pragma once
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include <thread>
#include <memory>
#include <vector>
#include <string>
#include <atomic>

namespace cpu_tlp {

    class InstructionMemoryComponent {
    public:
        InstructionMemoryComponent();
        ~InstructionMemoryComponent();

        // CAMBIO: Ahora recibe CPUSystemSharedData en lugar de InstructionMemorySharedData
        bool initialize(std::shared_ptr<CPUSystemSharedData> sharedData);

        void shutdown();
        bool isRunning() const;
        bool reloadInstructionMemory();
        void pauseProcessing();
        void resumeProcessing();

    private:
        void threadMain();
        uint64_t readInstructionFromFile(uint64_t address);
        bool loadInstructionMemory();
        void processPERequest(int peIndex);
        uint64_t bytesToUint64LittleEndian(const uint8_t* bytes);

    private:
        // CAMBIO: Ahora usa CPUSystemSharedData
        std::shared_ptr<CPUSystemSharedData> m_sharedData;
        std::unique_ptr<std::thread> m_executionThread;

        std::vector<uint8_t> m_instructionMemory;
        size_t m_memorySize;
        bool m_isRunning;
        std::atomic<bool> m_processingPaused;

        static constexpr uint64_t ERROR_INSTRUCTION = 0x4D00000000000000ULL;
        static constexpr const char* INSTRUCTION_FILE_PATH = "Assets/CPU_TLP/InstMem.bin";
    };

} // namespace cpu_tlp