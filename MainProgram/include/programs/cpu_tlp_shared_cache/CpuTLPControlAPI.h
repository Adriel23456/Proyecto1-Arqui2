#pragma once
#include <functional>

namespace cpu_tlp_ui {
	inline std::function<void()>        onResetPE0;
	inline std::function<void()>        onStepPE0;
	inline std::function<void(int)>     onStepUntilPE0;
	inline std::function<void()>        onStepIndefinitelyPE0;
	inline std::function<void()>        onStopPE0;
}
