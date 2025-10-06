#pragma once
#include <functional>

namespace cpu_tlp_ui {
    // PE0 callbacks
    inline std::function<void()>        onResetPE0;
    inline std::function<void()>        onStepPE0;
    inline std::function<void(int)>     onStepUntilPE0;
    inline std::function<void()>        onStepIndefinitelyPE0;
    inline std::function<void()>        onStopPE0;

    // PE1 callbacks
    inline std::function<void()>        onResetPE1;
    inline std::function<void()>        onStepPE1;
    inline std::function<void(int)>     onStepUntilPE1;
    inline std::function<void()>        onStepIndefinitelyPE1;
    inline std::function<void()>        onStopPE1;

    // PE2 callbacks
    inline std::function<void()>        onResetPE2;
    inline std::function<void()>        onStepPE2;
    inline std::function<void(int)>     onStepUntilPE2;
    inline std::function<void()>        onStepIndefinitelyPE2;
    inline std::function<void()>        onStopPE2;

    // PE3 callbacks
    inline std::function<void()>        onResetPE3;
    inline std::function<void()>        onStepPE3;
    inline std::function<void(int)>     onStepUntilPE3;
    inline std::function<void()>        onStepIndefinitelyPE3;
    inline std::function<void()>        onStopPE3;
}