#pragma once
#include <array>
#include <cstdint>
#include <string>

// Forward para controlar quién puede escribir
class CpuTLPControlAPI;
class PE0RegView;
class PE1RegView;
class PE2RegView;
class PE3RegView;

class RegTable {
public:
    static constexpr int kRegCount = 12;

    enum RegIndex : int {
        REG0 = 0, REG1, REG2, REG3, REG4, REG5, REG6, REG7, REG8,
        PEID,
        UPPER_REG,
        LOWER_REG
    };

    explicit RegTable(int peIndex = 0);

    // Solo lectura desde fuera
    uint64_t getValueByIndex(int idx) const;
    const std::string& getHexTextByIndex(int idx) const;

    // Render: AHORA SOLO LECTURA EN LA COLUMNA "Value"
    void render(const char* id);

private:
    // --- CONTROL DE ESCRITURA ---
    // Setters PRIVADOS: solo amigos pueden llamar
    friend class CpuTLPControlAPI;
    friend class PE0RegView;
    friend class PE1RegView;
    friend class PE2RegView;
    friend class PE3RegView;

    void setPEID_(int peIndex);
    void setValueByIndex_(int idx, uint64_t val);
    void setValueByName_(const std::string& key, uint64_t val);

    // --- DATOS ---
    std::array<std::string, kRegCount> m_names{};
    std::array<uint64_t, kRegCount>    m_values{};
    std::array<std::string, kRegCount> m_valueText{}; // "0x%016llX"

    // --- HELPERS (privados) ---
    static std::string toHex_(uint64_t v);
    static int indexFromName_(const std::string& keyIn);
};
