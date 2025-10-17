#include "programs/cpu_tlp_shared_cache/components/Assembler.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <algorithm> // Para std::transform
#include <cctype>    // Para std::toupper
#include <vector>    // Para std::vector
#include <string>    // Para std::string
#include <unordered_map> // Para std::unordered_map
#include <tuple>     // Para std::tuple

// ====================================================================
//                         UTILIDADES ROBUSTAS
// ====================================================================

std::string Assembler::up(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return r;
}

std::string Assembler::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::vector<std::string> Assembler::splitOperands(const std::string& s) {
    std::vector<std::string> r;
    std::string trimmed_input = trim(s);
    if (trimmed_input.empty()) return r;

    size_t start = 0;
    size_t pos = 0;

    while ((pos = trimmed_input.find(',', start)) != std::string::npos) {
        std::string token = trim(trimmed_input.substr(start, pos - start));
        if (!token.empty()) {
            r.push_back(token);
        }
        start = pos + 1;
    }

    // Agregar el último token (después de la última coma o todo si no hay comas)
    std::string last_token = trim(trimmed_input.substr(start));
    if (!last_token.empty()) {
        r.push_back(last_token);
    }

    return r;
}

bool Assembler::isImmediate(const std::string& t) {
    return !t.empty() && t[0] == '#';
}

long Assembler::parseIntImm(const std::string& t) {
    try {
        if (t.size() > 1) {
            return std::stol(t.substr(1));
        } else {
            throw std::runtime_error("Inmediato vacío: " + t);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Inmediato entero inválido: " + t + " (" + e.what() + ")");
    }
}

uint32_t Assembler::floatToU32(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

// ====================================================================
//                             ENSAMBLADOR
// ====================================================================

std::vector<Assembler::u64> Assembler::assembleFile(const std::string& path) {
    static const std::unordered_map<std::string, uint8_t> OPCODE_MAP = {
        {"ADD", 0x00}, {"SUB", 0x01}, {"ADC", 0x02}, {"SBC", 0x03},
        {"ADDI", 0x0E}, {"AND", 0x06}, {"ANDI", 0x06},
        {"MULI", 0x12}, {"INC", 0x1C},
        {"MOV", 0x31}, {"MOVI", 0x33},
        {"FMOVI", 0x35},
        {"CMP", 0x37}, {"CMPI", 0x3B},
        {"FCMP", 0x3F}, {"FCMPI", 0x42},
        {"BEQ", 0x46}, {"BNE", 0x47}, {"BLT", 0x48},
        {"BGT", 0x49}, {"BUN", 0x4A}, {"BORD", 0x4B},
        {"LDR", 0x4E}, {"STR", 0x4F},
        {"NOP", 0x4D}, {"SWI", 0x4C},
        {"FMUL", 0x20}, {"FDIV", 0x21}, {"FSQRT", 0x28}
    };

    static const std::unordered_map<std::string, uint8_t> REGS = {
        {"REG0", 0x0}, {"REG1", 0x1}, {"REG2", 0x2}, {"REG3", 0x3},
        {"REG4", 0x4}, {"REG5", 0x5}, {"REG6", 0x6}, {"REG7", 0x7},
        {"REG8", 0x8}, {"SIDS", 0x9}, {"UPPER_REG", 0xA}, {"LOWER_REG", 0xB}
    };

    std::ifstream ifs(path);
    if (!ifs) throw std::runtime_error("No se pudo abrir el archivo: " + path);

    // --- PRIMERA PASADA: Coleccionar instrucciones y etiquetas ---
    struct LineInfo { int lineno; std::string text; };
    std::vector<LineInfo> lines;
    std::string raw;
    int lineno = 1;
    while (std::getline(ifs, raw)) {
        if (!raw.empty() && raw.back() == '\r') {
            raw.pop_back();
        }
        size_t p = raw.find_first_of(";");
        if (p != std::string::npos) raw = raw.substr(0, p);
        lines.push_back({ lineno++, raw });
    }

    std::vector<std::tuple<int, std::string, int>> insts;
    std::unordered_map<std::string, int> labels;
    int pc = 0;
    for (auto& ln : lines) {
        std::string s = trim(ln.text);
        if (s.empty()) continue;
        
        while (true) {
            size_t col = s.find(':');
            if (col == std::string::npos) break;
            std::string lbl = trim(s.substr(0, col));
            if (!lbl.empty()) labels[up(lbl)] = pc;
            s = trim(s.substr(col + 1));
        }

        if (!s.empty()) {
            insts.emplace_back(pc, s, ln.lineno);
            ++pc;
        }
    }

    // --- SEGUNDA PASADA: Ensamblado ---
    std::vector<u64> out;
    std::vector<std::string> errors; // Vector para acumular errores

    std::cout << "[Assembler] Iniciando ensamblado: " << path << " (" << insts.size() << " instrucciones)\n";

    for (const auto& p : insts) {
        int curpc = std::get<0>(p);
        std::string text = std::get<1>(p);
        int lineNumber = std::get<2>(p);

        try {
            std::stringstream line_ss(text);
            std::string opc, rest;

            line_ss >> opc;
            std::getline(line_ss, rest);
            rest = trim(rest);

            std::string opcU = up(opc);
            if (OPCODE_MAP.find(opcU) == OPCODE_MAP.end()) {
                throw std::runtime_error("Opcode desconocido: '" + opc + "'");
            }

            uint8_t opcval = OPCODE_MAP.at(opcU);
            auto ops = splitOperands(rest);
            u64 word = 0;
            word |= (u64(opcval) & 0xFFULL) << 56;
            bool encoded = false;

            // 1. Tipo R (3 Operandos de Registro: RD, RN, RM)
            if (ops.size() == 3 && !isImmediate(ops[2])) {
                std::string rd = up(ops[0]), rn = up(ops[1]), rm = up(ops[2]);
                if (REGS.count(rd) == 0 || REGS.count(rn) == 0 || REGS.count(rm) == 0)
                    throw std::runtime_error("Registro inválido en instrucción de 3 operandos.");
                word |= (u64(REGS.at(rd)) & 0xFULL) << 52;
                word |= (u64(REGS.at(rn)) & 0xFULL) << 48;
                word |= (u64(REGS.at(rm)) & 0xFULL) << 44;
                encoded = true;
            }
            
            // 2. Tipo R (2 Operandos de Registro: RD, RN) -> RM=0
            else if (ops.size() == 2 && !isImmediate(ops[1])) {
                std::string rd = up(ops[0]), rn = up(ops[1]);
                if (REGS.count(rd) == 0 || REGS.count(rn) == 0)
                    throw std::runtime_error("Registro inválido en instrucción de 2 operandos.");
                word |= (u64(REGS.at(rd)) & 0xFULL) << 52;
                word |= (u64(REGS.at(rn)) & 0xFULL) << 48;
                encoded = true;
            }

            // 3. Tipo I (Con Inmediato)
            else if (!ops.empty() && isImmediate(ops.back())) {
                if (ops.size() < 2 || ops.size() > 3)
                    throw std::runtime_error("Instrucción con inmediato requiere 2 o 3 operandos.");
                
                std::string rd_s = up(ops[0]);
                if (REGS.count(rd_s) == 0) throw std::runtime_error("Registro destino inválido: " + ops[0]);
                
                uint8_t rd = REGS.at(rd_s);
                uint8_t rn = rd;

                if (ops.size() == 3) {
                    std::string rn_s = up(ops[1]);
                    if (isImmediate(rn_s)) throw std::runtime_error("Segundo operando no puede ser inmediato.");
                    if (REGS.count(rn_s) == 0) throw std::runtime_error("Registro fuente inválido: " + ops[1]);
                    rn = REGS.at(rn_s);
                }

                uint32_t imm_u32 = 0;
                std::string immtok = ops.back();
                if (immtok.find('.') != std::string::npos && opcU[0] == 'F') {
                    imm_u32 = floatToU32(std::stof(immtok.substr(1)));
                } else {
                    imm_u32 = static_cast<uint32_t>(parseIntImm(immtok));
                }

                word |= (u64(rd) & 0xFULL) << 52;
                word |= (u64(rn) & 0xFULL) << 48;
                // **CORRECCIÓN CRÍTICA: Sin desplazamiento de 12 bits**
                word |= (u64(imm_u32) & 0xFFFFFFFFULL);
                encoded = true;
            }

            // 4. Branch (1 Operando de Etiqueta)
            else if (ops.size() == 1 && !isImmediate(ops[0]) && opcU[0] == 'B') {
                std::string target = up(ops[0]);
                if (labels.count(target) == 0) {
                    throw std::runtime_error("Etiqueta no encontrada: '" + ops[0] + "'");
                }
                long offset = (long)labels.at(target) - (long)(curpc + 1);
                // **CORRECCIÓN CRÍTICA: Sin desplazamiento de 12 bits**
                word |= (u64(static_cast<uint32_t>(offset)) & 0xFFFFFFFFULL);
                encoded = true;
            }

            // 5. Sin operandos (NOP, SWI)
            else if (ops.empty()) {
                encoded = true;
            }
            
            if (!encoded) {
                throw std::runtime_error("Formato de instrucción no válido o número de operandos incorrecto.");
            }
            
            out.push_back(word);

        } catch (const std::exception& e) {
            std::stringstream error_ss;
            error_ss << "[Línea " << lineNumber << "] " << e.what() << " -> \"" << text << "\"";
            errors.push_back(error_ss.str());
        }
    }

    if (!errors.empty()) {
        std::stringstream final_error;
        final_error << "El ensamblado falló con " << errors.size() << " error(es):\n";
        for (const auto& err : errors) {
            final_error << "- " << err << "\n";
        }
        throw std::runtime_error(final_error.str());
    }

    std::cout << "[Assembler] Ensamblado completado. " << out.size() << " instrucciones.\n";
    return out;
}