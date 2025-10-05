#pragma once
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include <thread>
#include <memory>
#include <cstdint>
#include <array>

namespace cpu_tlp {

    // Forward declarations
    class PipelineStage;
    class HazardUnit;

    class PE0Component {
    public:
        PE0Component(int pe_id = 0);
        ~PE0Component();

        // Inicializa el componente y comienza la ejecución asíncrona
        bool initialize(std::shared_ptr<CPUSystemSharedData> sharedData);

        // Detiene el componente y espera a que termine el hilo
        void shutdown();

        // Comandos de control
        void step();
        void stepUntil(int value);
        void stepIndefinitely();
        void stopExecution();
        void reset();

        // Verifica si el componente está corriendo
        bool isRunning() const;

    private:
        // Función principal del hilo de ejecución
        void threadMain();

        // Ejecuta un ciclo del pipeline
        void executeCycle();

        // Estructura del pipeline
        struct Pipeline {
            // Registros entre etapas (FlipFlops)
            struct IFIDReg {
                uint64_t PC;
                uint64_t Instruction;
                bool valid;
            };

            struct IDEXReg {
                uint64_t PC;
                uint64_t A;
                uint64_t B;
                uint64_t Imm;
                uint8_t Rd;
                uint8_t ALUControl;
                uint8_t MemOp;
                bool RegWrite;
                bool MemWrite;
                bool Branch;
                uint8_t BranchOp;
                bool ImmOp;
                bool FlagsUpd;
                bool valid;
            };

            struct EXMEMReg {
                uint64_t ALUResult;
                uint64_t B;
                uint8_t Rd;
                uint8_t MemOp;
                bool RegWrite;
                bool MemWrite;
                bool PCSrc;
                bool valid;
            };

            struct MEMWBReg {
                uint64_t ALUResult;
                uint64_t MemData;
                uint8_t Rd;
                bool RegWrite;
                bool MemToReg;
                bool PCSrc;
                bool valid;
            };

            // Registros de pipeline
            IFIDReg IF_ID, next_IF_ID;
            IDEXReg ID_EX, next_ID_EX;
            EXMEMReg EX_MEM, next_EX_MEM;
            MEMWBReg MEM_WB, next_MEM_WB;
        };

        // Etapas del pipeline
        void stageFetch();
        void stageDecode();
        void stageExecute();
        void stageMemory();
        void stageWriteBack();

        // Unidades funcionales
        class ALU {
        public:
            uint64_t execute(uint8_t control, uint64_t A, uint64_t B, uint8_t& flags);
        };

        class ControlUnit {
        public:
            void decode(uint8_t opcode, uint8_t special);

            // Señales de control
            bool RegWriteS, RegWriteR;
            uint8_t MemOp;
            bool MemWriteG, MemWriteD, MemWriteV, MemWriteP;
            bool MemByte;
            bool PCSrc;
            uint8_t ALUSrc;
            uint8_t PrintEn;
            bool ComS;
            bool LogOut;
            bool BranchE;
            bool FlagsUpd;
            uint8_t BranchOp;
            bool ImmediateOp;
            uint8_t RegisterInB;
            bool RegisterInA;
        };

        // Register File de 12 registros
        class RegisterFile {
        public:
            RegisterFile();
            void reset();
            uint64_t read(uint8_t addr);
            void write(uint8_t addr, uint64_t value, bool we);

        private:
            std::array<uint64_t, 12> regs;
            static constexpr uint64_t INITIAL_LOWER = 0xFFFFFFFFFFFFFFFFULL;
        };

        // Hazard Unit
        class HazardUnit {
        public:
            void checkHazards(const Pipeline& pipeline);

            // Señales de control de hazards
            bool StallF, StallD, StallE, StallM, StallW;
            bool FlushD, FlushE;

        private:
            bool detectRAW(uint8_t rs, uint8_t rt, const Pipeline& pipeline);
            bool detectBranch(const Pipeline& pipeline);
            bool detectCacheStall(const Pipeline& pipeline);
        };

    private:
        // Miembros de datos
        int m_pe_id;
        std::shared_ptr<CPUSystemSharedData> m_sharedData;
        std::unique_ptr<std::thread> m_executionThread;

        // Estado del pipeline
        Pipeline m_pipeline;

        // Unidades funcionales
        ALU m_alu;
        ControlUnit m_controlUnit;
        RegisterFile m_registerFile;
        HazardUnit m_hazardUnit;

        // Program Counter
        uint64_t m_PC;
        uint64_t m_nextPC;

        // Flags (NZCV)
        uint8_t m_flags;

        // Estado interno
        bool m_isRunning;
        bool m_segmentationFault;

        // Constantes
        static constexpr uint64_t NOP_INSTRUCTION = 0x4D00000000000000ULL;
        static constexpr int MAX_CYCLES_PER_STEP = 1000000;
    };

} // namespace cpu_tlp