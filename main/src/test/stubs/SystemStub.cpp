#include "test/stubs/SystemStub.h"

SystemStub::SystemStub(gpio_num_t mqttLedPin, gpio_num_t ethernetLedPin) noexcept :
    adcOneshot(ADC_UNIT_1),
    settings(nvs),
    mqtt(mqttLedPin, gpio, gpioPinRegister, time),
    ethernetMan(ethernetLedPin, gpio, gpioPinRegister, time)
{
    nvs.begin("test");
}

System& SystemStub::buildSystem() noexcept {
    system.emplace(
        stateMachine, gpioPinRegister, gpio, adcOneshot, random, time, nvs, settings, mqtt, ethernetMan, httpServer
    );
    return *system;
}
