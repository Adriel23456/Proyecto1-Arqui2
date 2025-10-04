#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/MemCacheTable.h"

// Vista PE1 Mem: tabla 4x16 con headers de set cada 2 filas.
// UI solo lectura; datos se actualizan vía setters.
class PE1MemView : public ICpuTLPView {
public:
    void render() override;

    // --- API específica de PE1 para modificar datos ---
    // Por fila absoluta 0..15
    void setRow(int row, const std::string& tag, const std::string& data, const std::string& state);
    void setTag(int row, const std::string& tag);
    void setData(int row, const std::string& data);
    void setState(int row, const std::string& state);

    // Por (set, way) — útil si manejas mapeo 2-way
    void setBySetWay(int setIndex, int wayIndex, const std::string& tag, const std::string& data, const std::string& state);
    void setTagBySetWay(int setIndex, int wayIndex, const std::string& tag);
    void setDataBySetWay(int setIndex, int wayIndex, const std::string& data);
    void setStateBySetWay(int setIndex, int wayIndex, const std::string& state);

private:
    MemCacheTable m_table;
};
