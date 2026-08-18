#include "platform/SystemEsp32.h"
#include "platform/GpioEsp32.h"
#include "platform/TimeEsp32.h"
#include "platform/NVSEsp32.h"
#include "platform/MqttEsp32.h"
#include "platform/EthernetManagerEsp32.h"
#include "platform/HttpServerEsp32.h"
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

    // resolved here since mqtt/ethernetMan take their indicator pin as a ctor param
    const bool connLedsEnabled = settings.retrieveConnLedsEnabled().value_or(true);
    const gpio_num_t mqttLedPin = connLedsEnabled ? settings.retrieveMqttLedGpioPin().value_or(GPIO_NUM_NC) : GPIO_NUM_NC;
    const gpio_num_t ethernetLedPin = connLedsEnabled ? settings.retrieveEthernetLedGpioPin().value_or(GPIO_NUM_NC) : GPIO_NUM_NC;

    static MqttEsp32 mqtt(mqttLedPin, gpio, gpioPinRegister, time);
    static EthernetManagerEsp32 ethernetMan(ethernetLedPin, gpio, gpioPinRegister, time);
    static StateMachine stateMachine;
    static HttpServerEsp32 httpServer(ethernetMan, mqtt, settings, stateMachine);
    static System system(
        stateMachine, gpioPinRegister, gpio, time, nvs, settings, mqtt, ethernetMan, httpServer
    );
    return system;
}
