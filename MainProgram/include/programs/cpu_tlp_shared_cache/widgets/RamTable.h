#pragma once
#include <array>
#include <string>
#include <cstdint>

class RamTable {
public:
    static constexpr int   kRows = 128;
    static constexpr size_t kBytesPerRow = 32;           // 32B
    static constexpr uint64_t kBaseAddr = 0x0000000000000000ull;
    static constexpr uint64_t kStep = 0x20ull;       // 32 bytes (hex 0x20)
    static constexpr uint64_t kMaxAddr = kBaseAddr + kRows * kStep - kStep; // última dirección mostrada

    // Renderiza la tabla ocupando todo el tamaño disponible.
    void render(const char* id);

    // Set de datos por dirección (debe estar alineada a 32B y dentro [base, base+0x1000) )
    // 'hex' debe representar 32 bytes => 64 dígitos hex (con o sin "0x").
    void setDataByAddress(uint64_t address, const std::string& hex);

    // Opción por índice de fila [0..127]
    void setDataByIndex(int rowIndex, const std::string& hex);

private:
    // Guardamos el texto tal cual (normalizado) para cada fila
    std::array<std::string, kRows> m_dataHex{};

    // Helpers
    static bool sanitizeHex32B(const std::string& in, std::array<uint8_t, kBytesPerRow>& outBytes, std::string* normalizedHexOut);
    static std::string addrString(uint64_t addr64); // "0x" + 16 hex
};