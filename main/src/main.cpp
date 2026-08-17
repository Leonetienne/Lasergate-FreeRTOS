#include "platform/SystemEsp32.h"
#include "esp_system.h"

extern "C" void app_main() {
    System& system = getSystem();
    system.init();
    system.loop();
    system.free();

    esp_restart();
}
