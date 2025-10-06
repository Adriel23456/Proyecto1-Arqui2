#include "programs/cpu_tlp_shared_cache/components/PE1Component.h"
#include "programs/cpu_tlp_shared_cache/widgets/InstructionDisassembler.h"
#include "programs/cpu_tlp_shared_cache/widgets/Log.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>

static constexpr uint8_t REG_ZERO = 0;
static constexpr uint8_t REG_PEID = 9;

namespace cpu_tlp {

    // ============================================================================
    // HELPERS ARITMÉTICA
    // ============================================================================

    static inline void computeFlags64(uint64_t result, uint64_t a, uint64_t b,
        bool isSubtraction, uint8_t& flags) {
        bool N = (result >> 63) & 1;
        bool Z = (result == 0);

        bool C;
        if (isSubtraction) {
            C = (a >= b); // no borrow
        }
        else {
            C = (result < a); // carry
        }

        bool sa = (int64_t)a < 0;
        bool sb = (int64_t)b < 0;
        bool sr = (int64_t)result < 0;
        bool V;
        if (isSubtraction) {
            V = (sa != sb && sr != sa);
        }
        else {
            V = (sa == sb && sr != sa);
        }

        flags = (N ? 0x8 : 0) | (Z ? 0x4 : 0) | (C ? 0x2 : 0) | (V ? 0x1 : 0);
    }

    static inline double bitsToDouble(uint64_t bits) {
        double d;
        std::memcpy(&d, &bits, 8);
        return d;
    }

    static inline uint64_t doubleToBits(double d) {
        uint64_t bits;
        std::memcpy(&bits, &d, 8);
        return bits;
    }

    // ============================================================================
    // REGISTERFILE
    // ============================================================================

    PE1Component::RegisterFile::RegisterFile() {
        reset();
    }

    void PE1Component::RegisterFile::reset() {
        regs.fill(0ULL);
        regs[11] = 0xFFFFFFFFFFFFFFFFULL; // LOWER_REG
        regs[9] = 0ULL; // PEID
    }

    uint64_t PE1Component::RegisterFile::read(uint8_t addr) const {
        if (addr >= 12) return 0ULL;
        return regs[addr];
    }

    void PE1Component::RegisterFile::write(uint8_t addr, uint64_t value, bool we) {
        if (!we) return;
        if (addr >= 12) return;

        // Bloquear SIEMPRE los intentos de escritura a R0 y PEID
        if (addr == REG_ZERO || addr == REG_PEID) {
            return;
        }

        regs[addr] = value;

        if (onRegisterWrite) {
            onRegisterWrite(addr, value);
        }
    }

    // ============================================================================
    // ALU
    // ============================================================================

    PE1Component::ALU::Result PE1Component::ALU::execute(uint8_t control, uint64_t A, uint64_t B, uint8_t flagsIn) {
        Result res;
        res.value = 0;
        res.flags = flagsIn;

        bool carry = (flagsIn & 0x2) != 0;

        switch (control) {
            // Operaciones enteras
        case 0x00: // ADD
            res.value = A + B;
            computeFlags64(res.value, A, B, false, res.flags);
            break;
        case 0x01: // SUB
            res.value = A - B;
            computeFlags64(res.value, A, B, true, res.flags);
            break;
        case 0x02: // ADC
            res.value = A + B + (carry ? 1 : 0);
            computeFlags64(res.value, A, B + (carry ? 1 : 0), false, res.flags);
            break;
        case 0x03: // SBC
            res.value = A - B - (carry ? 0 : 1);
            computeFlags64(res.value, A, B + (carry ? 0 : 1), true, res.flags);
            break;
        case 0x04: // MUL
            res.value = A * B;
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x05: // DIV
            if (B != 0) res.value = (int64_t)A / (int64_t)B;
            else res.value = 0;
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x06: // AND
            res.value = A & B;
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x07: // ORR
            res.value = A | B;
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x08: // EOR
            res.value = A ^ B;
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x09: // BIC
            res.value = A & ~B;
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x0A: // LSL
            res.value = A << (B & 0x3F);
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x0B: // LSR
            res.value = A >> (B & 0x3F);
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x0C: // ASR
            res.value = (uint64_t)((int64_t)A >> (B & 0x3F));
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x0D: // ROR
        {
            uint8_t shift = B & 0x3F;
            res.value = (A >> shift) | (A << (64 - shift));
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
        }
        break;

        // Operaciones de punto flotante
        case 0x0E: { // FADD
            double da = bitsToDouble(A);
            double db = bitsToDouble(B);
            double r = da + db;
            res.value = doubleToBits(r);
            // Flags FP para "suma" (usados cuando FlagsUpd_D=1, p.ej. FCMN/FCMNI)
            if (std::isnan(da) || std::isnan(db) || std::isnan(r)) {
                res.flags = 0x1; // V=1 => unordered
            }
            else {
                bool N = (r < 0.0);
                bool Z = (r == 0.0);
                // No hay carry en FP; lo dejamos en 0
                res.flags = (N ? 0x8 : 0) | (Z ? 0x4 : 0);
            }
            break;
        }
        case 0x0F: { // FSUB
            double da = bitsToDouble(A);
            double db = bitsToDouble(B);
            double r = da - db;
            res.value = doubleToBits(r);
            // Flags FP estilo "compare": N/Z del resultado, C = (da >= db), V=unordered
            if (std::isnan(da) || std::isnan(db) || std::isnan(r)) {
                res.flags = 0x1; // V=1
            }
            else {
                bool N = (r < 0.0);
                bool Z = (r == 0.0);
                bool C = (da >= db);
                res.flags = (N ? 0x8 : 0) | (Z ? 0x4 : 0) | (C ? 0x2 : 0);
            }
            break;
        }
        case 0x10: // FMUL
            res.value = doubleToBits(bitsToDouble(A) * bitsToDouble(B));
            break;
        case 0x11: // FDIV
            res.value = doubleToBits(bitsToDouble(A) / bitsToDouble(B));
            break;
        case 0x12: // FCOPYSIGN
        {
            double mag = std::fabs(bitsToDouble(A));
            double sign = bitsToDouble(B);
            res.value = doubleToBits(std::copysign(mag, sign));
        }
        break;
        case 0x13: // FSQRT
            res.value = doubleToBits(std::sqrt(bitsToDouble(B)));
            break;
        case 0x14: // FNEG
            res.value = doubleToBits(-bitsToDouble(B));
            break;
        case 0x15: // FABS
            res.value = doubleToBits(std::fabs(bitsToDouble(B)));
            break;
        case 0x16: // FCDTI (double to int)
            res.value = (uint64_t)(int64_t)bitsToDouble(B);
            break;
        case 0x17: // FCDTD (int to double)
            res.value = doubleToBits((double)(int64_t)B);
            break;
        case 0x18: // RTNR (round to nearest)
            res.value = doubleToBits(std::round(bitsToDouble(B)));
            break;
        case 0x19: // RTZ (round to zero)
            res.value = doubleToBits(std::trunc(bitsToDouble(B)));
            break;
        case 0x1A: // RTP (round to positive)
            res.value = doubleToBits(std::ceil(bitsToDouble(B)));
            break;
        case 0x1B: // RTNE (round to negative)
            res.value = doubleToBits(std::floor(bitsToDouble(B)));
            break;

            // Operaciones especiales
        case 0x1C: // MOV
            res.value = B;
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x1D: // MVN
            res.value = ~B;
            res.flags = ((res.value >> 63) & 1) ? 0x8 : 0;
            if (res.value == 0) res.flags |= 0x4;
            break;
        case 0x1E: // FMOVI
            res.value = B;
            break;
        case 0x1F: // FMVNI
            res.value = ~B;
            break;
        case 0x20: { // FCMPS
            double da = bitsToDouble(A);
            double db = bitsToDouble(B);

            // Unordered si CUALQUIER operando es NaN
            if (std::isnan(da) || std::isnan(db)) {
                res.flags = 0x1;      // V=1, N=Z=C=0
                res.value = 0;
                break;
            }

            // Comparación directa (evita problemas como +inf vs +inf, -0.0 vs +0.0, etc.)
            bool Z = (da == db);      // true para +inf==+inf y -0.0==+0.0
            bool N = (da < db);      // "negativo" si Rn < Rm
            bool C = (da >= db);      // "carry/borrow" estilo ARM: Rn >= Rm

            res.flags = (N ? 0x8 : 0) | (Z ? 0x4 : 0) | (C ? 0x2 : 0);
            res.value = 0;
            break;
        }
                 break;
        case 0x21: // CRASH
            std::cerr << "[PE" << 0 << "] CRASH instruction executed!\n";
            res.value = 0;
            break;
        case 0x22: // NOTHING
            res.value = 0;
            break;
        default:
            res.value = A;
            break;
        }

        return res;
    }

    // ============================================================================
    // CONTROLUNIT
    // ============================================================================

    PE1Component::ControlUnit::Signals PE1Component::ControlUnit::decode(uint8_t opcode) {
        Signals sig;

        // Tabla de control completa
        switch (opcode) {
            // 0x00-0x0D: Operaciones enteras básicas
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

            // 0x0E-0x1D: Operaciones con inmediato (enteras)
        case 0x0E: case 0x0F: case 0x10: case 0x11:
        case 0x12: case 0x13: case 0x14: case 0x15:
        case 0x16: case 0x17: case 0x18: case 0x19:
        case 0x1A: case 0x1B:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode - 0x0E;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;

            // 0x1C: INC Rd  (Rd <- Rd + 1)
        // 0x1D: DEC Rd  (Rd <- Rd - 1)
        case 0x1C: { // INC
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1;
            sig.ALUControl_D = 0x00;  // ADD
            sig.BranchOp_D = 0; sig.BranchE = 0;
            sig.ImmOp = 1;            // usa imm=1 (aunque no lo mostremos en el disassembler)
            sig.DataType = 0;
            break;
        }
        case 0x1D: { // DEC
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1;
            sig.ALUControl_D = 0x01;  // SUB
            sig.BranchOp_D = 0; sig.BranchE = 0;
            sig.ImmOp = 1;            // usa imm=1
            sig.DataType = 0;
            break;
        }

                 // 0x1E-0x27: Operaciones floating point SIN inmediato
        case 0x1E: // FADD Rd, Rn, Rm
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x0E;  // FADD
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

        case 0x1F: // FSUB Rd, Rn, Rm
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x0F;  // FSUB
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

        case 0x20: // FMUL Rd, Rn, Rm
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x10;  // FMUL
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

        case 0x21: // FDIV Rd, Rn, Rm ← ESTE FALTABA
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x11;  // FDIV
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

        case 0x22: // FCOPYSIGN Rd, Rn, Rm
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x12;  // FCOPYSIGN
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

            // 0x23-0x27: Operaciones floating point con inmediato float
        case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode - 0x23 + 0x0E;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 1;
            break;

            // 0x28-0x36: Más operaciones floating point
        case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C:
        case 0x2D: case 0x2E: case 0x2F: case 0x30:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode - 0x28 + 0x13;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

            // 0x31-0x32: MOV/MVN sin inmediato (enteros)
        case 0x31: // MOV (sin inmediato)
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x1C; // MOV
            sig.BranchOp_D = 0; sig.BranchE = 0;
            sig.ImmOp = 0;      // ← SIN inmediato
            sig.DataType = 0;   // ← ENTERO
            break;

        case 0x32: // MVN (sin inmediato)
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x1D; // MVN
            sig.BranchOp_D = 0; sig.BranchE = 0;
            sig.ImmOp = 0;      // ← SIN inmediato
            sig.DataType = 0;   // ← ENTERO
            break;

            // 0x33: MOVI (con inmediato entero)
        case 0x33:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x1C; // MOV
            sig.BranchOp_D = 0; sig.BranchE = 0;
            sig.ImmOp = 1;      // ← CON inmediato
            sig.DataType = 0;   // ← ENTERO
            break;

            // 0x34: MVNI (con inmediato entero)
        case 0x34:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x1D; // MVN
            sig.BranchOp_D = 0; sig.BranchE = 0;
            sig.ImmOp = 1;      // ← CON inmediato
            sig.DataType = 0;   // ← ENTERO (NOT float!)
            break;

            // 0x35: FMOVI (con inmediato float)
        case 0x35:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x1E; // FMOVI
            sig.BranchOp_D = 0; sig.BranchE = 0;
            sig.ImmOp = 1;      // ← CON inmediato
            sig.DataType = 1;   // ← FLOAT
            break;

            // 0x36: FMVNI (con inmediato float)
        case 0x36:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x1F; // FMVNI
            sig.BranchOp_D = 0; sig.BranchE = 0;
            sig.ImmOp = 1;      // ← CON inmediato
            sig.DataType = 1;   // ← FLOAT
            break;

            // 0x37-0x3E: Comparaciones sin escritura
        case 0x37: // CMP (Rn - Rm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x01; // SUB
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

        case 0x38: // CMN (Rn + Rm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x00; // ADD
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

        case 0x39: // TST (Rn & Rm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x06; // AND
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

        case 0x3A: // TEQ (Rn ^ Rm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x08; // EOR
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

        case 0x3B: // CMPI (Rn - Imm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x01; // SUB
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;

        case 0x3C: // CMNI (Rn + Imm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x00; // ADD
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;

        case 0x3D: // TSTI (Rn & Imm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x06; // AND
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;

        case 0x3E: // TEQI (Rn ^ Imm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x08; // EOR
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;

            // 0x3F-0x44: Comparaciones floating point (sin escritura a Rd)
        // FCMP  -> usa FSUB (flags de r = Rn - Rm)
        // FCMN  -> usa FADD (flags de r = Rn + Rm)
        // FCMPS -> usa comparador dedicado (tu ALU 0x20)
        // ... y sus variantes con inmediato (imm float)

        case 0x3F: { // FCMP Rn, Rm  (flags de Rn - Rm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x0F; // FSUB
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;
        }
        case 0x40: { // FCMN Rn, Rm  (flags de Rn + Rm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x0E; // FADD
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;
        }
        case 0x41: { // FCMPS Rn, Rm  (comparador dedicado)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x20; // FCMPS (tu caso 0x20)
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;
        }
        case 0x42: { // FCMPI Rn, #immf  (flags de Rn - imm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x0F; // FSUB
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 1; // imm float->double
            break;
        }
        case 0x43: { // FCMNI Rn, #immf (flags de Rn + imm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x0E; // FADD
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 1;
            break;
        }
        case 0x44: { // FCMPSI Rn, #immf (comparador dedicado con imm)
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = 0x20; // FCMPS
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 1;
            break;
        }

                 // 0x45-0x4B: Branches
        case 0x45: case 0x46: case 0x47: case 0x48:
        case 0x49: case 0x4A: case 0x4B:
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 1;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0;
            sig.BranchOp_D = opcode - 0x45 + 1;
            sig.BranchE = 1;  // ← CAMBIAR DE 0 A 1
            sig.ImmOp = 1; sig.DataType = 0;
            break;

            // 0x4C-0x4D: CRASH y NOTHING
        case 0x4C:
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x21;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;
        case 0x4D:
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x22;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

            // 0x4E-0x51: Operaciones de memoria
        case 0x4E: // LDR
            sig.RegWrite_D = 1; sig.MemOp_D = 1; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 1; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;
        case 0x4F: // STR
            sig.RegWrite_D = 0; sig.MemOp_D = 1; sig.C_WE_D = 1;
            sig.C_REQUEST_D = 1; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;
        case 0x50: // LDRI (ISB)
            sig.RegWrite_D = 1; sig.MemOp_D = 1; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 1; sig.C_ISB_D = 1; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;
        case 0x51: // STRI (ISB)
            sig.RegWrite_D = 0; sig.MemOp_D = 1; sig.C_WE_D = 1;
            sig.C_REQUEST_D = 1; sig.C_ISB_D = 1; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;

            // 0x52-0x5A: Más operaciones floating point con inmediato
        case 0x52: case 0x53: case 0x54: case 0x55:
        case 0x56: case 0x57: case 0x58: case 0x59: case 0x5A:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode - 0x52 + 0x13;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 1;
            break;

        default: // NOP
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0x22;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;
        }

        return sig;
    }

    // ============================================================================
    // HAZARDUNIT - VERSIÓN CORREGIDA SIN std::to_string
    // ============================================================================

    PE1Component::HazardUnit::Outputs
        PE1Component::HazardUnit::detect(
            bool INS_READY,
            bool C_REQUEST_M, bool C_READY,
            bool SegmentationFault,
            bool PCSrc_AND,
            bool PCSrc_W,  // ← NUEVO
            uint8_t Rd_in_D, uint8_t Rn_in, uint8_t Rm_in,
            uint8_t Rd_in_E, uint8_t Rd_in_M, uint8_t Rd_in_W,
            bool RegWrite_E, bool RegWrite_M, bool RegWrite_W,
            bool BranchE,
            uint8_t BranchOp_E
        ) {
        Outputs out;
        out.StallF = out.StallD = out.StallE = out.StallM = out.StallW = false;
        out.FlushD = out.FlushE = false;

        log_build_and_print([&](std::ostringstream& oss) {
            oss << "  [HazardUnit] Inputs: Rd_D=" << (int)Rd_in_D
                << " Rn=" << (int)Rn_in << " Rm=" << (int)Rm_in
                << " | Rd_E=" << (int)Rd_in_E << "(RegWr=" << (int)RegWrite_E << ")"
                << " Rd_M=" << (int)Rd_in_M << "(RegWr=" << (int)RegWrite_M << ")"
                << " Rd_W=" << (int)Rd_in_W << "(RegWr=" << (int)RegWrite_W << ")"
                << " | BranchE=" << (int)BranchE << " PCSrc_AND=" << (int)PCSrc_AND
                << " PCSrc_W=" << (int)PCSrc_W << "\n";
            });

        // 1. SegmentationFault
        if (SegmentationFault) {
            out.StallF = out.StallD = out.StallE = out.StallM = out.StallW = true;
            log_line("  [HazardUnit] SEGFAULT -> Full stall\n");
            return out;
        }

        // 2. InstructionLatency
        if (!INS_READY) {
            out.StallF = true;
            out.StallD = true;
            log_line("  [HazardUnit] !INS_READY -> StallF, StallD\n");
            return out;
        }

        // 3. CacheLatency
        if (C_REQUEST_M && !C_READY) {
            out.StallF = out.StallD = out.StallE = out.StallM = out.StallW = true;
            log_line("  [HazardUnit] Cache latency -> Full stall\n");
            return out;
        }

        // 4. RAW
        bool rawHazard = false;

        if (Rn_in != 0) {
            if (RegWrite_E && Rd_in_E != 0 && Rd_in_E == Rn_in) {
                rawHazard = true;
                log_build_and_print([&](std::ostringstream& oss) {
                    oss << "  [HazardUnit] RAW: Rn=" << (int)Rn_in
                        << " depends on Rd_E=" << (int)Rd_in_E << "\n";
                    });
            }
            if (RegWrite_M && Rd_in_M != 0 && Rd_in_M == Rn_in) {
                rawHazard = true;
                log_build_and_print([&](std::ostringstream& oss) {
                    oss << "  [HazardUnit] RAW: Rn=" << (int)Rn_in
                        << " depends on Rd_M=" << (int)Rd_in_M << "\n";
                    });
            }
        }
        if (Rm_in != 0) {
            if (RegWrite_E && Rd_in_E != 0 && Rd_in_E == Rm_in) {
                rawHazard = true;
                log_build_and_print([&](std::ostringstream& oss) {
                    oss << "  [HazardUnit] RAW: Rm=" << (int)Rm_in
                        << " depends on Rd_E=" << (int)Rd_in_E << "\n";
                    });
            }
            if (RegWrite_M && Rd_in_M != 0 && Rd_in_M == Rm_in) {
                rawHazard = true;
                log_build_and_print([&](std::ostringstream& oss) {
                    oss << "  [HazardUnit] RAW: Rm=" << (int)Rm_in
                        << " depends on Rd_M=" << (int)Rd_in_M << "\n";
                    });
            }
        }

        if (rawHazard) {
            out.StallF = out.StallD = true;
            out.FlushE = true;
            log_line("  [HazardUnit] -> Applying: StallF, StallD, FlushE\n");
            return out;
        }

        // 5. BRANCHING - VERSIÓN FINAL CORREGIDA
        // Detectar branch en DECODE con BranchE
        if (BranchE && !branchActive) {
            branchActive = true;
            branchCycles = 0;
            branchWaitingForW = false;
            log_line("  [HazardUnit] Branch detected in Decode, starting handling\n");
        }

        if (branchActive) {
            branchCycles++;

            if (branchCycles == 1) {
                // Ciclo 1: Branch en D → StallF, FlushD siempre
                out.StallF = true;
                out.FlushD = true;
                log_line("  [HazardUnit] Branch cycle 1 (in D) -> StallF, FlushD\n");
            }
            else if (branchCycles == 2) {
                // Ciclo 2: Branch en E, evaluar PCSrc_AND
                if (PCSrc_AND) {
                    // Branch TOMADO: continuar con StallF + FlushD
                    out.StallF = true;
                    out.FlushD = true;
                    branchWaitingForW = true;
                    log_line("  [HazardUnit] Branch cycle 2 (in E) TAKEN -> StallF, FlushD (continuing)\n");
                }
                else {
                    // Branch NO TOMADO: terminar inmediatamente
                    branchActive = false;
                    log_line("  [HazardUnit] Branch cycle 2 (in E) NOT TAKEN -> end\n");
                }
            }
            else if (branchWaitingForW) {
                // Ciclos 3+: Branch avanzando hacia W
                if (PCSrc_W) {
                    // Branch llegó a W - PERMITIR que PC se actualice (NO StallF)
                    // pero mantener FlushD para limpiar la instrucción incorrecta
                    out.FlushD = true;  // ← Solo FlushD, sin StallF
                    branchWaitingForW = false;
                    branchActive = false;  // ← Terminar aquí mismo
                    log_build_and_print([&](std::ostringstream& oss) {
                        oss << "  [HazardUnit] Branch in W (cycle " << branchCycles
                            << ") PC updating NOW -> FlushD only\n";
                        });
                }
                else {
                    // Aún esperando que llegue a W
                    out.StallF = true;
                    out.FlushD = true;
                    log_build_and_print([&](std::ostringstream& oss) {
                        oss << "  [HazardUnit] Branch cycle " << branchCycles
                            << " (waiting for W) -> StallF, FlushD\n";
                        });
                }
            }
            else {
                // Ya no debería llegar aquí
                branchActive = false;
                log_line("  [HazardUnit] Branch cleanup\n");
            }
        }

        if (!rawHazard && !branchActive) {
            log_line("  [HazardUnit] No hazards detected\n");
        }

        return out;
    }

    // ============================================================================
    // HELPERS
    // ============================================================================

    uint64_t PE1Component::extendImmediate(uint32_t imm, bool dataType) {
        if (dataType == 0) {
            // Extensión de signo de 32 bits a 64 bits (complemento a 2)
            if (imm & 0x80000000) {
                return 0xFFFFFFFF00000000ULL | imm;
            }
            else {
                return imm;
            }
        }
        else {
            // Conversión de float 32-bit a double 64-bit
            float f;
            std::memcpy(&f, &imm, 4);
            double d = static_cast<double>(f);
            uint64_t result;
            std::memcpy(&result, &d, 8);
            return result;
        }
    }

    bool PE1Component::evaluateBranchCondition(uint8_t branchOp, uint8_t flags) {
        bool N = (flags & 0x8) != 0;
        bool Z = (flags & 0x4) != 0;
        bool C = (flags & 0x2) != 0;
        bool V = (flags & 0x1) != 0;

        switch (branchOp) {
        case 0x0: return false;           // No branch
        case 0x1: return true;            // B (always)
        case 0x2: return Z;               // BEQ
        case 0x3: return !Z;              // BNE
        case 0x4: return N != V;          // BLT
        case 0x5: return !Z && (N == V);  // BGT
        case 0x6: return V;               // BUN (unordered)
        case 0x7: return !V;              // BORD (ordered)
        default: return false;
        }
    }

    // ============================================================================
    // CONSTRUCTOR / DESTRUCTOR / LIFECYCLE
    // ============================================================================

    PE1Component::PE1Component(int pe_id)
        : m_pe_id(pe_id)
        , m_sharedData(nullptr)
        , m_executionThread(nullptr)
        , m_isRunning(false)
        , m_segmentationFault(false)
    {
        reset();
    }

    PE1Component::~PE1Component() {
        shutdown();
    }

    bool PE1Component::initialize(std::shared_ptr<CPUSystemSharedData> sharedData) {
        if (m_isRunning.load()) {
            std::cerr << "[PE" << m_pe_id << "] Already running\n";
            return false;
        }

        m_sharedData = std::move(sharedData);
        if (!m_sharedData) {
            std::cerr << "[PE" << m_pe_id << "] Invalid shared data\n";
            return false;
        }

        // Configurar PEID en el registro file
        m_registerFile.setPEID(m_pe_id);

        // NUEVO: Conectar RegisterFile con el snapshot compartido
        m_registerFile.onRegisterWrite = [this](uint8_t addr, uint64_t value) {
            if (addr < 12) {
                m_sharedData->pe_registers[m_pe_id].registers[addr].store(value, std::memory_order_release);
            }
            };

        // Reset de control
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.command.store(0, std::memory_order_release);
        ctrl.step_count.store(0, std::memory_order_release);
        ctrl.running.store(false, std::memory_order_release);
        ctrl.should_stop.store(false, std::memory_order_release);

        m_isRunning.store(true, std::memory_order_release);
        m_sharedData->system_should_stop.store(false, std::memory_order_release);

        // Lanzar hilo
        m_executionThread = std::make_unique<std::thread>(&PE1Component::threadMain, this);

        std::cout << "[PE" << m_pe_id << "] Initialized successfully\n";
        return true;
    }

    void PE1Component::shutdown() {
        if (!m_isRunning.load()) return;

        std::cout << "[PE" << m_pe_id << "] Shutting down...\n";

        m_sharedData->system_should_stop.store(true, std::memory_order_release);
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.should_stop.store(true, std::memory_order_release);

        if (m_executionThread && m_executionThread->joinable()) {
            m_executionThread->join();
        }
        m_executionThread.reset();
        m_isRunning.store(false, std::memory_order_release);

        std::cout << "[PE" << m_pe_id << "] Shutdown complete\n";
    }

    bool PE1Component::isRunning() const {
        return m_isRunning.load(std::memory_order_acquire);
    }

    void PE1Component::reset() {
        // Reset del pipeline
        PC_F = 0x0;
        PCPlus8_F = 0x0;

        IF_ID = IF_ID_next = {};
        IF_ID.Instr_F = NOP_INSTRUCTION;
        IF_ID.PC_F = 0x0;

        ID_EX = ID_EX_next = {};
        ID_EX.ALUControl_D = 0x22;
        ID_EX.Instr_D = NOP_INSTRUCTION;

        EX_MEM = EX_MEM_next = {};
        EX_MEM.Instr_E = NOP_INSTRUCTION;
        MEM_WB = MEM_WB_next = {};
        MEM_WB.Instr_M = NOP_INSTRUCTION;

        InstrD = NOP_INSTRUCTION;
        PC_in = 0x0;
        SegmentationFault = false;
        m_segmentationFault = false;

        // Reset register file
        m_registerFile.reset();
        m_registerFile.setPEID(m_pe_id);

        // Sincronizar snapshot de registros después del reset
        if (m_sharedData) {
            for (int i = 0; i < 12; ++i) {
                uint64_t val = m_registerFile.read(i);
                m_sharedData->pe_registers[m_pe_id].registers[i].store(val, std::memory_order_release);
            }
        }

        m_hazards = {};

        // Reset tracking de instrucciones (local)
        m_stageInstructions.fill(NOP_INSTRUCTION);

        // AGREGAR: Sincronizar tracking de instrucciones con SharedData
        if (m_sharedData) {
            for (int i = 0; i < 5; ++i) {
                m_sharedData->pe_instruction_tracking[m_pe_id].stage_instructions[i].store(
                    NOP_INSTRUCTION, std::memory_order_release
                );
            }
        }

        // Reset señales compartidas (escritura)
        if (m_sharedData) {
            auto& instConn = m_sharedData->instruction_connections[m_pe_id];
            instConn.PC_F.store(0x0, std::memory_order_release);

            auto& cacheConn = m_sharedData->cache_connections[m_pe_id];
            cacheConn.ALUOut_M.store(0x0, std::memory_order_release);
            cacheConn.RD_Rm_Special_M.store(0x0, std::memory_order_release);
            cacheConn.C_WE_M.store(false, std::memory_order_release);
            cacheConn.C_ISB_M.store(false, std::memory_order_release);
            cacheConn.C_REQUEST_M.store(false, std::memory_order_release);
        }

        std::cout << "[PE" << m_pe_id << "] Reset complete\n";
    }

    // ============================================================================
    // CONTROL API
    // ============================================================================

    void PE1Component::step() {
        if (!m_sharedData) {
            std::cerr << "[PE" << m_pe_id << "] step() called but no shared data!\n";
            return;
        }
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.command.store(1, std::memory_order_release);
        ctrl.running.store(true, std::memory_order_release);
        std::cout << "[PE" << m_pe_id << "] step() command issued\n";
    }

    void PE1Component::stepUntil(int value) {
        if (!m_sharedData) {
            std::cerr << "[PE" << m_pe_id << "] stepUntil() called but no shared data!\n";
            return;
        }
        if (value <= 0) value = 1;
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.step_count.store(value, std::memory_order_release);
        ctrl.command.store(2, std::memory_order_release);
        ctrl.running.store(true, std::memory_order_release);
        std::cout << "[PE" << m_pe_id << "] stepUntil(" << value << ") command issued\n";
    }

    void PE1Component::stepIndefinitely() {
        if (!m_sharedData) {
            std::cerr << "[PE" << m_pe_id << "] stepIndefinitely() called but no shared data!\n";
            return;
        }
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.should_stop.store(false, std::memory_order_release);
        ctrl.command.store(3, std::memory_order_release);
        ctrl.running.store(true, std::memory_order_release);
        std::cout << "[PE" << m_pe_id << "] stepIndefinitely() command issued\n";
    }

    void PE1Component::stopExecution() {
        if (!m_sharedData) {
            std::cerr << "[PE" << m_pe_id << "] stopExecution() called but no shared data!\n";
            return;
        }
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.should_stop.store(true, std::memory_order_release);
        std::cout << "[PE" << m_pe_id << "] stopExecution() command issued\n";
    }

    // ============================================================================
    // THREAD MAIN - AGREGAR PRINTS
    // ============================================================================

    void PE1Component::threadMain() {
        using namespace std::chrono_literals;

        std::cout << "[PE" << m_pe_id << "] Thread started\n";

        while (!m_sharedData->system_should_stop.load(std::memory_order_acquire)) {
            auto& ctrl = m_sharedData->pe_control[m_pe_id];
            int cmd = ctrl.command.load(std::memory_order_acquire);

            switch (cmd) {
            case 0: // idle
                ctrl.running.store(false, std::memory_order_release);
                std::this_thread::sleep_for(1ms);
                break;

            case 1: // step
                std::cout << "[PE" << m_pe_id << "] Executing STEP\n";
                executeCycle();
                ctrl.command.store(0, std::memory_order_release);
                ctrl.running.store(false, std::memory_order_release);
                std::cout << "[PE" << m_pe_id << "] STEP completed\n";
                break;

            case 2: { // step_until
                int left = ctrl.step_count.load(std::memory_order_acquire);
                if (left <= 0) {
                    std::cout << "[PE" << m_pe_id << "] STEP_UNTIL finished (count reached 0)\n";
                    ctrl.command.store(0, std::memory_order_release);
                    ctrl.running.store(false, std::memory_order_release);
                    break;
                }
                std::cout << "[PE" << m_pe_id << "] Executing STEP_UNTIL (remaining: " << left << ")\n";
                executeCycle();
                ctrl.step_count.store(left - 1, std::memory_order_release);
                if (left - 1 <= 0 || ctrl.should_stop.load(std::memory_order_acquire)) {
                    std::cout << "[PE" << m_pe_id << "] STEP_UNTIL completed\n";
                    ctrl.command.store(0, std::memory_order_release);
                    ctrl.running.store(false, std::memory_order_release);
                    ctrl.should_stop.store(false, std::memory_order_release);
                }
                break;
            }

            case 3: { // step_infinite - AGREGADO: llaves para crear scope
                if (ctrl.should_stop.load(std::memory_order_acquire)) {
                    std::cout << "[PE" << m_pe_id << "] STEP_INFINITE stopped by user\n";
                    ctrl.command.store(0, std::memory_order_release);
                    ctrl.running.store(false, std::memory_order_release);
                    ctrl.should_stop.store(false, std::memory_order_release);
                    break;
                }
                // Solo imprimir cada 100 ciclos para no saturar
                static int infinite_counter = 0;
                if (infinite_counter % 100 == 0) {
                    std::cout << "[PE" << m_pe_id << "] STEP_INFINITE running (cycle " << infinite_counter << ")\n";
                }
                infinite_counter++;
                executeCycle();
                std::this_thread::sleep_for(10us);
                break;
            } // AGREGADO: cierre de llaves

            case 4: // reset
                std::cout << "[PE" << m_pe_id << "] Executing RESET\n";
                reset();
                ctrl.command.store(0, std::memory_order_release);
                ctrl.running.store(false, std::memory_order_release);
                std::cout << "[PE" << m_pe_id << "] RESET completed\n";
                break;

            default:
                std::cerr << "[PE" << m_pe_id << "] Unknown command: " << cmd << "\n";
                ctrl.command.store(0, std::memory_order_release);
                ctrl.running.store(false, std::memory_order_release);
                break;
            }
        }

        std::cout << "[PE" << m_pe_id << "] Thread ending\n";
    }

    // ============================================================================
    // EXECUTE CYCLE - AGREGAR PRINT
    // ============================================================================

    void PE1Component::executeCycle() {
        auto& instConn = m_sharedData->instruction_connections[m_pe_id];

        log_build_and_print([&](std::ostringstream& oss) {
            oss << "[PE" << m_pe_id << "] executeCycle() START - PC=0x"
                << std::hex << PC_F
                << " PC_F(shared)=0x" << instConn.PC_F.load(std::memory_order_acquire)
                << std::dec << "\n";
            });

        PCPlus8_F = PC_F + 8;

        stageWriteBack();
        stageMemory();
        stageExecute();
        stageDecode();
        stageFetch();

        updateInstructionTracking();

        // Actualizar flipflops
        if (!m_hazards.StallW) {
            MEM_WB = MEM_WB_next;
        }
        if (!m_hazards.StallM) {
            EX_MEM = EX_MEM_next;
        }
        if (!m_hazards.StallE) {
            if (m_hazards.FlushE) {
                // CRÍTICO: Preservar los flags durante FLUSH
                uint8_t preserved_flags = ID_EX.Flags_in;

                ID_EX = {};
                ID_EX.ALUControl_D = 0x22;  // NOTHING
                ID_EX.FlagsUpd_D = 0;       // ← NO actualizar flags
                ID_EX.Instr_D = FLUSH_INSTRUCTION;

                // Restaurar los flags preservados
                ID_EX.Flags_in = preserved_flags;
            }
            else {
                ID_EX = ID_EX_next;
            }
        }
        if (!m_hazards.StallD) {
            if (m_hazards.FlushD) {
                IF_ID.Instr_F = FLUSH_INSTRUCTION;
            }
            else {
                IF_ID = IF_ID_next;
            }
        }

        uint64_t old_pc = PC_F;
        if (!m_hazards.StallF) {
            PC_F = PC_prime;
        }

        instConn.PC_F.store(PC_F, std::memory_order_release);

        log_build_and_print([&](std::ostringstream& oss) {
            oss << "  [executeCycle] END - PC changed: "
                << std::hex << old_pc << " -> " << PC_F
                << " StallF=" << std::dec << m_hazards.StallF << "\n";
            });

        std::this_thread::sleep_for(std::chrono::microseconds(5));
    }

    // ============================================================================
    // STAGE: FETCH - VERSIÓN CORREGIDA
    // ============================================================================

    void PE1Component::stageFetch() {
        auto& instConn = m_sharedData->instruction_connections[m_pe_id];

        bool ins_ready = instConn.INS_READY.load(std::memory_order_acquire);
        uint64_t instr = instConn.InstrF.load(std::memory_order_acquire);

        log_build_and_print([&](std::ostringstream& oss) {
            oss << "  [stageFetch] PC=" << std::hex << PC_F
                << " INS_READY=" << std::dec << (int)ins_ready
                << " Instr=0x" << std::hex << instr << std::dec << "\n";
            });

        if (ins_ready) {
            IF_ID_next.Instr_F = instr;
            IF_ID_next.PC_F = PC_F;
        }
        else {
            // Hold: no “ensucies” el front con una instrucción aún no válida
            IF_ID_next = IF_ID;   // conserva lo que hay
        }
    }

    // ============================================================================
    // STAGE: DECODE
    // ============================================================================

    void PE1Component::stageDecode() {
        // Leer del flipflop IF/ID
        InstrD = IF_ID.Instr_F;
        PC_in = IF_ID.PC_F;

        // Descomponer instrucción
        Op_in = (InstrD >> 56) & 0xFF;
        Rd_in_D = (InstrD >> 52) & 0xF;
        Rn_in = (InstrD >> 48) & 0xF;
        Rm_in = (InstrD >> 44) & 0xF;
        Imm_in = (InstrD >> 12) & 0xFFFFFFFF;

        // Control Unit
        ctrlSignals = m_controlUnit.decode(Op_in);

        // --- INC/DEC: son unarias y leen el mismo Rd ---
        // Asegura que la HazardUnit vea dependencia RAW contra Rd
        if (Op_in == 0x1C || Op_in == 0x1D) {
            Rn_in = Rd_in_D;   // fuente
            Rm_in = Rd_in_D;   // también como Rm para ser conservadores
        }

        // --- SWI: si llega a Decode, detener ejecución (incluye STEP infinito) ---
        if (Op_in == 0x4C) {
            auto& ctrl = m_sharedData->pe_control[m_pe_id];
            const int currentCmd = ctrl.command.load(std::memory_order_acquire);
            const bool wasInfinite = (currentCmd == 3);

            // Señalizar STOP inmediato
            ctrl.should_stop.store(true, std::memory_order_release);
            ctrl.command.store(0, std::memory_order_release);   // regresar a idle
            ctrl.running.store(false, std::memory_order_release);

            // *** NUEVO: notificar al hilo principal que muestre popup ***
            m_sharedData->ui_signals[m_pe_id].swi_count.fetch_add(1, std::memory_order_acq_rel);

            // Mensajes para consola + log
            log_build_and_print([&](std::ostringstream& oss) {
                oss << "[PE" << m_pe_id << "] SWI reached Decode -> STOP"
                    << (wasInfinite ? " (infinite STEP disabled)" : "")
                    << " | PC=0x" << std::hex << PC_in << std::dec << "\n";
                });
            std::cout << "[PE" << m_pe_id << "] Se aplicó un SWI -> STOP\n";
        }

        // Register File (lectura)
        RD_Rn_out = m_registerFile.read(Rn_in);
        RD_Rm_out = m_registerFile.read(Rm_in);
        UPPER_out = m_registerFile.getUpper();
        LOWER_out = m_registerFile.getLower();

        // Compare for alignment
        SegmentationFault = false;
        if (PC_in < UPPER_out || PC_in > LOWER_out) {
            SegmentationFault = true;
            m_segmentationFault = true;
            log_build_and_print([&](std::ostringstream& oss) {
                oss << "[PE" << m_pe_id << "] SEGMENTATION FAULT at PC=0x"
                    << std::hex << PC_in << std::dec << "\n";
                });
        }

        // Extend
        Imm_ext_in = extendImmediate(Imm_in, ctrlSignals.DataType);

        // Mux de Branching
        SrcA_D = ctrlSignals.BranchE ? PC_in : RD_Rn_out;

        // Mux de Inmediato
        SrcB_D = ctrlSignals.ImmOp ? Imm_ext_in : RD_Rm_out;

        // Hazard detection
        auto& instConn = m_sharedData->instruction_connections[m_pe_id];
        bool ins_ready = instConn.INS_READY.load(std::memory_order_acquire);

        auto& cacheConn = m_sharedData->cache_connections[m_pe_id];
        bool c_request = EX_MEM.C_REQUEST_E;
        bool c_ready = cacheConn.C_READY.load(std::memory_order_acquire);

        m_hazards = m_hazardUnit.detect(
            ins_ready, c_request, c_ready,
            /*SegmentationFault*/ m_segmentationFault,  // <-- usar el sticky
            PCSrc_AND,
            MEM_WB.PCSrc_M,
            Rd_in_D, Rn_in, Rm_in,
            ID_EX.Rd_in_D, EX_MEM.Rd_in_E, MEM_WB.Rd_in_M,
            ID_EX.RegWrite_D, EX_MEM.RegWrite_E, MEM_WB.RegWrite_M,
            ctrlSignals.BranchE,
            ID_EX.BranchOp_D
        );

        // Hazards aplicados (atómico)
        log_build_and_print([&](std::ostringstream& oss) {
            oss << "  [stageDecode] Hazards applied: "
                << "StallF=" << m_hazards.StallF
                << " StallD=" << m_hazards.StallD
                << " FlushD=" << m_hazards.FlushD
                << " StallE=" << m_hazards.StallE
                << " FlushE=" << m_hazards.FlushE
                << " StallM=" << m_hazards.StallM
                << " StallW=" << m_hazards.StallW << "\n";
            });

        // Preparar next flipflop
        uint8_t rd = Rd_in_D;
        bool regWriteAllowed = ctrlSignals.RegWrite_D && (rd != REG_ZERO) && (rd != REG_PEID);
        ID_EX_next.RegWrite_D = regWriteAllowed;
        ID_EX_next.MemOp_D = ctrlSignals.MemOp_D;
        ID_EX_next.C_WE_D = ctrlSignals.C_WE_D;
        ID_EX_next.C_REQUEST_D = ctrlSignals.C_REQUEST_D;
        ID_EX_next.C_ISB_D = ctrlSignals.C_ISB_D;
        ID_EX_next.PCSrc_D = ctrlSignals.PCSrc_D;
        ID_EX_next.FlagsUpd_D = ctrlSignals.FlagsUpd_D;
        ID_EX_next.ALUControl_D = ctrlSignals.ALUControl_D;
        ID_EX_next.BranchOp_D = ctrlSignals.BranchOp_D;
        ID_EX_next.Flags_in = Flags_prime; // de Execute
        ID_EX_next.SrcA_D = SrcA_D;
        ID_EX_next.SrcB_D = SrcB_D;
        ID_EX_next.RD_Rm_out = RD_Rm_out;
        ID_EX_next.Rd_in_D = rd;

        ID_EX_next.Instr_D = InstrD;  // << ESTA ES LA INSTRUCCIÓN EN DECODE
    }

    // ============================================================================
    // STAGE: EXECUTE
    // ============================================================================

    void PE1Component::stageExecute() {
        // Leer del flipflop ID/EX
        uint8_t flags_e = ID_EX.Flags_in;
        uint64_t srcA = ID_EX.SrcA_D;
        uint64_t srcB = ID_EX.SrcB_D;
        uint8_t aluCtrl = ID_EX.ALUControl_D;

        // ALU
        ALU::Result aluRes = m_alu.execute(aluCtrl, srcA, srcB, flags_e);
        ALUResult_E = aluRes.value;
        ALUFlagsOut = aluRes.flags;

        // CondUnit
        if (ID_EX.FlagsUpd_D) {
            Flags_prime = ALUFlagsOut;
        }
        else {
            Flags_prime = flags_e;
        }

        CondExE = evaluateBranchCondition(ID_EX.BranchOp_D, Flags_prime);

        // AND de branching
        PCSrc_AND = ID_EX.PCSrc_D && CondExE;

        // NUEVO: Print de debug para flags
        if (ID_EX.BranchOp_D != 0 || ID_EX.FlagsUpd_D) {
            log_build_and_print([&](std::ostringstream& oss) {
                bool N = (Flags_prime & 0x8) != 0;
                bool Z = (Flags_prime & 0x4) != 0;
                bool C = (Flags_prime & 0x2) != 0;
                bool V = (Flags_prime & 0x1) != 0;
                oss << "  [stageExecute] Flags_prime=0x" << std::hex << (int)Flags_prime
                    << " (N=" << N << " Z=" << Z << " C=" << C << " V=" << V << ")"
                    << " BranchOp=" << std::dec << (int)ID_EX.BranchOp_D
                    << " CondExE=" << CondExE << " PCSrc_AND=" << PCSrc_AND << "\n";
                });
        }

        // Preparar next flipflop
        EX_MEM_next.RegWrite_E = ID_EX.RegWrite_D;
        EX_MEM_next.MemOp_E = ID_EX.MemOp_D;
        EX_MEM_next.C_WE_E = ID_EX.C_WE_D;
        EX_MEM_next.C_REQUEST_E = ID_EX.C_REQUEST_D;
        EX_MEM_next.C_ISB_E = ID_EX.C_ISB_D;
        EX_MEM_next.PCSrc_AND = PCSrc_AND;
        EX_MEM_next.ALUResult_E = ALUResult_E;
        EX_MEM_next.RD_Rm_Special_E = ID_EX.RD_Rm_out;
        EX_MEM_next.Rd_in_E = ID_EX.Rd_in_D;

        EX_MEM_next.Instr_E = ID_EX.Instr_D;
    }

    // ============================================================================
    // STAGE: MEMORY
    // ============================================================================

    void PE1Component::stageMemory() {
        // Leer del flipflop EX/MEM
        uint64_t aluOut = EX_MEM.ALUResult_E;
        uint64_t rdRm = EX_MEM.RD_Rm_Special_E;

        // Escribir señales a la cache (compartidas)
        auto& cacheConn = m_sharedData->cache_connections[m_pe_id];
        cacheConn.ALUOut_M.store(aluOut, std::memory_order_release);
        cacheConn.RD_Rm_Special_M.store(rdRm, std::memory_order_release);
        cacheConn.C_WE_M.store(EX_MEM.C_WE_E, std::memory_order_release);
        cacheConn.C_ISB_M.store(EX_MEM.C_ISB_E, std::memory_order_release);
        cacheConn.C_REQUEST_M.store(EX_MEM.C_REQUEST_E, std::memory_order_release);

        // Leer respuesta de la cache
        uint64_t rdCout = cacheConn.RD_C_out.load(std::memory_order_acquire);

        // Mux de memoria
        ALUOutM_O = EX_MEM.MemOp_E ? rdCout : aluOut;

        // Preparar next flipflop
        MEM_WB_next.RegWrite_M = EX_MEM.RegWrite_E;
        MEM_WB_next.PCSrc_M = EX_MEM.PCSrc_AND;
        MEM_WB_next.ALUOutM_O = ALUOutM_O;
        MEM_WB_next.Rd_in_M = EX_MEM.Rd_in_E;

        MEM_WB_next.Instr_M = EX_MEM.Instr_E;
    }

    // ============================================================================
    // STAGE: WRITEBACK
    // ============================================================================

    void PE1Component::stageWriteBack() {
        // Leer del flipflop MEM/WB
        uint8_t regWrite = MEM_WB.RegWrite_M;
        uint8_t pcsrc = MEM_WB.PCSrc_M;
        uint64_t aluOutW = MEM_WB.ALUOutM_O;
        uint8_t rdInW = MEM_WB.Rd_in_M;

        // Register File (escritura PRIMERO, luego lectura en Decode)
        m_registerFile.write(rdInW, aluOutW, regWrite);

        // Mux selector de PC
        PC_prime = pcsrc ? aluOutW : PCPlus8_F;
    }

    // ============================================================================

    void PE1Component::updateInstructionTracking() {
        // Snapshot *después* del latch de este ciclo:
        std::array<uint64_t, 5> snap;

        // F: lo recién fetcheado este ciclo
        snap[0] = IF_ID_next.Instr_F;
        // D..W: lo que quedó latcheado tras aplicar stalls/flushes
        snap[1] = IF_ID.Instr_F;
        snap[2] = ID_EX.Instr_D;
        snap[3] = EX_MEM.Instr_E;
        snap[4] = MEM_WB.Instr_M;

        m_stageInstructions = snap;

        // (Opcional pero recomendado) Usa el desensamblador consistente
        // en el log, para evitar discrepancias NAME vs UI.
        log_build_and_print([&](std::ostringstream& oss) {
            oss << "  [PIPELINE] F: " << InstructionDisassembler::disassemble(snap[0])
                << " | D: " << InstructionDisassembler::disassemble(snap[1])
                << " | E: " << InstructionDisassembler::disassemble(snap[2])
                << " | M: " << InstructionDisassembler::disassemble(snap[3])
                << " | W: " << InstructionDisassembler::disassemble(snap[4]) << "\n";
            });

        // Publicación a la UI: escribe con "seqlock" simple para evitar tearing visual
        if (m_sharedData) {
            auto& track = m_sharedData->pe_instruction_tracking[m_pe_id];
            track.version.fetch_add(1, std::memory_order_acq_rel); // impar = escribiendo
            for (int i = 0; i < 5; ++i) {
                track.stage_instructions[i].store(snap[i], std::memory_order_release);
            }
            track.version.fetch_add(1, std::memory_order_acq_rel); // par = estable
        }
    }

} // namespace cpu_tlp