#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/RegTable.h"
#include <cstdint>
#include <string>

class PE2RegView : public ICpuTLPView {
public:
    PE2RegView() : m_table(2) {}
    void render() override { m_table.render("##PE2_RegTable"); }

    void setRegValueByIndex(int idx, uint64_t v) { m_table.setValueByIndex(idx, v); }
    void setRegValueByName(const std::string& name, uint64_t v) { m_table.setValueByName(name, v); }

private:
    RegTable m_table;
};
