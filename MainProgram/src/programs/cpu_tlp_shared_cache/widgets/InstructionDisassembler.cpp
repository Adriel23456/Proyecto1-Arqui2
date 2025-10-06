#include "programs/cpu_tlp_shared_cache/widgets/InstructionDisassembler.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace cpu_tlp {

    const char* InstructionDisassembler::getOpcodeMnemonic(uint8_t opcode) {
        switch (opcode) {
            // Operaciones enteras básicas
        case 0x00: return "ADD";
        case 0x01: return "SUB";
        case 0x02: return "ADC";
        case 0x03: return "SBC";
        case 0x04: return "MUL";
        case 0x05: return "DIV";
        case 0x06: return "AND";
        case 0x07: return "ORR";
        case 0x08: return "EOR";
        case 0x09: return "BIC";
        case 0x0A: return "LSL";
        case 0x0B: return "LSR";
        case 0x0C: return "ASR";
        case 0x0D: return "ROR";

            // Operaciones con inmediato
        case 0x0E: return "ADDI";
        case 0x0F: return "SUBI";
        case 0x10: return "ADCI";
        case 0x11: return "SBCI";
        case 0x12: return "MULI";
        case 0x13: return "DIVI";
        case 0x14: return "ANDI";
        case 0x15: return "ORRI";
        case 0x16: return "EORI";
        case 0x17: return "BICI";
        case 0x18: return "LSLI";
        case 0x19: return "LSRI";
        case 0x1A: return "ASRI";
        case 0x1B: return "RORI";
        case 0x1C: return "INC";
        case 0x1D: return "DEC";

            // Operaciones float
        case 0x1E: return "FADD";
        case 0x1F: return "FSUB";
        case 0x20: return "FMUL";
        case 0x21: return "FDIV";
        case 0x22: return "FCOPYSIGN";
        case 0x23: return "FADDI";
        case 0x24: return "FSUBI";
        case 0x25: return "FMULI";
        case 0x26: return "FDIVI";
        case 0x27: return "FCOPYSIGNI";
        case 0x28: return "FSQRT";
        case 0x29: return "FNEG";
        case 0x2A: return "FABS";
        case 0x2B: return "CDTI";
        case 0x2C: return "CDTD";
        case 0x2D: return "RTNR";
        case 0x2E: return "RTZ";
        case 0x2F: return "RTP";
        case 0x30: return "RTNE";

            // MOV variants
        case 0x31: return "MOV";
        case 0x32: return "MVN";
        case 0x33: return "MOVI";
        case 0x34: return "MVNI";
        case 0x35: return "FMOVI";
        case 0x36: return "FMVNI";

            // Comparaciones
        case 0x37: return "CMP";
        case 0x38: return "CMN";
        case 0x39: return "TST";
        case 0x3A: return "TEQ";
        case 0x3B: return "CMPI";
        case 0x3C: return "CMNI";
        case 0x3D: return "TSTI";
        case 0x3E: return "TEQI";
        case 0x3F: return "FCMP";
        case 0x40: return "FCMN";
        case 0x41: return "FCMPS";
        case 0x42: return "FCMPI";
        case 0x43: return "FCMNI";
        case 0x44: return "FCMPSI";

            // Branches
        case 0x45: return "B";
        case 0x46: return "BEQ";
        case 0x47: return "BNE";
        case 0x48: return "BLT";
        case 0x49: return "BGT";
        case 0x4A: return "BUN";
        case 0x4B: return "BORD";

            // Especiales
        case 0x4C: return "SWI";
        case 0x4D: return "NOP";

            // Memoria
        case 0x4E: return "LDR";
        case 0x4F: return "STR";
        case 0x50: return "LDRB";
        case 0x51: return "STRB";

            // Float con inmediato
        case 0x52: return "FSQRTI";
        case 0x53: return "FNEGI";
        case 0x54: return "FABSI";
        case 0x55: return "CDTII";
        case 0x56: return "CDTDI";
        case 0x57: return "RTNRI";
        case 0x58: return "RTZI";
        case 0x59: return "RTPI";
        case 0x5A: return "RTNEI";

        default: return "???";
        }
    }

    std::string InstructionDisassembler::getRegisterName(uint8_t regCode) {
        switch (regCode) {
        case 0x0: return "REG0";
        case 0x1: return "REG1";
        case 0x2: return "REG2";
        case 0x3: return "REG3";
        case 0x4: return "REG4";
        case 0x5: return "REG5";
        case 0x6: return "REG6";
        case 0x7: return "REG7";
        case 0x8: return "REG8";
        case 0x9: return "PEID";
        case 0xA: return "UPPER";
        case 0xB: return "LOWER";
        default: return "???";
        }
    }

    int32_t InstructionDisassembler::signExtend32(uint32_t value) {
        if (value & 0x80000000) {
            return static_cast<int32_t>(value | 0xFFFFFFFF00000000ULL);
        }
        return static_cast<int32_t>(value);
    }

    std::string InstructionDisassembler::formatImmediate(uint32_t imm, bool isFloat) {
        std::ostringstream oss;

        if (isFloat) {
            // Convertir float32 a double y mostrar
            float f;
            std::memcpy(&f, &imm, 4);
            oss << "#" << std::fixed << std::setprecision(6) << static_cast<double>(f);
        }
        else {
            // Mostrar como decimal con signo
            int32_t signedImm = signExtend32(imm);
            oss << "#" << signedImm;
        }

        return oss.str();
    }

    std::string InstructionDisassembler::disassemble(uint64_t instruction) {
        // Extraer campos
        uint8_t opcode = (instruction >> 56) & 0xFF;
        uint8_t rd = (instruction >> 52) & 0xF;
        uint8_t rn = (instruction >> 48) & 0xF;
        uint8_t rm = (instruction >> 44) & 0xF;
        uint32_t imm = (instruction >> 12) & 0xFFFFFFFF;

        const char* mnemonic = getOpcodeMnemonic(opcode);
        std::ostringstream result;
        result << mnemonic;

        // Determinar formato según opcode
        switch (opcode) {
            // Rd, Rn, Rm (3 registros)
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D:
        case 0x1E: case 0x1F: case 0x20: case 0x21: case 0x22:
            result << " " << getRegisterName(rd) << ", "
                << getRegisterName(rn) << ", "
                << getRegisterName(rm);
            break;

            // Rd, Rn, #imm (entero)
        case 0x0E: case 0x0F: case 0x10: case 0x11:
        case 0x12: case 0x13: case 0x14: case 0x15:
        case 0x16: case 0x17: case 0x18: case 0x19:
        case 0x1A: case 0x1B:
        case 0x3B: case 0x3C: case 0x3D: case 0x3E:
            result << " " << getRegisterName(rd) << ", "
                << getRegisterName(rn) << ", "
                << formatImmediate(imm, false);
            break;

            // Rd, Rn, #imm (float)
        case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x42: case 0x43: case 0x44:
            result << " " << getRegisterName(rd) << ", "
                << getRegisterName(rn) << ", "
                << formatImmediate(imm, true);
            break;

            // Rd, Rm (2 registros)
        case 0x28: case 0x29: case 0x2A: case 0x2B:
        case 0x2C: case 0x2D: case 0x2E: case 0x2F: case 0x30:
        case 0x31: case 0x32:
            result << " " << getRegisterName(rd) << ", "
                << getRegisterName(rm);
            break;

            // Rd, #imm (MOV variants - entero)
        case 0x1C: case 0x1D: case 0x33: case 0x34:
            result << " " << getRegisterName(rd) << ", "
                << formatImmediate(imm, false);
            break;

            // Rd, #imm (MOV variants - float)
        case 0x35: case 0x36:
        case 0x52: case 0x53: case 0x54: case 0x55:
        case 0x56: case 0x57: case 0x58: case 0x59: case 0x5A:
            result << " " << getRegisterName(rd) << ", "
                << formatImmediate(imm, true);
            break;

            // Rn, Rm (comparaciones)
        case 0x37: case 0x38: case 0x39: case 0x3A:
        case 0x3F: case 0x40: case 0x41:
            result << " " << getRegisterName(rn) << ", "
                << getRegisterName(rm);
            break;

            // Branches: etiqueta (mostrar como decimal)
        case 0x45: case 0x46: case 0x47: case 0x48:
        case 0x49: case 0x4A: case 0x4B:
        {
            int32_t offset = signExtend32(imm);
            result << " label_" << offset;
            break;
        }

        // Memoria: Rd, [Rn, #offset] o Rm, [Rn, #offset]
        case 0x4E: case 0x50: // LDR, LDRB
        {
            int32_t offset = signExtend32(imm);
            result << " " << getRegisterName(rd) << ", ["
                << getRegisterName(rn);
            if (offset != 0) {
                result << ", #" << offset;
            }
            result << "]";
            break;
        }

        case 0x4F: case 0x51: // STR, STRB
        {
            int32_t offset = signExtend32(imm);
            result << " " << getRegisterName(rm) << ", ["
                << getRegisterName(rn);
            if (offset != 0) {
                result << ", #" << offset;
            }
            result << "]";
            break;
        }

        // Sin operandos
        case 0x4C: case 0x4D: // SWI, NOP
            break;

        default:
            result << " (unknown format)";
            break;
        }

        return result.str();
    }

} // namespace cpu_tlp