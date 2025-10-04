#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include <cstdint>

class AnalysisDataView : public ICpuTLPView {
public:
    void render() override;

    // --- Setters ---
    void setCacheMisses(uint64_t v) { m_cacheMisses = v; }
    void setInvalidations(uint64_t v) { m_invalidations = v; }
    void setReadWriteOps(uint64_t v) { m_readWriteOps = v; }
    void setTransactionsMESI(uint64_t v) { m_transactionsMESI = v; }
    void setTrafficPE0(uint64_t v) { m_trafficPE[0] = v; }
    void setTrafficPE1(uint64_t v) { m_trafficPE[1] = v; }
    void setTrafficPE2(uint64_t v) { m_trafficPE[2] = v; }
    void setTrafficPE3(uint64_t v) { m_trafficPE[3] = v; }
    void setTrafficPE(size_t peIndex, uint64_t v) { if (peIndex < 4) m_trafficPE[peIndex] = v; }

    // Opcional: reseteo rápido
    void reset() {
        m_cacheMisses = m_invalidations = m_readWriteOps = m_transactionsMESI = 0;
        for (auto& t : m_trafficPE) t = 0;
    }

private:
    uint64_t m_cacheMisses = 0;
    uint64_t m_invalidations = 0;
    uint64_t m_readWriteOps = 0;
    uint64_t m_transactionsMESI = 0;
    uint64_t m_trafficPE[4] = { 0,0,0,0 };
};
