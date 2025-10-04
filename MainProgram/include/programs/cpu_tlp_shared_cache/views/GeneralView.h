#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"

class GeneralView : public ICpuTLPView {
public:
    void render() override;
};
