#include "test/stubs/SystemStub.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/TimeStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/MqttStub.h"
#include "test/stubs/EthernetManagerStub.h"
#include "test/stubs/HttpServerStub.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"

System& getSystem() noexcept {
    static GpioStub gpio;
    static AdcOneshotStub adcOneshot(ADC_UNIT_1);
    static RandomStub random;
    static TimeStub time;
    static NVSStub nvs;
    nvs.begin("system");
    static GpioPinRegister gpioPinRegister;
    static SettingsManager settings(nvs);

    // resolved here since mqtt/ethernetMan take their indicator pin as a ctor param
    const bool connLedsEnabled = settings.retrieveConnLedsEnabled().value_or(true);
    const gpio_num_t mqttLedPin = connLedsEnabled ? settings.retrieveMqttLedGpioPin().value_or(GPIO_NUM_NC) : GPIO_NUM_NC;
    const gpio_num_t ethernetLedPin = connLedsEnabled ? settings.retrieveEthernetLedGpioPin().value_or(GPIO_NUM_NC) : GPIO_NUM_NC;

    static MqttStub mqtt(mqttLedPin, gpio, gpioPinRegister, time);
    static EthernetManagerStub ethernetMan(ethernetLedPin, gpio, gpioPinRegister, time);
    static HttpServerStub httpServer;
    static StateMachine stateMachine;
    static System system(
        stateMachine, gpioPinRegister, gpio, adcOneshot, random, time, nvs, settings, mqtt, ethernetMan, httpServer
    );
    return system;
}
