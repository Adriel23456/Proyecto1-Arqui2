#pragma once
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include <thread>
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <array>  // AÑADIR ESTE INCLUDE

namespace cpu_tlp {

    class InstructionMemoryComponent {
    public:
        InstructionMemoryComponent();
        ~InstructionMemoryComponent();

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
        std::shared_ptr<CPUSystemSharedData> m_sharedData;
        std::unique_ptr<std::thread> m_executionThread;

        std::vector<uint8_t> m_instructionMemory;
        size_t m_memorySize;
        bool m_isRunning;
        std::atomic<bool> m_processingPaused;

        // AÑADIR ESTA LÍNEA: Convertir lastPC en variable miembro
        std::array<uint64_t, 4> m_lastPC;

        static constexpr uint64_t ERROR_INSTRUCTION = 0x4D00000000000000ULL;
        static constexpr const char* INSTRUCTION_FILE_PATH = "Assets/CPU_TLP/InstMem.bin";
    };

} // namespace cpu_tlp