#ifndef LASERGATE_V2_TIMEESP32_H
#define LASERGATE_V2_TIMEESP32_H

#include "hal/ITime.h"

/**
 * Esp32-Implementation for time
 */
class TimeEsp32 : public ITime {
public:
    TimeEsp32() = default;
    TimeEsp32(const TimeEsp32&) = default;
    TimeEsp32(TimeEsp32&&) = default;

    /**
     * @return Current calendar time in seconds since epoch
     */
    [[nodiscard]] time_t getTime() const noexcept override;

    /**
    * @param since The time reference to measure the time distance towards
    * @return How many seconds have elapsed since reference
    */
    [[nodiscard]] int getSecondsSince(const time_t &since) const noexcept override;

    /**
     * @return A monotonic millisecond timestamp
     */
    [[nodiscard]] int64_t getMillis() const noexcept override;
};

#endif //LASERGATE_V2_TIMEESP32_H
