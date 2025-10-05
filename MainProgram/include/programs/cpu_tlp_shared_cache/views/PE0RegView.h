#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/RegTable.h"
#include <cstdint>
#include <string>

class PE0RegView : public ICpuTLPView {
public:
    PE0RegView() : m_table(0) {}
    void render() override { m_table.render("##PE0_RegTable"); }

    // setters programáticos (64 bits)
    void setRegValueByIndex(int idx, uint64_t v) { m_table.setValueByIndex(idx, v); }
    void setRegValueByName(const std::string& name, uint64_t v) { m_table.setValueByName(name, v); }

private:
    RegTable m_table;
};
