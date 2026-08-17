#include "platform/SystemEsp32.h"
#include "platform/GpioEsp32.h"
#include "platform/TimeEsp32.h"
#include "platform/NVSEsp32.h"
#include "platform/MqttEsp32.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"

System& getSystem() noexcept {
    static GpioEsp32 gpio;
    static TimeEsp32 time;
    static NVSEsp32 nvs;
    nvs.begin("system");
    static GpioPinRegister gpioPinRegister;
    static SettingsManager settings(nvs);

    // resolved here since mqtt takes its indicator pin as a ctor param
    const gpio_num_t mqttLedPin = settings.retrieveMqttLedGpioPin().value_or(GPIO_NUM_NC);

    static MqttEsp32 mqtt(mqttLedPin, gpio, gpioPinRegister, time);
    static StateMachine stateMachine;
    static System system(
        stateMachine, gpioPinRegister, gpio, time, nvs, settings, mqtt
    );
    return system;
}
