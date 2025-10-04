#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"

class PE3MemView : public ICpuTLPView {
public:
    void render() override;
};
