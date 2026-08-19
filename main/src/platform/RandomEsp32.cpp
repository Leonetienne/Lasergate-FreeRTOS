#include "platform/RandomEsp32.h"
#include "esp_random.h"

int32_t RandomEsp32::getNextInt() noexcept {
    return static_cast<int32_t>(esp_random());
}
