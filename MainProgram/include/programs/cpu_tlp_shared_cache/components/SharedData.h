//SharedData.h
#pragma once
#include <atomic>
#include <cstdint>

namespace cpu_tlp {

    // =======================
    // ANALYSIS COUNTERS (NEW)
    // =======================
    struct AnalysisCounters {
        std::atomic<uint64_t> traffic_pe[4];
        std::atomic<uint64_t> cache_misses{ 0 };
        std::atomic<uint64_t> invalidations{ 0 };
        std::atomic<uint64_t> transactions_mesi{ 0 };

        void reset() {
            for (auto& a : traffic_pe) a.store(0, std::memory_order_relaxed);
            cache_misses.store(0, std::memory_order_relaxed);
            invalidations.store(0, std::memory_order_relaxed);
            transactions_mesi.store(0, std::memory_order_relaxed);
        }
    };

    // ============================================================================
    // INSTRUCTION MEMORY CONNECTION (existente)
    // ============================================================================
    struct InstructionConnection {
        std::atomic<uint64_t> PC_F{ 0 };
        std::atomic<uint64_t> InstrF{ 0 };
        std::atomic<bool> INS_READY{ false };
    };

    // ============================================================================
    // CACHE CONNECTION (existente)
    // ============================================================================
    struct CacheConnection {
        std::atomic<uint64_t> ALUOut_M{ 0 };
        std::atomic<uint64_t> RD_Rm_Special_M{ 0 };
        std::atomic<bool> C_WE_M{ false };
        std::atomic<bool> C_ISB_M{ false };
        std::atomic<bool> C_REQUEST_M{ false };
        std::atomic<uint64_t> RD_C_out{ 0 };
        std::atomic<bool> C_READY{ false };
        std::atomic<bool> C_READY_ACK{ false };
    };

    // ============================================================================
    // RAM CONNECTION (NUEVO - AGREGAR AQUÍ)
    // ============================================================================
    struct RAMConnection {
        std::atomic<uint16_t> request_address{ 0 };
        std::atomic<uint64_t> write_data{ 0 };
        std::atomic<bool> write_enable{ false };
        std::atomic<bool> request_active{ false };
        std::atomic<uint64_t> read_data{ 0 };
        std::atomic<bool> response_ready{ false };
    };

    // ============================================================================
    // PE CONTROL (existente)
    // ============================================================================
    struct PEControl {
        std::atomic<int> command{ 0 };
        std::atomic<int> step_count{ 0 };
        std::atomic<bool> running{ false };
        std::atomic<bool> should_stop{ false };
    };

    // ============================================================================
    // PE REGISTERS SNAPSHOT (existente)
    // ============================================================================
    struct PERegistersSnapshot {
        std::atomic<uint64_t> registers[12];
        PERegistersSnapshot() {
            for (int i = 0; i < 12; ++i) {
                registers[i].store(0, std::memory_order_relaxed);
            }
        }
    };

    // ============================================================================
    // PE INSTRUCTION TRACKING (existente)
    // ============================================================================
    struct PEInstructionTracking {
        std::atomic<uint64_t> version{ 0 };
        std::atomic<uint64_t> stage_instructions[5];
        PEInstructionTracking() {
            version.store(0, std::memory_order_relaxed);
            for (int i = 0; i < 5; ++i) {
                stage_instructions[i].store(0, std::memory_order_relaxed);
            }
        }
    };

    // ============================================================================
    // UI SIGNALS (existente)
    // ============================================================================
    struct UISignals {
        std::atomic<uint32_t> swi_count{ 0 };
    };

    // ============================================================================
    // MAIN SHARED DATA STRUCTURE (existente + modificación)
    // ============================================================================
    struct CPUSystemSharedData {
        std::atomic<bool> system_should_stop{ false };

        InstructionConnection instruction_connections[4];
        CacheConnection cache_connections[4];
        RAMConnection ram_connection;  // AGREGAR ESTA LÍNEA

        PEControl pe_control[4];
        PERegistersSnapshot pe_registers[4];
        PEInstructionTracking pe_instruction_tracking[4];
        UISignals ui_signals[4];

        // ===== Analysis =====
        AnalysisCounters analysis;
    };

} // namespace cpu_tlp