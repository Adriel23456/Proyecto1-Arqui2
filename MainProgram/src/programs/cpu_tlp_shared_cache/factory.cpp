#include "programs/cpu_tlp_shared_cache/CpuTLPSharedCacheState.h"
#include "core/State.h"
#include <memory>

std::unique_ptr<State> CreateCpuTLPSharedCacheState(StateManager* sm, sf::RenderWindow* win) {
    return std::make_unique<CpuTLPSharedCacheState>(sm, win);
}

