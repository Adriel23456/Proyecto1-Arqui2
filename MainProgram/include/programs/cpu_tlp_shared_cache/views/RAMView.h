#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/RamTable.h"

// Forward declaration
namespace cpu_tlp { class SharedMemoryComponent; }

class RAMView : public ICpuTLPView {
public:
    RAMView() : m_sharedMemoryComponent(nullptr) {}

    void setSharedMemoryComponent(cpu_tlp::SharedMemoryComponent* component) {
        m_sharedMemoryComponent = component;
    }

    void render() override;

private:
    RamTable m_table;
    cpu_tlp::SharedMemoryComponent* m_sharedMemoryComponent;
};