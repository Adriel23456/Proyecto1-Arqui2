#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/CacheMemTable.h"
#include <array>
#include <cstdint>
#include <string>

class PE2MemView : public ICpuTLPView {
public:
    void render() override;

    // FIRMA CORRECTA: recibe std::array directamente
    void setBySetWay(int setIndex, int wayIndex,
        const std::string& tag,
        const std::array<uint8_t, 32>& data,  // Array directamente
        const std::string& mesi);

private:
    CacheMemTable m_table;
};