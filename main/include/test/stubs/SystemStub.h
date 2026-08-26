#ifndef LASERGATE_V2_SYSTEMSTUB_H
#define LASERGATE_V2_SYSTEMSTUB_H

#include "GpioPinRegister.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "System.h"
#include "compat/gpio_num_t.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/EthernetManagerStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/HttpServerStub.h"
#include "test/stubs/MqttStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/TimeStub.h"
#include <optional>

/**
 * Wires a System against host-side test stubs.
 */
class SystemStub {
public:
    explicit SystemStub(gpio_num_t mqttLedPin = GPIO_NUM_NC, gpio_num_t ethernetLedPin = GPIO_NUM_NC) noexcept;
    SystemStub(const SystemStub&) = delete;
    SystemStub(SystemStub&&) = delete;

    /**
     * Builds the System against the settings as they currently stand. Call once, after
     * configuring settings.
     */
    System& buildSystem() noexcept;

    GpioPinRegister gpioPinRegister;
    GpioStub gpio;
    AdcOneshotStub adcOneshot;
    RandomStub random;
    TimeStub time;
    NVSStub nvs;
    SettingsManager settings;
    MqttStub mqtt;
    EthernetManagerStub ethernetMan;
    HttpServerStub httpServer;
    StateMachine stateMachine;

private:
    std::optional<System> system;
};

#endif //LASERGATE_V2_SYSTEMSTUB_H
