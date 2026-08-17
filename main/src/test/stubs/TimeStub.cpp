#include "../../../include/test/stubs/TimeStub.h"

time_t TimeStub::getTime() const noexcept {
    return this->stubbedCurrentTime;
}

int TimeStub::getSecondsSince(const time_t& since) const noexcept {
    return static_cast<int>(stubbedCurrentTime - since);
}

void TimeStub::setStubbedTime(time_t stubbedTime) noexcept {
    this->stubbedCurrentTime = stubbedTime;
}

int64_t TimeStub::getMillis() const noexcept {
    return this->stubbedCurrentMillis;
}

void TimeStub::setStubbedMillis(int64_t stubbedMillis) noexcept {
    this->stubbedCurrentMillis = stubbedMillis;
}
