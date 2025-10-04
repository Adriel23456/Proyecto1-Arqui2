#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"

class RAMView : public ICpuTLPView {
public:
    void render() override;
};
