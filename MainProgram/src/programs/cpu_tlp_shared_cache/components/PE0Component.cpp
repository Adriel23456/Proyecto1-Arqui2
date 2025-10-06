#include "programs/cpu_tlp_shared_cache/components/PE0Component.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>

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

    PE0Component::RegisterFile::RegisterFile() {
        reset();
    }

    void PE0Component::RegisterFile::reset() {
        regs.fill(0ULL);
        regs[11] = 0xFFFFFFFFFFFFFFFFULL; // LOWER_REG
        regs[9] = 0ULL; // PEID
    }

    uint64_t PE0Component::RegisterFile::read(uint8_t addr) const {
        if (addr >= 12) return 0ULL;
        return regs[addr];
    }

    void PE0Component::RegisterFile::write(uint8_t addr, uint64_t value, bool we) {
        if (!we) return;
        if (addr == 0) return; // REG0 inmutable
        if (addr >= 12) return;
        regs[addr] = value;

        // NUEVO: Notificar al snapshot (se configurará desde fuera)
        if (onRegisterWrite) {
            onRegisterWrite(addr, value);
        }
    }

    // ============================================================================
    // ALU
    // ============================================================================

    PE0Component::ALU::Result PE0Component::ALU::execute(uint8_t control, uint64_t A, uint64_t B, uint8_t flagsIn) {
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
        case 0x0E: // FADD
            res.value = doubleToBits(bitsToDouble(A) + bitsToDouble(B));
            break;
        case 0x0F: // FSUB
            res.value = doubleToBits(bitsToDouble(A) - bitsToDouble(B));
            break;
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
        case 0x20: // FCMPS
        {
            double da = bitsToDouble(A);
            double db = bitsToDouble(B);
            if (std::isnan(da) || std::isnan(db)) {
                res.flags = 0x1; // V=1 (unordered)
            }
            else {
                double diff = da - db;
                bool N = diff < 0;
                bool Z = diff == 0;
                bool C = da >= db;
                res.flags = (N ? 0x8 : 0) | (Z ? 0x4 : 0) | (C ? 0x2 : 0);
            }
            res.value = 0;
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

    PE0Component::ControlUnit::Signals PE0Component::ControlUnit::decode(uint8_t opcode) {
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

            // 0x1C-0x1D: MOV especiales
        case 0x1C: case 0x1D:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 1; sig.DataType = 0;
            break;

            // 0x1E-0x22: Operaciones floating point sin inmediato
        case 0x1E: case 0x1F: case 0x20: case 0x21: case 0x22:
            sig.RegWrite_D = 1; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode;
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
        case 0x37: case 0x38: case 0x39: case 0x3A:
        case 0x3B: case 0x3C: case 0x3D: case 0x3E:
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode - 0x37;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = 0; sig.DataType = 0;
            break;

            // 0x3F-0x44: Comparaciones floating point
        case 0x3F: case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 0;
            sig.FlagsUpd_D = 1; sig.ALUControl_D = opcode - 0x3F + 0x1E;
            sig.BranchOp_D = 0; sig.BranchE = 0; sig.ImmOp = (opcode >= 0x42 ? 1 : 0);
            sig.DataType = (opcode >= 0x42 ? 1 : 0);
            break;

            // 0x45-0x4B: Branches
        case 0x45: case 0x46: case 0x47: case 0x48:
        case 0x49: case 0x4A: case 0x4B:
            sig.RegWrite_D = 0; sig.MemOp_D = 0; sig.C_WE_D = 0;
            sig.C_REQUEST_D = 0; sig.C_ISB_D = 0; sig.PCSrc_D = 1;
            sig.FlagsUpd_D = 0; sig.ALUControl_D = 0;
            sig.BranchOp_D = opcode - 0x45 + 1; sig.BranchE = 0;
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
    // HAZARDUNIT
    // ============================================================================

    PE0Component::HazardUnit::Outputs PE0Component::HazardUnit::detect(
        bool INS_READY,
        bool C_REQUEST_M, bool C_READY,
        bool SegmentationFault,
        bool PCSrc_AND,
        uint8_t Rd_in_D, uint8_t Rn_in, uint8_t Rm_in,
        uint8_t Rd_in_E, uint8_t Rd_in_M, uint8_t Rd_in_W,
        bool RegWrite_E, bool RegWrite_M, bool RegWrite_W,
        bool BranchE
    ) {
        Outputs out;
        out.StallF = out.StallD = out.StallE = out.StallM = out.StallW = false;
        out.FlushD = out.FlushE = false;

        // 1. SegmentationFault: FULL STALL
        if (SegmentationFault) {
            out.StallF = out.StallD = out.StallE = out.StallM = out.StallW = true;
            std::cerr << "[PE HazardUnit] SegmentationFault detected! Full stall.\n";
            return out;
        }

        // 2. InstructionLatency: FULL STALL hasta INS_READY
        if (!INS_READY) {
            out.StallF = out.StallD = out.StallE = out.StallM = out.StallW = true;
            return out;
        }

        // 3. CacheLatency: FULL STALL mientras C_REQUEST && !C_READY
        if (C_REQUEST_M && !C_READY) {
            out.StallF = out.StallD = out.StallE = out.StallM = out.StallW = true;
            return out;
        }

        // 4. RAW: Detección de dependencias
        bool rawHazard = false;

        // Verificar si Rn o Rm dependen de Rd en etapas posteriores
        if (Rn_in != 0) { // REG0 nunca genera hazard
            if (RegWrite_E && Rd_in_E != 0 && Rd_in_E == Rn_in) rawHazard = true;
            if (RegWrite_M && Rd_in_M != 0 && Rd_in_M == Rn_in) rawHazard = true;
        }
        if (Rm_in != 0) {
            if (RegWrite_E && Rd_in_E != 0 && Rd_in_E == Rm_in) rawHazard = true;
            if (RegWrite_M && Rd_in_M != 0 && Rd_in_M == Rm_in) rawHazard = true;
        }

        if (rawHazard) {
            out.StallF = out.StallD = true;
            out.FlushE = true;
            return out;
        }

        // 5. BRANCHING
        if (BranchE) {
            if (!branchActive) {
                branchActive = true;
                branchCycles = 0;
            }
        }

        if (branchActive) {
            branchCycles++;

            if (PCSrc_AND) {
                // Branch tomado: 4 flushes en D
                if (branchCycles <= 4) {
                    out.StallF = true;
                    out.FlushD = true;
                }
                else {
                    branchActive = false;
                }
            }
            else {
                // Branch no tomado: 1 flush en D
                if (branchCycles <= 1) {
                    out.StallF = true;
                    out.FlushD = true;
                }
                else {
                    branchActive = false;
                }
            }
        }

        return out;
    }

    // ============================================================================
    // HELPERS
    // ============================================================================

    uint64_t PE0Component::extendImmediate(uint32_t imm, bool dataType) {
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

    bool PE0Component::evaluateBranchCondition(uint8_t branchOp, uint8_t flags) {
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

    PE0Component::PE0Component(int pe_id)
        : m_pe_id(pe_id)
        , m_sharedData(nullptr)
        , m_executionThread(nullptr)
        , m_isRunning(false)
        , m_segmentationFault(false)
    {
        reset();
    }

    PE0Component::~PE0Component() {
        shutdown();
    }

    bool PE0Component::initialize(std::shared_ptr<CPUSystemSharedData> sharedData) {
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
        m_executionThread = std::make_unique<std::thread>(&PE0Component::threadMain, this);

        std::cout << "[PE" << m_pe_id << "] Initialized successfully\n";
        return true;
    }

    void PE0Component::shutdown() {
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

    bool PE0Component::isRunning() const {
        return m_isRunning.load(std::memory_order_acquire);
    }

    void PE0Component::reset() {
        // Reset del pipeline
        PC_F = 0x0;
        PCPlus8_F = 0x0;

        IF_ID = IF_ID_next = {};
        IF_ID.Instr_F = NOP_INSTRUCTION;
        IF_ID.PC_F = 0x0;

        ID_EX = ID_EX_next = {};
        ID_EX.ALUControl_D = 0x22;

        EX_MEM = EX_MEM_next = {};
        MEM_WB = MEM_WB_next = {};

        InstrD = NOP_INSTRUCTION;
        PC_in = 0x0;
        SegmentationFault = false;
        m_segmentationFault = false;

        // Reset register file
        m_registerFile.reset();
        m_registerFile.setPEID(m_pe_id);

        // NUEVO: Sincronizar snapshot después del reset
        if (m_sharedData) {
            for (int i = 0; i < 12; ++i) {
                uint64_t val = m_registerFile.read(i);
                m_sharedData->pe_registers[m_pe_id].registers[i].store(val, std::memory_order_release);
            }
        }

        m_hazards = {};

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

    void PE0Component::step() {
        if (!m_sharedData) {
            std::cerr << "[PE" << m_pe_id << "] step() called but no shared data!\n";
            return;
        }
        auto& ctrl = m_sharedData->pe_control[m_pe_id];
        ctrl.command.store(1, std::memory_order_release);
        ctrl.running.store(true, std::memory_order_release);
        std::cout << "[PE" << m_pe_id << "] step() command issued\n";
    }

    void PE0Component::stepUntil(int value) {
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

    void PE0Component::stepIndefinitely() {
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

    void PE0Component::stopExecution() {
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

    void PE0Component::threadMain() {
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

    void PE0Component::executeCycle() {
        std::cout << "[PE" << m_pe_id << "] executeCycle() - PC=0x" << std::hex << PC_F << std::dec << "\n";

        // Orden inverso: WriteBack -> Memory -> Execute -> Decode -> Fetch
        stageWriteBack();
        stageMemory();
        stageExecute();
        stageDecode();
        stageFetch();

        // Actualizar flipflops (simula flanco de reloj)
        if (!m_hazards.StallW) {
            MEM_WB = MEM_WB_next;
        }
        if (!m_hazards.StallM) {
            EX_MEM = EX_MEM_next;
        }
        if (!m_hazards.StallE) {
            if (m_hazards.FlushE) {
                ID_EX = {}; // flush
                ID_EX.ALUControl_D = 0x22; // NOTHING
            }
            else {
                ID_EX = ID_EX_next;
            }
        }
        if (!m_hazards.StallD) {
            if (m_hazards.FlushD) {
                IF_ID.Instr_F = NOP_INSTRUCTION;
            }
            else {
                IF_ID = IF_ID_next;
            }
        }
        if (!m_hazards.StallF) {
            PC_F = PC_prime;
        }
    }

    // ============================================================================
    // STAGE: FETCH
    // ============================================================================

    void PE0Component::stageFetch() {
        auto& instConn = m_sharedData->instruction_connections[m_pe_id];

        // Escribir PC_F a la memoria de instrucciones
        instConn.PC_F.store(PC_F, std::memory_order_release);

        // Calcular PC + 8
        PCPlus8_F = PC_F + 8;

        // Leer INS_READY e Instr_F
        bool ins_ready = instConn.INS_READY.load(std::memory_order_acquire);
        uint64_t instr = instConn.InstrF.load(std::memory_order_acquire);

        // Preparar next flipflop
        IF_ID_next.Instr_F = instr;
        IF_ID_next.PC_F = PC_F;

        // Detectar hazards (incluye INS_READY)
        // Se hará en stageDecode después de tener toda la info
    }

    // ============================================================================
    // STAGE: DECODE
    // ============================================================================

    void PE0Component::stageDecode() {
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
            std::cerr << "[PE" << m_pe_id << "] SEGMENTATION FAULT at PC=0x"
                << std::hex << PC_in << std::dec << "\n";
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
            ins_ready, c_request, c_ready, SegmentationFault, PCSrc_AND,
            Rd_in_D, Rn_in, Rm_in,
            ID_EX.Rd_in_D, EX_MEM.Rd_in_E, MEM_WB.Rd_in_M,
            ID_EX.RegWrite_D, EX_MEM.RegWrite_E, MEM_WB.RegWrite_M,
            ctrlSignals.BranchE
        );

        // Preparar next flipflop
        ID_EX_next.RegWrite_D = ctrlSignals.RegWrite_D;
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
        ID_EX_next.Rd_in_D = Rd_in_D;
    }

    // ============================================================================
    // STAGE: EXECUTE
    // ============================================================================

    void PE0Component::stageExecute() {
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
    }

    // ============================================================================
    // STAGE: MEMORY
    // ============================================================================

    void PE0Component::stageMemory() {
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
    }

    // ============================================================================
    // STAGE: WRITEBACK
    // ============================================================================

    void PE0Component::stageWriteBack() {
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

} // namespace cpu_tlp
    

    