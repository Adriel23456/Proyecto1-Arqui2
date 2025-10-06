#pragma once
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include <thread>
#include <memory>
#include <cstdint>
#include <array>
#include <atomic>
#include <functional>
#include <string>

namespace cpu_tlp {

    class PE0Component {
    public:
        PE0Component(int pe_id = 0);
        ~PE0Component();

        bool initialize(std::shared_ptr<CPUSystemSharedData> sharedData);
        void shutdown();

        // Comandos de control
        void step();
        void stepUntil(int value);
        void stepIndefinitely();
        void stopExecution();
        void reset();

        bool isRunning() const;

    private:
        void threadMain();
        void executeCycle();

        // ============ ESTRUCTURAS INTERNAS ============

        // FlipFlops entre etapas
        struct FetchDecodeReg {
            uint64_t Instr_F;
            uint64_t PC_F;
        };

        struct DecodeExecuteReg {
            uint8_t RegWrite_D;
            uint8_t MemOp_D;
            uint8_t C_WE_D;
            uint8_t C_REQUEST_D;
            uint8_t C_ISB_D;
            uint8_t PCSrc_D;
            uint8_t FlagsUpd_D;
            uint8_t ALUControl_D;
            uint8_t BranchOp_D;
            uint8_t Flags_in;  // Flags' desde Execute
            uint64_t SrcA_D;
            uint64_t SrcB_D;
            uint64_t RD_Rm_out;
            uint8_t Rd_in_D;
            // NUEVO: instrucción que entra a Execute
            uint64_t Instr_D = 0x4D00000000000000ULL; // NOP
        };

        struct ExecuteMemoryReg {
            uint8_t RegWrite_E;
            uint8_t MemOp_E;
            uint8_t C_WE_E;
            uint8_t C_REQUEST_E;
            uint8_t C_ISB_E;
            uint8_t PCSrc_AND;
            uint64_t ALUResult_E;
            uint64_t RD_Rm_Special_E;
            uint8_t Rd_in_E;
            // NUEVO: instrucción que entra a Memory
            uint64_t Instr_E = 0x4D00000000000000ULL; // NOP
        };

        struct MemoryWriteBackReg {
            uint8_t RegWrite_M;
            uint8_t PCSrc_M;
            uint64_t ALUOutM_O;
            uint8_t Rd_in_M;
            // NUEVO: instrucción que entra a WriteBack
            uint64_t Instr_M = 0x4D00000000000000ULL; // NOP
        };

        // ============ UNIDADES FUNCIONALES ============

        class RegisterFile {
        public:
            RegisterFile();
            void reset();
            uint64_t read(uint8_t addr) const;
            void write(uint8_t addr, uint64_t value, bool we);
            uint64_t getUpper() const { return regs[10]; }
            uint64_t getLower() const { return regs[11]; }
            void setPEID(int pe_id) { regs[9] = pe_id; }
            std::function<void(uint8_t addr, uint64_t value)> onRegisterWrite;
        private:
            std::array<uint64_t, 12> regs;
        };

        class ALU {
        public:
            struct Result {
                uint64_t value;
                uint8_t flags; // NZCV
            };
            Result execute(uint8_t control, uint64_t A, uint64_t B, uint8_t flagsIn);
        };

        class ControlUnit {
        public:
            struct Signals {
                uint8_t RegWrite_D;
                uint8_t MemOp_D;
                uint8_t C_WE_D;
                uint8_t C_REQUEST_D;
                uint8_t C_ISB_D;
                uint8_t PCSrc_D;
                uint8_t FlagsUpd_D;
                uint8_t ALUControl_D;
                uint8_t BranchOp_D;
                uint8_t BranchE;
                uint8_t ImmOp;
                uint8_t DataType;
            };
            Signals decode(uint8_t opcode);
        };

        class HazardUnit {
        public:
            struct Outputs {
                bool StallF, StallD, FlushD, StallE, FlushE, StallM, StallW;
            };

            Outputs detect(
                bool INS_READY,
                bool C_REQUEST_M, bool C_READY,
                bool SegmentationFault,
                bool PCSrc_AND,
                bool PCSrc_W,
                uint8_t Rd_in_D, uint8_t Rn_in, uint8_t Rm_in,
                uint8_t Rd_in_E, uint8_t Rd_in_M, uint8_t Rd_in_W,
                bool RegWrite_E, bool RegWrite_M, bool RegWrite_W,
                bool BranchE,
                uint8_t BranchOp_E
            );
        private:
            int branchCycles = 0;
            bool branchActive = false;
            bool branchWaitingForW = false;
        };

        // ============ ETAPAS DEL PIPELINE ============
        void stageFetch();
        void stageDecode();
        void stageExecute();
        void stageMemory();
        void stageWriteBack();

        // ============ HELPERS ============
        uint64_t extendImmediate(uint32_t imm, bool dataType);
        bool evaluateBranchCondition(uint8_t branchOp, uint8_t flags);

        // ============ MIEMBROS DE DATOS ============
        int m_pe_id;
        std::shared_ptr<CPUSystemSharedData> m_sharedData;
        std::unique_ptr<std::thread> m_executionThread;

        // Estado del CPU
        uint64_t PC_F;
        uint64_t PCPlus8_F;

        // Registros de pipeline
        FetchDecodeReg IF_ID, IF_ID_next;
        DecodeExecuteReg ID_EX, ID_EX_next;
        ExecuteMemoryReg EX_MEM, EX_MEM_next;
        MemoryWriteBackReg MEM_WB, MEM_WB_next;

        // Señales intermedias (Decode)
        uint64_t InstrD;
        uint64_t PC_in;
        uint8_t Op_in, Rn_in, Rm_in, Rd_in_D;
        uint32_t Imm_in;
        uint64_t RD_Rn_out, RD_Rm_out, UPPER_out, LOWER_out;
        uint64_t Imm_ext_in;
        bool SegmentationFault;
        uint64_t SrcA_D, SrcB_D;
        ControlUnit::Signals ctrlSignals;

        // Señales intermedias (Execute)
        uint8_t ALUFlagsOut;
        uint64_t ALUResult_E;
        uint8_t Flags_prime;  // Flags'
        bool CondExE;
        bool PCSrc_AND;

        // Señales intermedias (Memory)
        uint64_t ALUOutM_O;

        // Señales intermedias (WriteBack)
        uint64_t PC_prime;

        // Unidades
        RegisterFile m_registerFile;
        ALU m_alu;
        ControlUnit m_controlUnit;
        HazardUnit m_hazardUnit;
        HazardUnit::Outputs m_hazards;

        // Estado interno
        std::atomic<bool> m_isRunning;
        bool m_segmentationFault;

        // Constantes
        static constexpr uint64_t NOP_INSTRUCTION = 0x4D00000000000000ULL;
        static constexpr uint64_t FLUSH_INSTRUCTION = 0x4D00000000000001ULL;

        // Tracking de instrucciones por etapa
        std::array<uint64_t, 5> m_stageInstructions;

        // Helper para actualizar tracking
        void updateInstructionTracking();
    };

} // namespace cpu_tlp