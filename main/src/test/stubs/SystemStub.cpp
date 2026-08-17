#include "test/stubs/SystemStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/TimeStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/MqttStub.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"

System& getSystem() noexcept {
    static GpioStub gpio;
    static TimeStub time;
    static NVSStub nvs;
    nvs.begin("system");
    static GpioPinRegister gpioPinRegister;
    static SettingsManager settings(nvs);

    // resolved here since mqtt takes its indicator pin as a ctor param
    const gpio_num_t mqttLedPin = settings.retrieveMqttLedGpioPin().value_or(GPIO_NUM_NC);

    static MqttStub mqtt(mqttLedPin, gpio, gpioPinRegister, time);
    static StateMachine stateMachine;
    static System system(
        stateMachine, gpioPinRegister, gpio, time, nvs, settings, mqtt
    );
    return system;
}
