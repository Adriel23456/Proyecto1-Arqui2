#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <cstdint>
#include <algorithm>
#include <stdexcept>

class Assembler {
public:
    using u64 = uint64_t;

    /**
     * Ensambla el archivo en la ruta dada.
     * @param path Ruta del archivo de ensamblaje.
     * @return Vector de instrucciones (uint64_t) ensambladas.
     */
    std::vector<u64> assembleFile(const std::string& path);

private:
    // Utilidades
    std::string up(const std::string& s);
    std::string trim(const std::string& s);

    // Función de split robusta
    std::vector<std::string> splitOperands(const std::string& s);

    bool isImmediate(const std::string& t);
    long parseIntImm(const std::string& t);
    uint32_t floatToU32(float f);
};