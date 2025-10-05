#pragma once
#include <atomic>
#include <cstdint>
#include <array>

namespace cpu_tlp {

    // Estructura existente para las conexiones de cada PE con la Instruction Memory
    struct PEInstructionConnection {
        // INS_READY: 1 bit - Escritura por InstructionMemory, Lectura por PE
        std::atomic<bool> INS_READY{ false };

        // InstrF: 64 bits - Escritura por InstructionMemory, Lectura por PE
        std::atomic<uint64_t> InstrF{ 0x0000000000000000ULL };

        // PC_F: 64 bits - Lectura por InstructionMemory, Escritura por PE
        std::atomic<uint64_t> PC_F{ 0x0000000000000000ULL };
    };

    // NUEVA: Estructura para las conexiones de cada PE con la Cache Privada
    struct PECacheConnection {
        // De PE a Cache
        std::atomic<uint64_t> ALUOut_M{ 0x0000000000000000ULL };
        std::atomic<uint64_t> RD_Rm_Special_M{ 0x0000000000000000ULL };
        std::atomic<bool> C_WE_M{ false };
        std::atomic<bool> C_ISB_M{ false };
        std::atomic<bool> C_REQUEST_M{ false };

        // De Cache a PE
        std::atomic<uint64_t> RD_C_out{ 0x0000000000000000ULL };
        std::atomic<bool> C_READY{ false };
    };

    // NUEVA: Estructura de control del PE
    struct PEControlSignals {
        std::atomic<int> command{ 0 }; // 0=idle, 1=step, 2=step_until, 3=step_infinite, 4=reset
        std::atomic<int> step_count{ 0 }; // Para step_until
        std::atomic<bool> running{ false };
        std::atomic<bool> should_stop{ false };
    };

    // Contenedor actualizado para todas las conexiones
    struct InstructionMemorySharedData {
        std::array<PEInstructionConnection, 4> pe_connections;
        std::atomic<bool> should_stop{ false };
        std::atomic<bool> component_ready{ false };
    };

    // NUEVA: Estructura completa de datos compartidos del sistema
    struct CPUSystemSharedData {
        // Conexiones con Instruction Memory
        std::array<PEInstructionConnection, 4> instruction_connections;

        // Conexiones con Cache
        std::array<PECacheConnection, 4> cache_connections;

        // Control de cada PE
        std::array<PEControlSignals, 4> pe_control;

        // Control global del sistema
        std::atomic<bool> system_should_stop{ false };
    };

} // namespace cpu_tlp