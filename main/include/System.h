//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_SYSTEM_H
#define LASERGATE_V2_SYSTEM_H

#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"
#include "hal/IGpio.h"
#include "hal/ITime.h"
#include "hal/INVS.h"
#include "hal/IMqtt.h"
#include "platform/GpioDigitalWritePin.h"
#include <string>

/**
 * System entrypoint and main runtime.
 *
 * Depends only on interfaces (plus the portable, non-hardware logic classes),
 * so it can be run against either the esp32 platform implementations.
 */
class System {
public:
    System(
        StateMachine& stateMachine,
        GpioPinRegister& gpioPinRegister,
        IGpio& gpio,
        ITime& i_time,
        INVS& nvs,
        SettingsManager& settings,
        IMqtt& mqtt
    ) noexcept;
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System(System&&) = delete;
    ~System() noexcept;

    void init() noexcept;

    /**
     * Runs the runtime loop until the state machine enters STATE::SHUTTING_DOWN.
     */
    void loop() noexcept;

    /**
     * Will free acquired resources
     * @return Success state
     */
    bool free() noexcept;

    /**
     * Processes one iteration of runtime work (test-pin toggle, mqtt activity
     * LED pulse). Called repeatedly by loop().
     */
    void update() noexcept;

private:
    void beforeShutdown() noexcept;

    /**
     * Called by mqtt once a connection to the broker is established.
     */
    void onMqttConnected() noexcept;

    /**
     * Called by mqtt once a previously established connection is lost.
     */
    void onMqttDisconnected() noexcept;

    /**
     * Called by mqtt for every message received on a subscribed topic.
     */
    void onMqttMessage(const std::string& topic, const std::string& payload) noexcept;

    static constexpr gpio_num_t LASER_TEST_PIN = GPIO_NUM_39;
    static constexpr int64_t LASER_TEST_TOGGLE_INTERVAL_MS = 1000;
    int64_t lastToggleMillis = 0;

    bool isInitialized = false;
    StateMachine& stateMachine;
    GpioPinRegister& gpioPinRegister;
    IGpio& gpio;
    ITime& i_time;
    INVS& nvs;
    SettingsManager& settings;
    IMqtt& mqtt;

    GpioDigitalWritePin laserTestPin;
    std::string lwtTopic;
};

#endif //LASERGATE_V2_SYSTEM_H
