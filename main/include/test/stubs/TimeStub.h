#ifndef LASERGATE_V2_TIMESTUB_H
#define LASERGATE_V2_TIMESTUB_H

#include "hal/ITime.h"

class TimeStub : public ITime {
public:
    TimeStub() = default;
    TimeStub(const TimeStub&) = default;
    TimeStub(TimeStub&&) = default;

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

    /**
     * Testing stub method: set the current time reported to callers
     */
    void setStubbedTime(time_t stubbedTime) noexcept;

    /**
     * Testing stub method: set the millisecond timestamp reported to callers
     */
    void setStubbedMillis(int64_t stubbedMillis) noexcept;

private:
    time_t stubbedCurrentTime = 1700000000; // Tu 14. Nov 23:13:20 CET 2023
    int64_t stubbedCurrentMillis = 0;
};

#endif //LASERGATE_V2_TIMESTUB_H
