#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/RamTable.h"

class RAMView : public ICpuTLPView {
public:
    void render() override;

    // API: setear 32B en una dirección
    void setData(uint64_t address, const std::string& hex32bytes) { m_table.setDataByAddress(address, hex32bytes); }
    // Alternativa por índice [0..127]
    void setDataByIndex(int rowIndex, const std::string& hex32bytes) { m_table.setDataByIndex(rowIndex, hex32bytes); }

private:
    RamTable m_table;
};