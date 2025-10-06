#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/RegTable.h"
#include <cstdint>
#include <string>

class CpuTLPControlAPI;
class CpuTLPSharedCacheState; // <<<< AGREGAR

class PE2RegView : public ICpuTLPView {
public:
    PE2RegView() : m_table(2) {}
    void render() override { m_table.render("##PE2_RegTable"); }

private:
    friend class CpuTLPControlAPI;
    friend class CpuTLPSharedCacheState; // <<<< AGREGAR

    void setRegValueByIndex_(int idx, uint64_t v) { m_table.setValueByIndex_(idx, v); }
    void setRegValueByName_(const std::string& name, uint64_t v) { m_table.setValueByName_(name, v); }
    void setPEID_(int peIndex) { m_table.setPEID_(peIndex); }

    RegTable m_table;
};