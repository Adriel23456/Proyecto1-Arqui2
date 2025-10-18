#pragma once
#include <array>
#include <string>
#include <cstdint>

class RamTable {
public:
    static constexpr int   kRows = 512;                  // 512 posiciones de 64 bits
    static constexpr size_t kBytesPerRow = 8;            // 8 bytes (64 bits)
    static constexpr uint64_t kBaseAddr = 0x0000000000000000ull;
    static constexpr uint64_t kStep = 0x8ull;            // incremento de 8 bytes (64 bits)
    static constexpr uint64_t kMaxAddr = kBaseAddr + (kRows - 1) * kStep; // 0x00000000000001FF8

    void render(const char* id);

    // Set de datos por dirección (debe estar alineada a 8 bytes)
    void setDataByAddress(uint64_t address, uint64_t value);
    void setDataByIndex(int rowIndex, uint64_t value);

private:
    std::array<uint64_t, kRows> m_values{};

    static std::string addrString(uint64_t addr64);
    static std::string toHex_(uint64_t v);
};