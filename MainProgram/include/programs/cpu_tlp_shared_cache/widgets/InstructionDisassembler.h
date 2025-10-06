#pragma once
#include <string>
#include <cstdint>
#include <cstring>

namespace cpu_tlp {

    class InstructionDisassembler {
    public:
        // Función estática principal - convierte instrucción binaria a string
        static std::string disassemble(uint64_t instruction);

    private:
        // Helpers internos
        static std::string getRegisterName(uint8_t regCode);
        static std::string formatImmediate(uint32_t imm, bool isFloat);
        static int32_t signExtend32(uint32_t value);

        // Tablas de mnemonics
        static const char* getOpcodeMnemonic(uint8_t opcode);
    };

} // namespace cpu_tlp