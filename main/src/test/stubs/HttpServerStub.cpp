#include "test/stubs/HttpServerStub.h"

HttpServerStub::HttpServerStub(HttpServerStub&& other) noexcept :
    running(other.running),
    beginCallCount(other.beginCallCount),
    freeCallCount(other.freeCallCount)
{
    other.running = false;
}

bool HttpServerStub::begin() noexcept {
    ++beginCallCount;
    if (running) {
        return false;
    }

    running = true;
    return true;
}

bool HttpServerStub::free() noexcept {
    ++freeCallCount;
    if (!running) {
        return false;
    }

    running = false;
    return true;
}

bool HttpServerStub::test_isRunning() const noexcept {
    return running;
}

int HttpServerStub::test_getBeginCallCount() const noexcept {
    return beginCallCount;
}

int HttpServerStub::test_getFreeCallCount() const noexcept {
    return freeCallCount;
}
