#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/RegTable.h"
#include <cstdint>
#include <string>

class CpuTLPControlAPI;

class PE1RegView : public ICpuTLPView {
public:
    PE1RegView() : m_table(1) {}
    void render() override { m_table.render("##PE1_RegTable"); }

private:
    friend class CpuTLPControlAPI;
    void setRegValueByIndex_(int idx, uint64_t v) { m_table.setValueByIndex_(idx, v); }
    void setRegValueByName_(const std::string& name, uint64_t v) { m_table.setValueByName_(name, v); }
    void setPEID_(int peIndex) { m_table.setPEID_(peIndex); }

    RegTable m_table;
};
