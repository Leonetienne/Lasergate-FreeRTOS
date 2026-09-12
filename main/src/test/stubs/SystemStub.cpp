#include "test/stubs/SystemStub.h"

SystemStub::SystemStub(gpio_num_t mqttLedPin, gpio_num_t ethernetLedPin) noexcept :
    adcOneshot(ADC_UNIT_1),
    adcOneshot2(ADC_UNIT_2),
    settings(nvs),
    mqtt(mqttLedPin, gpio, gpioPinRegister, time),
    ethernetMan(ethernetLedPin, gpio, gpioPinRegister, time)
{
    nvs.begin("test");
}

System& SystemStub::buildSystem() noexcept {
    system.emplace(
        stateMachine, gpioPinRegister, gpio, adcOneshot, adcOneshot2, random, time, nvs, settings, mqtt, ethernetMan,
        httpServer, apiController
    );
    return *system;
}
