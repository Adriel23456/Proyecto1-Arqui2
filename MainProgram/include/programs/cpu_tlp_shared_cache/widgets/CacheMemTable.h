#pragma once
#include <array>
#include <string>
#include <cstdint>

class CacheMemTable {
public:
    struct CacheLine {
        std::string tag;
        std::array<uint8_t, 32> data;
        std::string mesi;

        CacheLine() : tag("0x00000000000000"), mesi("I") {
            data.fill(0);
        }
    };

    CacheMemTable() : m_selectedSet(0), m_selectedWay(0) {
        m_lines.fill(CacheLine());
    }

    void setLine(int index, const std::string& tag, const std::array<uint8_t, 32>& data, const std::string& mesi);
    void setLineBySetWay(int set, int way, const std::string& tag, const std::array<uint8_t, 32>& data, const std::string& mesi);

    // Renderizar la tabla con navegación
    void render(const char* id);

    // Getters para navegación
    int getSelectedSet() const { return m_selectedSet; }
    int getSelectedWay() const { return m_selectedWay; }

    // Setters para navegación programática
    void setSelectedSet(int set) { if (set >= 0 && set < kSets) m_selectedSet = set; }
    void setSelectedWay(int way) { if (way >= 0 && way < kWays) m_selectedWay = way; }

private:
    static constexpr int kSets = 8;
    static constexpr int kWays = 2;
    static constexpr int kTotalLines = kSets * kWays;

    std::array<CacheLine, kTotalLines> m_lines;

    // Estado de navegación
    int m_selectedSet;
    int m_selectedWay;

    // Helpers
    std::string dataToHex(const std::array<uint8_t, 32>& data);
    uint64_t extract8Bytes(const std::array<uint8_t, 32>& data, int segment);
    std::string format8BytesAsHex(const std::array<uint8_t, 32>& data, int segment);

    // Obtener la línea actual basada en set/way seleccionados
    const CacheLine& getCurrentLine() const {
        int index = m_selectedSet * kWays + m_selectedWay;
        return m_lines[index];
    }
};