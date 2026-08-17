#ifndef LASERGATE_V2_ITIME_H
#define LASERGATE_V2_ITIME_H

#include "compat/time_t.h"
#include <cstdint>

/**
 * Abstract time interface
 */
class ITime {
public:
    virtual ~ITime() = default;

    /**
     * @return Current calendar time in seconds since epoch
     */
    [[nodiscard]] virtual time_t getTime() const noexcept = 0;

    /**
     * @param since The time reference to measure the time distance towards
     * @return How many seconds have elapsed since reference
     */
    [[nodiscard]] virtual int getSecondsSince(const time_t& since) const noexcept = 0;

    /**
     * @return A monotonic millisecond timestamp
     */
    [[nodiscard]] virtual int64_t getMillis() const noexcept = 0;
};

#endif //LASERGATE_V2_ITIME_H
