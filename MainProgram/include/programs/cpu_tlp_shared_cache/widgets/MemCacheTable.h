#pragma once
#include <array>
#include <string>
#include <imgui.h>

// Tabla genérica 4x16 con encabezados de set cada 2 filas.
// - Columnas: Tag | Data | State | MESI
// - MESI se deriva de State ("" -> "Null", M/E/S/I por primera letra, case-insensitive).
class MemCacheTable {
public:
    static constexpr int kSets = 8;
    static constexpr int kWaysPerSet = 2;
    static constexpr int kRows = kSets * kWaysPerSet;

    struct Row {
        std::string tag;
        std::string data;
        std::string state;
    };

    // Renderiza ocupando el área disponible (con scroll vertical si hace falta).
    void render(const char* id);

    // Setters por fila
    void setRow(int row, const std::string& tag, const std::string& data, const std::string& state);
    void setTag(int row, const std::string& tag);
    void setData(int row, const std::string& data);
    void setState(int row, const std::string& state);

    // Setters por (set, way) — útil si manejas mapeo 2-way
    void setBySetWay(int setIndex, int wayIndex, const std::string& tag, const std::string& data, const std::string& state);
    void setTagBySetWay(int setIndex, int wayIndex, const std::string& tag);
    void setDataBySetWay(int setIndex, int wayIndex, const std::string& data);
    void setStateBySetWay(int setIndex, int wayIndex, const std::string& state);

    const Row& getRow(int row) const { return m_rows[rowClamp(row)]; }

private:
    std::array<Row, kRows> m_rows{}; // inicializado vacío

    static int rowClamp(int r) {
        if (r < 0) return 0;
        if (r >= kRows) return kRows - 1;
        return r;
    }
    static int idx(int setIndex, int wayIndex) {
        if (setIndex < 0) setIndex = 0; if (setIndex >= kSets) setIndex = kSets - 1;
        if (wayIndex < 0) wayIndex = 0; if (wayIndex >= kWaysPerSet) wayIndex = kWaysPerSet - 1;
        return setIndex * kWaysPerSet + wayIndex;
    }
    static const char* deriveMESI(const std::string& state);
};
