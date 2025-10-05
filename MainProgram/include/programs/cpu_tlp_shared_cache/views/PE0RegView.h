#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/RegTable.h"
#include <cstdint>
#include <string>

class CpuTLPControlAPI; // API central autorizado

class PE0RegView : public ICpuTLPView {
public:
    PE0RegView() : m_table(0) {}
    void render() override { m_table.render("##PE0_RegTable"); }

private:
    // Solo el API puede invocar estos setters
    friend class CpuTLPControlAPI;

    // Wrappers privados hacia los setters privados del RegTable
    void setRegValueByIndex_(int idx, uint64_t v) { m_table.setValueByIndex_(idx, v); }
    void setRegValueByName_(const std::string& name, uint64_t v) { m_table.setValueByName_(name, v); }
    void setPEID_(int peIndex) { m_table.setPEID_(peIndex); }

    RegTable m_table;
};
