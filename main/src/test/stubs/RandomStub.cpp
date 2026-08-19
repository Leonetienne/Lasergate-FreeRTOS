#include "test/stubs/RandomStub.h"

RandomStub::RandomStub() noexcept :
    engine { std::random_device{}() }
{
}

int32_t RandomStub::getNextInt() noexcept {
    return static_cast<int32_t>(engine());
}

void RandomStub::test_setSeed(uint32_t seed) noexcept {
    engine.seed(seed);
}
