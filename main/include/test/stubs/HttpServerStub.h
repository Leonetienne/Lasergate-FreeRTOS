#ifndef LASERGATE_V2_HTTPSERVERSTUB_H
#define LASERGATE_V2_HTTPSERVERSTUB_H

#include "hal/IHttpServer.h"

class HttpServerStub : public IHttpServer {
public:
    HttpServerStub() = default;
    HttpServerStub(const HttpServerStub&) = delete;
    HttpServerStub& operator=(const HttpServerStub&) = delete;
    HttpServerStub(HttpServerStub&&) noexcept;
    ~HttpServerStub() override = default;

    /**
     * Will report itself as running, without starting an actual server
     */
    bool begin() noexcept override;

    /**
     * Will report itself as stopped
     */
    bool free() noexcept override;

    /* Unit test interrogators */
    [[nodiscard]] bool test_isRunning() const noexcept;
    [[nodiscard]] int test_getBeginCallCount() const noexcept;
    [[nodiscard]] int test_getFreeCallCount() const noexcept;

private:
    bool running = false;
    int beginCallCount = 0;
    int freeCallCount = 0;
};

#endif //LASERGATE_V2_HTTPSERVERSTUB_H
