#include "programs/cpu_tlp_shared_cache/components/Assembler.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>

// ═══════════════════════════════════════════════════════════════════════════
//                           UTILIDADES DE PROCESAMIENTO
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Convierte una cadena a mayúsculas
 */
std::string Assembler::up(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return r;
}

/**
 * Elimina espacios en blanco al inicio y final de la cadena
 */
std::string Assembler::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

/**
 * Separa operandos por comas, eliminando espacios
 */
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

    std::string last_token = trim(trimmed_input.substr(start));
    if (!last_token.empty()) {
        r.push_back(last_token);
    }

    return r;
}

/**
 * Verifica si un token es un inmediato (comienza con '#')
 */
bool Assembler::isImmediate(const std::string& t) {
    return !t.empty() && t[0] == '#';
}

/**
 * Parsea un inmediato entero (elimina el '#' y convierte)
 * Soporta tanto decimal (#123, #-45) como hexadecimal (#0x800, #0xFF)
 */
long Assembler::parseIntImm(const std::string& t) {
    try {
        if (t.size() <= 1) {
            throw std::runtime_error("Inmediato vacio: " + t);
        }

        std::string num_str = t.substr(1); // Eliminar el '#'

        // Detectar si es hexadecimal (comienza con 0x o 0X)
        if (num_str.size() >= 2 &&
            num_str[0] == '0' &&
            (num_str[1] == 'x' || num_str[1] == 'X')) {
            // Hexadecimal: usar base 16
            return std::stol(num_str, nullptr, 16);
        }
        else {
            // Decimal: usar base 10 (soporta negativos)
            return std::stol(num_str, nullptr, 10);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Inmediato entero invalido: " + t + " (" + e.what() + ")");
    }
}

/**
 * Convierte un float a su representación binaria uint32_t (IEEE 754)
 */
uint32_t Assembler::floatToU32(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

// ═══════════════════════════════════════════════════════════════════════════
//                              ENSAMBLADOR PRINCIPAL
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Assembler::u64> Assembler::assembleFile(const std::string& path) {

    // ───────────────────────────────────────────────────────────────────────
    // TABLA DE OPCODES - Mapeo de mnemonics a valores hexadecimales
    // ───────────────────────────────────────────────────────────────────────
    static const std::unordered_map<std::string, uint8_t> OPCODE_MAP = {
        // Operaciones aritméticas enteras (3 registros)
        {"ADD", 0x00}, {"SUB", 0x01}, {"ADC", 0x02}, {"SBC", 0x03},
        {"MUL", 0x04}, {"DIV", 0x05},

        // Operaciones lógicas (3 registros)
        {"AND", 0x06}, {"ORR", 0x07}, {"EOR", 0x08}, {"BIC", 0x09},

        // Operaciones de desplazamiento (3 registros)
        {"LSL", 0x0A}, {"LSR", 0x0B}, {"ASR", 0x0C}, {"ROR", 0x0D},

        // Operaciones con inmediato entero
        {"ADDI", 0x0E}, {"SUBI", 0x0F}, {"ADCI", 0x10}, {"SBCI", 0x11},
        {"MULI", 0x12}, {"DIVI", 0x13}, {"ANDI", 0x14}, {"ORRI", 0x15},
        {"EORI", 0x16}, {"BICI", 0x17}, {"LSLI", 0x18}, {"LSRI", 0x19},
        {"ASRI", 0x1A}, {"RORI", 0x1B},

        // Incremento/Decremento
        {"INC", 0x1C}, {"DEC", 0x1D},

        // Operaciones float (sin inmediato)
        {"FADD", 0x1E}, {"FSUB", 0x1F}, {"FMUL", 0x20}, {"FDIV", 0x21},
        {"FCOPYSIGN", 0x22},

        // Operaciones float (con inmediato)
        {"FADDI", 0x23}, {"FSUBI", 0x24}, {"FMULI", 0x25}, {"FDIVI", 0x26},
        {"FCOPYSIGNI", 0x27},

        // Operaciones float unarias
        {"FSQRT", 0x28}, {"FNEG", 0x29}, {"FABS", 0x2A},
        {"CDTI", 0x2B}, {"CDTD", 0x2C}, {"RTNR", 0x2D},
        {"RTZ", 0x2E}, {"RTP", 0x2F}, {"RTNE", 0x30},

        // MOV y variantes
        {"MOV", 0x31}, {"MVN", 0x32}, {"MOVI", 0x33}, {"MVNI", 0x34},
        {"FMOVI", 0x35}, {"FMVNI", 0x36},

        // MOVL - Carga dirección de etiqueta (usa mismo opcode que MOVI)
        {"MOVL", 0x33},

        // Comparaciones enteras (sin inmediato)
        {"CMP", 0x37}, {"CMN", 0x38}, {"TST", 0x39}, {"TEQ", 0x3A},

        // Comparaciones enteras (con inmediato)
        {"CMPI", 0x3B}, {"CMNI", 0x3C}, {"TSTI", 0x3D}, {"TEQI", 0x3E},

        // Comparaciones float (sin inmediato)
        {"FCMP", 0x3F}, {"FCMN", 0x40}, {"FCMPS", 0x41},

        // Comparaciones float (con inmediato)
        {"FCMPI", 0x42}, {"FCMNI", 0x43}, {"FCMPSI", 0x44},

        // Branches (saltos condicionales e incondicionales)
        {"B", 0x45}, {"BEQ", 0x46}, {"BNE", 0x47}, {"BLT", 0x48},
        {"BGT", 0x49}, {"BUN", 0x4A}, {"BORD", 0x4B},

        // Instrucciones especiales
        {"SWI", 0x4C},  // Software Interrupt
        {"NOP", 0x4D},  // No Operation

        // Operaciones de memoria
        {"LDR", 0x4E}, {"STR", 0x4F}, {"LDRB", 0x50}, {"STRB", 0x51},

        // Float unarias con inmediato
        {"FSQRTI", 0x52}, {"FNEGI", 0x53}, {"FABSI", 0x54},
        {"CDTII", 0x55}, {"CDTDI", 0x56}, {"RTNRI", 0x57},
        {"RTZI", 0x58}, {"RTPI", 0x59}, {"RTNEI", 0x5A}
    };

    // ───────────────────────────────────────────────────────────────────────
    // TABLA DE REGISTROS - Mapeo de nombres a códigos
    // ───────────────────────────────────────────────────────────────────────
    static const std::unordered_map<std::string, uint8_t> REGS = {
        {"REG0", 0x0}, {"REG1", 0x1}, {"REG2", 0x2}, {"REG3", 0x3},
        {"REG4", 0x4}, {"REG5", 0x5}, {"REG6", 0x6}, {"REG7", 0x7},
        {"REG8", 0x8}, {"SIDS", 0x9}, {"UPPER_REG", 0xA}, {"LOWER_REG", 0xB}
    };

    // ───────────────────────────────────────────────────────────────────────
    // APERTURA Y LECTURA DEL ARCHIVO
    // ───────────────────────────────────────────────────────────────────────
    std::ifstream ifs(path);
    if (!ifs) throw std::runtime_error("No se pudo abrir el archivo: " + path);

    // ═══════════════════════════════════════════════════════════════════════
    //                          PRIMERA PASADA
    //                   Recolección de etiquetas y instrucciones
    // ═══════════════════════════════════════════════════════════════════════

    struct LineInfo { int lineno; std::string text; };
    std::vector<LineInfo> lines;
    std::string raw;
    int lineno = 1;

    // Leer todas las líneas, eliminando comentarios
    while (std::getline(ifs, raw)) {
        // Eliminar \r si existe (compatibilidad Windows)
        if (!raw.empty() && raw.back() == '\r') {
            raw.pop_back();
        }
        // Eliminar comentarios (todo después de ';')
        size_t p = raw.find_first_of(";");
        if (p != std::string::npos) raw = raw.substr(0, p);
        lines.push_back({ lineno++, raw });
    }

    std::vector<std::tuple<int, std::string, int>> insts;  // (PC, texto, línea)
    std::unordered_map<std::string, int> labels;           // etiqueta → PC
    int pc = 0;  // Program Counter (en unidades de instrucciones)

    // Procesar líneas para extraer etiquetas e instrucciones
    for (auto& ln : lines) {
        std::string s = trim(ln.text);
        if (s.empty()) continue;

        // Procesar todas las etiquetas en la línea (puede haber múltiples)
        while (true) {
            size_t col = s.find(':');
            if (col == std::string::npos) break;
            std::string lbl = trim(s.substr(0, col));
            if (!lbl.empty()) labels[up(lbl)] = pc;
            s = trim(s.substr(col + 1));
        }

        // Si queda texto después de las etiquetas, es una instrucción
        if (!s.empty()) {
            insts.emplace_back(pc, s, ln.lineno);
            ++pc;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //                          SEGUNDA PASADA
    //                         Ensamblado de instrucciones
    // ═══════════════════════════════════════════════════════════════════════

    std::vector<u64> out;              // Instrucciones binarias resultantes
    std::vector<std::string> errors;   // Errores acumulados

    std::cout << "[Assembler] Iniciando ensamblado: " << path
        << " (" << insts.size() << " instrucciones)\n";

    for (const auto& p : insts) {
        int curpc = std::get<0>(p);
        std::string text = std::get<1>(p);
        int lineNumber = std::get<2>(p);

        try {
            // ───────────────────────────────────────────────────────────────
            // Separar opcode del resto de operandos
            // ───────────────────────────────────────────────────────────────
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
            u64 word = 0;
            word |= (u64(opcval) & 0xFFULL) << 56;  // Opcode en bits [63:56]
            bool encoded = false;

            // ═══════════════════════════════════════════════════════════════
            //              CASO ESPECIAL: INSTRUCCIONES DE MEMORIA
            //              (Procesadas ANTES de splitOperands)
            // ═══════════════════════════════════════════════════════════════
            // Formato: LDR/STR REG, [BASE] o REG, [BASE, #offset]
            // Estas usan sintaxis con corchetes que no se puede separar con comas

            if (opcval == 0x4E || opcval == 0x4F || opcval == 0x50 || opcval == 0x51) {
                size_t bracket_open = rest.find('[');
                size_t bracket_close = rest.find(']');

                if (bracket_open == std::string::npos || bracket_close == std::string::npos) {
                    throw std::runtime_error("Instruccion de memoria requiere: REG, [BASE] o REG, [BASE, #offset]");
                }

                // Extraer primer registro (antes del '[')
                std::string before_bracket = rest.substr(0, bracket_open);
                size_t comma_pos = before_bracket.find(',');
                if (comma_pos == std::string::npos) {
                    throw std::runtime_error("Falta coma antes de '['");
                }
                std::string reg_str = trim(before_bracket.substr(0, comma_pos));
                std::string reg_str_upper = up(reg_str);

                if (REGS.count(reg_str_upper) == 0) {
                    throw std::runtime_error("Registro invalido: " + reg_str);
                }
                uint8_t reg = REGS.at(reg_str_upper);

                // Extraer contenido dentro de los corchetes
                std::string inside_brackets = trim(rest.substr(bracket_open + 1, bracket_close - bracket_open - 1));

                uint8_t base_reg = 0;
                uint32_t offset_u32 = 0;

                // Verificar si hay offset: [BASE, #offset]
                size_t comma_inside = inside_brackets.find(',');
                if (comma_inside != std::string::npos) {
                    // Formato: [BASE, #offset]
                    std::string base_str = trim(inside_brackets.substr(0, comma_inside));
                    std::string offset_str = trim(inside_brackets.substr(comma_inside + 1));

                    std::string base_str_upper = up(base_str);
                    if (REGS.count(base_str_upper) == 0) {
                        throw std::runtime_error("Registro base invalido: " + base_str);
                    }
                    base_reg = REGS.at(base_str_upper);

                    if (!isImmediate(offset_str)) {
                        throw std::runtime_error("Offset debe ser inmediato: " + offset_str);
                    }
                    long offset_val = parseIntImm(offset_str);
                    offset_u32 = static_cast<uint32_t>(static_cast<int32_t>(offset_val));
                }
                else {
                    // Formato: [BASE] (sin offset)
                    std::string base_str_upper = up(inside_brackets);
                    if (REGS.count(base_str_upper) == 0) {
                        throw std::runtime_error("Registro base invalido: " + inside_brackets);
                    }
                    base_reg = REGS.at(base_str_upper);
                    offset_u32 = 0;
                }

                // Codificar según el tipo de instrucción
                if (opcval == 0x4E || opcval == 0x50) {
                    // LDR/LDRB: primer registro → Rd (bits 52-55)
                    word |= (u64(reg) & 0xFULL) << 52;
                    word |= (u64(base_reg) & 0xFULL) << 48;
                }
                else {
                    // STR/STRB: primer registro → Rm (bits 44-47)
                    word |= (u64(reg) & 0xFULL) << 44;
                    word |= (u64(base_reg) & 0xFULL) << 48;
                }
                word |= (u64(offset_u32) & 0xFFFFFFFFULL) << 12;
                encoded = true;
            }
            // ═══════════════════════════════════════════════════════════════
            //                  RESTO DE INSTRUCCIONES
            //                  (Usan splitOperands normal)
            // ═══════════════════════════════════════════════════════════════
            else {
                auto ops = splitOperands(rest);

                // ───────────────────────────────────────────────────────────
                // MOVL: Carga dirección absoluta de etiqueta (PRIORIDAD)
                // ───────────────────────────────────────────────────────────
                // Formato: MOVL REG, .LABEL
                // DEBE IR ANTES de la verificación genérica de 2 operandos
                // Carga el PC (en bytes) de la etiqueta en el registro

                if (ops.size() == 2 && !isImmediate(ops[1]) && opcU == "MOVL") {
                    std::string rd_s = up(ops[0]);
                    if (REGS.count(rd_s) == 0) {
                        throw std::runtime_error("Registro destino invalido para MOVL: " + ops[0]);
                    }

                    std::string target = up(ops[1]);
                    if (labels.count(target) == 0) {
                        throw std::runtime_error("Etiqueta no encontrada para MOVL: '" + ops[1] + "'");
                    }

                    uint8_t rd = REGS.at(rd_s);

                    // El inmediato es el PC absoluto de la etiqueta EN BYTES
                    long target_pc_instructions = labels.at(target);
                    long target_address_bytes = target_pc_instructions * 8;  // Convertir a bytes
                    uint32_t imm_u32 = static_cast<uint32_t>(static_cast<int32_t>(target_address_bytes));

                    word |= (u64(rd) & 0xFULL) << 52;                    // Rd
                    word |= (u64(rd) & 0xFULL) << 48;                    // Rn (mismo que Rd)
                    word |= (u64(imm_u32) & 0xFFFFFFFFULL) << 12;        // Inmediato (dirección)
                    encoded = true;
                }

                // ───────────────────────────────────────────────────────────
                // FORMATO R: 3 Operandos de Registro (Rd, Rn, Rm)
                // ───────────────────────────────────────────────────────────
                // Ejemplo: ADD REG1, REG2, REG3

                else if (ops.size() == 3 && !isImmediate(ops[2])) {
                    std::string rd = up(ops[0]), rn = up(ops[1]), rm = up(ops[2]);
                    if (REGS.count(rd) == 0 || REGS.count(rn) == 0 || REGS.count(rm) == 0)
                        throw std::runtime_error("Registro invalido en instruccion de 3 operandos.");

                    word |= (u64(REGS.at(rd)) & 0xFULL) << 52;  // Rd [55:52]
                    word |= (u64(REGS.at(rn)) & 0xFULL) << 48;  // Rn [51:48]
                    word |= (u64(REGS.at(rm)) & 0xFULL) << 44;  // Rm [47:44]
                    encoded = true;
                }

                // ───────────────────────────────────────────────────────────
                // FORMATO R: 2 Operandos de Registro
                // ───────────────────────────────────────────────────────────
                // Dos subcasos:
                // 1. Comparaciones: CMP/CMN/TST/TEQ/FCMP/FCMN/FCMPS → Rn, Rm
                // 2. MOV/MVN y otros: → Rd, Rm

                else if (ops.size() == 2 && !isImmediate(ops[1])) {
                    std::string arg0 = up(ops[0]), arg1 = up(ops[1]);
                    if (REGS.count(arg0) == 0 || REGS.count(arg1) == 0)
                        throw std::runtime_error("Registro invalido en instruccion de 2 operandos.");

                    // Instrucciones de comparación: primer operando → Rn, segundo → Rm
                    if (opcval == 0x37 || opcval == 0x38 || opcval == 0x39 || opcval == 0x3A ||
                        opcval == 0x3F || opcval == 0x40 || opcval == 0x41) {
                        word |= (u64(REGS.at(arg0)) & 0xFULL) << 48;  // Rn
                        word |= (u64(REGS.at(arg1)) & 0xFULL) << 44;  // Rm
                        encoded = true;
                    }
                    // Otras instrucciones: primer operando → Rd, segundo → Rm
                    else {
                        word |= (u64(REGS.at(arg0)) & 0xFULL) << 52;  // Rd
                        word |= (u64(REGS.at(arg1)) & 0xFULL) << 44;  // Rm
                        encoded = true;
                    }
                }

                // ───────────────────────────────────────────────────────────
                // FORMATO I: Instrucciones con Inmediato
                // ───────────────────────────────────────────────────────────
                // Formatos: Rd, #imm  o  Rd, Rn, #imm

                else if (!ops.empty() && isImmediate(ops.back())) {
                    if (ops.size() < 2 || ops.size() > 3)
                        throw std::runtime_error("Instruccion con inmediato requiere 2 o 3 operandos.");

                    std::string rd_s = up(ops[0]);
                    if (REGS.count(rd_s) == 0)
                        throw std::runtime_error("Registro destino invalido: " + ops[0]);

                    uint8_t rd = REGS.at(rd_s);
                    uint8_t rn = rd;  // Por defecto, Rn = Rd

                    // Si hay 3 operandos, el segundo es Rn
                    if (ops.size() == 3) {
                        std::string rn_s = up(ops[1]);
                        if (isImmediate(rn_s))
                            throw std::runtime_error("Segundo operando no puede ser inmediato.");
                        if (REGS.count(rn_s) == 0)
                            throw std::runtime_error("Registro fuente invalido: " + ops[1]);
                        rn = REGS.at(rn_s);
                    }

                    uint32_t imm_u32 = 0;
                    std::string immtok = ops.back();

                    // Determinar tipo: float (tiene '.') o entero
                    if (immtok.find('.') != std::string::npos && opcU[0] == 'F') {
                        // Float: convertir a IEEE 754 de 32 bits
                        float fval = std::stof(immtok.substr(1));
                        imm_u32 = floatToU32(fval);
                    }
                    else {
                        // Entero: mantener representación de complemento a 2
                        // Ahora soporta hexadecimal (#0x800) y decimal (#123, #-45)
                        long val = parseIntImm(immtok);
                        imm_u32 = static_cast<uint32_t>(static_cast<int32_t>(val));
                    }

                    word |= (u64(rd) & 0xFULL) << 52;                    // Rd
                    word |= (u64(rn) & 0xFULL) << 48;                    // Rn
                    word |= (u64(imm_u32) & 0xFFFFFFFFULL) << 12;        // Inmediato [43:12]
                    encoded = true;
                }

                // ───────────────────────────────────────────────────────────
                // BRANCHES: Saltos a etiquetas
                // ───────────────────────────────────────────────────────────
                // Formato: BEQ .LABEL, BLT .LOOP, etc.
                // El offset se calcula en BYTES (instrucciones × 8)

                else if (ops.size() == 1 && !isImmediate(ops[0]) && opcU[0] == 'B') {
                    std::string target = up(ops[0]);
                    if (labels.count(target) == 0) {
                        throw std::runtime_error("Etiqueta no encontrada: '" + ops[0] + "'");
                    }

                    // Calcular offset en bytes
                    long offset_instructions = (long)labels.at(target) - (long)curpc;
                    long offset_bytes = offset_instructions * 8;  // Cada instrucción = 8 bytes

                    uint32_t offset_u32 = static_cast<uint32_t>(static_cast<int32_t>(offset_bytes));
                    word |= (u64(offset_u32) & 0xFFFFFFFFULL) << 12;
                    encoded = true;
                }

                // ───────────────────────────────────────────────────────────
                // INC/DEC: Operaciones unarias de incremento/decremento
                // ───────────────────────────────────────────────────────────
                // Formato: INC REG3, DEC REG2
                // Internamente se codifican como ADD/SUB con inmediato = 1

                else if (ops.size() == 1 && !isImmediate(ops[0]) &&
                    (opcval == 0x1C || opcval == 0x1D)) {
                    std::string rd_s = up(ops[0]);
                    if (REGS.count(rd_s) == 0) {
                        throw std::runtime_error("Registro invalido para INC/DEC: " + ops[0]);
                    }

                    uint8_t rd = REGS.at(rd_s);
                    uint32_t imm_u32 = 1;  // Siempre usa inmediato = 1

                    word |= (u64(rd) & 0xFULL) << 52;                    // Rd (destino)
                    word |= (u64(rd) & 0xFULL) << 48;                    // Rn (fuente = destino)
                    word |= (u64(imm_u32) & 0xFFFFFFFFULL) << 12;        // Inmediato = 1
                    encoded = true;
                }

                // ───────────────────────────────────────────────────────────
                // SIN OPERANDOS: NOP, SWI
                // ───────────────────────────────────────────────────────────

                else if (ops.empty()) {
                    encoded = true;
                }

                // ───────────────────────────────────────────────────────────
                // ERROR: Formato no reconocido
                // ───────────────────────────────────────────────────────────

                if (!encoded) {
                    throw std::runtime_error("Formato de instruccion no valido o numero de operandos incorrecto.");
                }
            }

            // Agregar instrucción binaria al resultado
            out.push_back(word);

        }
        catch (const std::exception& e) {
            // Acumular errores para reportar al final
            std::stringstream error_ss;
            error_ss << "[Linea " << lineNumber << "] " << e.what() << " -> \"" << text << "\"";
            errors.push_back(error_ss.str());
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //                      REPORTE DE ERRORES O ÉXITO
    // ═══════════════════════════════════════════════════════════════════════

    if (!errors.empty()) {
        std::stringstream final_error;
        final_error << "El ensamblado fallo con " << errors.size() << " error(es):\n";
        for (const auto& err : errors) {
            final_error << "  * " << err << "\n";
        }
        throw std::runtime_error(final_error.str());
    }

    std::cout << "[Assembler] OK Ensamblado completado exitosamente: "
        << out.size() << " instrucciones generadas.\n";

    return out;
}