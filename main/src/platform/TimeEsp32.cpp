#include "../../include/platform/TimeEsp32.h"
#include <chrono>

time_t TimeEsp32::getTime() const noexcept {
    return time(nullptr);
}

int TimeEsp32::getSecondsSince(const time_t& since) const noexcept {
    return static_cast<int>(difftime(getTime(), since));
}

int64_t TimeEsp32::getMillis() const noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
