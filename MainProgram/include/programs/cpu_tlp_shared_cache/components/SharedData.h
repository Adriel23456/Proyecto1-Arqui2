#pragma once
#include <atomic>
#include <cstdint>
#include <array>

namespace cpu_tlp {

    struct PEInstructionConnection {
        std::atomic<bool> INS_READY{ false };
        std::atomic<uint64_t> InstrF{ 0x0000000000000000ULL };
        std::atomic<uint64_t> PC_F{ 0x0000000000000000ULL };
    };

    struct PECacheConnection {
        std::atomic<uint64_t> ALUOut_M{ 0x0000000000000000ULL };
        std::atomic<uint64_t> RD_Rm_Special_M{ 0x0000000000000000ULL };
        std::atomic<bool> C_WE_M{ false };
        std::atomic<bool> C_ISB_M{ false };
        std::atomic<bool> C_REQUEST_M{ false };
        std::atomic<uint64_t> RD_C_out{ 0x0000000000000000ULL };
        std::atomic<bool> C_READY{ false };
    };

    struct PEControlSignals {
        std::atomic<int> command{ 0 };
        std::atomic<int> step_count{ 0 };
        std::atomic<bool> running{ false };
        std::atomic<bool> should_stop{ false };
    };

    // NUEVO: Snapshot de registros para visualización (thread-safe)
    struct PERegisterSnapshot {
        static constexpr int REG_COUNT = 12;
        std::array<std::atomic<uint64_t>, REG_COUNT> registers;

        PERegisterSnapshot() {
            for (auto& reg : registers) {
                reg.store(0ULL, std::memory_order_relaxed);
            }
            // LOWER_REG inicial
            registers[11].store(0xFFFFFFFFFFFFFFFFULL, std::memory_order_relaxed);
        }
    };

    struct InstructionMemorySharedData {
        std::array<PEInstructionConnection, 4> pe_connections;
        std::atomic<bool> should_stop{ false };
        std::atomic<bool> component_ready{ false };
    };

    struct CPUSystemSharedData {
        std::array<PEInstructionConnection, 4> instruction_connections;
        std::array<PECacheConnection, 4> cache_connections;
        std::array<PEControlSignals, 4> pe_control;

        // NUEVO: Snapshots de registros para cada PE (solo lectura desde UI)
        std::array<PERegisterSnapshot, 4> pe_registers;

        std::atomic<bool> system_should_stop{ false };
    };

} // namespace cpu_tlp