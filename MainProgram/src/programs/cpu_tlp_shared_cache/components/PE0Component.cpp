#include "programs/cpu_tlp_shared_cache/components/PE0Component.h"
#include "programs/cpu_tlp_shared_cache/components/SharedData.h"
#include <chrono>
#include <thread>
#include <iostream>

namespace cpu_tlp {

    // ============================
    // Helpers aritmética portable
    // ============================
    static inline uint64_t add64(uint64_t a, uint64_t b, bool& c, bool& v) {
        uint64_t r = a + b;
        c = (r < a); // carry (unsigned overflow)
        bool sa = (int64_t)a < 0, sb = (int64_t)b < 0, sr = (int64_t)r < 0;
        v = (sa == sb && sr != sa); // signed overflow
        return r;
    }
    static inline uint64_t sub64(uint64_t a, uint64_t b, bool& c, bool& v) {
        uint64_t r = a - b;
        c = (a >= b); // no borrow
        bool sa = (int64_t)a < 0, sb = (int64_t)b < 0, sr = (int64_t)r < 0;
        v = (sa != sb && sr != sa); // signed overflow
        return r;
    }

    // ============================
    // ALU (implementación mínima)
    // ============================
    uint64_t PE0Component::ALU::execute(uint8_t control, uint64_t A, uint64_t B, uint8_t& flags) {
        bool C = false, V = false;
        uint64_t R = 0;
        switch (control) {
        case 0b000000: R = add64(A, B, C, V); break; // ADD
        case 0b000001: R = sub64(A, B, C, V); break; // SUB
        case 0b000110: R = (A & B); break;           // AND
        case 0b000111: R = (A | B); break;           // ORR
        case 0b001000: R = (A ^ B); break;           // EOR
        default:       R = A; break;                 // por defecto pasa A
        }
        // Flags NZCV muy simples
        uint8_t N = (R >> 63) & 1U;
        uint8_t Z = (R == 0) ? 1U : 0U;
        uint8_t Cf = C ? 1U : 0U;
        uint8_t Vf = V ? 1U : 0U;
        flags = (uint8_t)((N << 3) | (Z << 2) | (Cf << 1) | (Vf));
        return R;
    }

    // =====================================
    // ControlUnit (dummy, todo en cero)
    // =====================================
    void PE0Component::ControlUnit::decode(uint8_t /*opcode*/, uint8_t /*special*/) {
        RegWriteS = RegWriteR = false;
        MemOp = 0;
        MemWriteG = MemWriteD = MemWriteV = MemWriteP = false;
        MemByte = false;
        PCSrc = false;
        ALUSrc = 0;
        PrintEn = 0;
        ComS = false;
        LogOut = false;
        BranchE = false;
        FlagsUpd = false;
        BranchOp = 0;
        ImmediateOp = false;
        RegisterInB = 0;
        RegisterInA = false;
    }

    // =====================================
    // RegisterFile (12 registros)
    // =====================================
    PE0Component::RegisterFile::RegisterFile() { reset(); }

    void PE0Component::RegisterFile::reset() {
        regs.fill(0ULL);
        regs[11] = INITIAL_LOWER; // LOWER_REG = 0xFFFFFFFFFFFFFFFF
        regs[9] = 0ULL;          // PEID por defecto (si se requiere, sobrescribir afuera)
    }

    uint64_t PE0Component::RegisterFile::read(uint8_t addr) {
        if (addr >= regs.size()) return 0ULL;
        return regs[addr];
    }

    void PE0Component::RegisterFile::write(uint8_t addr, uint64_t value, bool we) {
        if (!we) return;
        if (addr == 0) return; // REG0 inmutable
        if (addr < regs.size()) regs[addr] = value;
    }

    // =====================================
    // HazardUnit (dummy: sin hazards)
    // =====================================
    void PE0Component::HazardUnit::checkHazards(const Pipeline& /*pipeline*/) {
        StallF = StallD = StallE = StallM = StallW = false;
        FlushD = FlushE = false;
    }

    bool PE0Component::HazardUnit::detectRAW(uint8_t, uint8_t, const Pipeline&) { return false; }
    bool PE0Component::HazardUnit::detectBranch(const Pipeline&) { return false; }
    bool PE0Component::HazardUnit::detectCacheStall(const Pipeline&) { return false; }

    // =====================================
    // PE0Component: ctor/dtor
    // =====================================
    PE0Component::PE0Component(int pe_id)
        : m_pe_id(pe_id)
        , m_sharedData(nullptr)
        , m_executionThread(nullptr)
        , m_PC(0)
        , m_nextPC(0)
        , m_flags(0)
        , m_isRunning(false)
        , m_segmentationFault(false) {
        // limpiar pipeline
        m_pipeline = Pipeline{};
    }

    PE0Component::~PE0Component() {
        shutdown();
    }

    // =====================================
    // Ciclo de vida (hilo único del PE0)
    // =====================================
    bool PE0Component::initialize(std::shared_ptr<CPUSystemSharedData> sharedData) {
        if (m_isRunning) return true;
        m_sharedData = std::move(sharedData);
        if (!m_sharedData) return false;

        // Reset de control para este PE
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.command.store(0);
        ctrl.step_count.store(0);
        ctrl.running.store(false);
        ctrl.should_stop.store(false);

        m_isRunning = true;
        m_sharedData->system_should_stop.store(false);

        // ¡UN SOLO HILO del PE0!
        m_executionThread = std::make_unique<std::thread>(&PE0Component::threadMain, this);
        return true;
    }

    void PE0Component::shutdown() {
        if (!m_isRunning) return;

        // Señal global + local
        m_sharedData->system_should_stop.store(true);
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.should_stop.store(true);

        if (m_executionThread && m_executionThread->joinable())
            m_executionThread->join();
        m_executionThread.reset();
        m_isRunning = false;
    }

    bool PE0Component::isRunning() const { return m_isRunning; }

    // =====================================
    // API de control desde el hilo principal
    // =====================================
    void PE0Component::step() {
        if (!m_sharedData) return;
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.command.store(1); // step
        ctrl.running.store(true);
    }

    void PE0Component::stepUntil(int value) {
        if (!m_sharedData) return;
        if (value <= 0) value = 1;
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.step_count.store(value);
        ctrl.command.store(2); // step_until
        ctrl.running.store(true);
    }

    void PE0Component::stepIndefinitely() {
        if (!m_sharedData) return;
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.should_stop.store(false);
        ctrl.command.store(3); // step_infinite
        ctrl.running.store(true);
    }

    void PE0Component::stopExecution() {
        if (!m_sharedData) return;
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.should_stop.store(true);
        // dejar command como esté; el hilo lo observará y volverá a idle
    }

    void PE0Component::reset() {
        // Estado interno
        m_pipeline = Pipeline{};
        m_registerFile.reset();
        m_hazardUnit.checkHazards(m_pipeline);
        m_PC = 0ULL; m_nextPC = 0ULL;
        m_flags = 0;
        m_segmentationFault = false;

        // Señales externas opcionales
        if (m_sharedData) {
            // no definimos memoria/instrucciones aquí porque viven en otra sharedData
            auto& ctrl = m_sharedData->pe_control[m_pe_id];
            ctrl.command.store(0);
            ctrl.step_count.store(0);
            ctrl.should_stop.store(false);
            ctrl.running.store(false);
        }
    }

    // =====================================
    // Hilo del PE0 (único)
    // =====================================
    void PE0Component::threadMain() {
        using namespace std::chrono_literals;

        while (!m_sharedData->system_should_stop.load(std::memory_order_acquire)) {
            auto& ctrl = m_sharedData->pe_control[m_pe_id];
            int cmd = ctrl.command.load(std::memory_order_acquire);

            switch (cmd) {
            case 0: // idle
                ctrl.running.store(false);
                std::this_thread::sleep_for(1ms);
                break;

            case 1: // step
                executeCycle();
                ctrl.command.store(0);
                ctrl.running.store(false);
                break;

            case 2: { // step_until (por conteo)
                int left = ctrl.step_count.load();
                if (left <= 0) { ctrl.command.store(0); ctrl.running.store(false); break; }
                // ejecutar 1 ciclo por iteración para seguir observando should_stop
                executeCycle();
                ctrl.step_count.store(left - 1);
                if (left - 1 <= 0 || ctrl.should_stop.load()) {
                    ctrl.command.store(0);
                    ctrl.running.store(false);
                    ctrl.should_stop.store(false);
                }
                break;
            }

            case 3: // step_infinite
                if (ctrl.should_stop.load()) {
                    ctrl.command.store(0);
                    ctrl.running.store(false);
                    ctrl.should_stop.store(false);
                    break;
                }
                executeCycle();
                break;

            case 4: // reset
                reset();
                ctrl.command.store(0);
                ctrl.running.store(false);
                break;

            default:
                // comando inválido -> idle
                ctrl.command.store(0);
                ctrl.running.store(false);
                break;
            }
        }
    }

    // =====================================
    // Núcleo de un ciclo de CPU (mínimo)
    // =====================================
    void PE0Component::executeCycle() {
        // Pipeline “vacío” para que todo enlace; puedes ir llenando lógica real.
        stageFetch();
        stageDecode();
        stageExecute();
        stageMemory();
        stageWriteBack();

        // Avance del PC muy básico: +8 por ciclo
        m_nextPC = m_PC + 8ULL;
        m_PC = m_nextPC;
    }

    // Etapas “stub” (no hacen nada todavía, pero evitan unresolved)
    void PE0Component::stageFetch() { m_pipeline.next_IF_ID = { m_PC, NOP_INSTRUCTION, true }; }
    void PE0Component::stageDecode() { /* TODO: decodificar m_pipeline.IF_ID.Instruction */ }
    void PE0Component::stageExecute() { /* TODO: usar m_alu.execute(...) y actualizar flags */ }
    void PE0Component::stageMemory() { /* TODO: integrar con cache si aplica */ }
    void PE0Component::stageWriteBack() {/* TODO: escribir en RegisterFile si RegWrite */ }

} // namespace cpu_tlp
