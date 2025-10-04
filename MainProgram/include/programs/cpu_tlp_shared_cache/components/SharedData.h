#pragma once
#include <atomic>
#include <cstdint>
#include <array>

namespace cpu_tlp {

    // Estructura para las conexiones de cada PE con la Instruction Memory
    struct PEInstructionConnection {
        // INS_READY: 1 bit - Escritura por InstructionMemory, Lectura por PE
        std::atomic<bool> INS_READY{ false };

        // InstrF: 64 bits - Escritura por InstructionMemory, Lectura por PE
        std::atomic<uint64_t> InstrF{ 0x0000000000000000ULL };

        // PC_F: 64 bits - Lectura por InstructionMemory, Escritura por PE
        std::atomic<uint64_t> PC_F{ 0x0000000000000000ULL };
    };

    // Contenedor para todas las conexiones de los 4 PEs
    struct InstructionMemorySharedData {
        std::array<PEInstructionConnection, 4> pe_connections;

        // Flag para indicar que el componente debe detenerse
        std::atomic<bool> should_stop{ false };

        // Flag para indicar que el componente está listo
        std::atomic<bool> component_ready{ false };
    };

} // namespace cpu_tlp