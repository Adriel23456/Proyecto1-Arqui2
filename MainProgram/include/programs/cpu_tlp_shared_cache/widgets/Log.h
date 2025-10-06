#pragma once
#include <mutex>
#include <sstream>
#include <iostream>

inline std::mutex& log_mutex() {
    static std::mutex m;
    return m;
}

inline void log_line(const std::string& s) {
    std::lock_guard<std::mutex> lk(log_mutex());
    std::cout << s << std::flush;
}

// Helper cómodo para armar + emitir en una sola llamada
template <typename Fn>
inline void log_build_and_print(Fn&& builder) {
    std::ostringstream oss;
    builder(oss);
    log_line(oss.str());
}