//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_SYSTEM_H
#define LASERGATE_V2_SYSTEM_H

#include "Gate.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"
#include "hal/IAdcOneshot.h"
#include "hal/IGpio.h"
#include "hal/IRandom.h"
#include "hal/ITime.h"
#include "hal/INVS.h"
#include "hal/IMqtt.h"
#include "hal/IEthernetManager.h"
#include "hal/IHttpServer.h"
#include <string>

/**
 * System entrypoint and main runtime.
 */
class System {
public:
    System(
        StateMachine& stateMachine,
        GpioPinRegister& gpioPinRegister,
        IGpio& i_gpio,
        IAdcOneshot& i_adcOneshot,
        IRandom& i_random,
        ITime& i_time,
        INVS& i_nvs,
        SettingsManager& settings,
        IMqtt& i_mqtt,
        IEthernetManager& i_ethernetMan,
        IHttpServer& i_httpServer
    ) noexcept;
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System(System&&) = delete;
    ~System() noexcept;

    void initialize() noexcept;

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
     * Processes one iteration of runtime work (mqtt activity LED pulse).
     * Called repeatedly by loop().
     */
    void update() noexcept;

private:
    void beforeShutdown() noexcept;

    /**
     * Wires up the mqtt connected/disconnected/message callbacks.
     */
    void wireMqttCallbacks() noexcept;

    /**
     * Wires up the ethernet connected/disconnected callbacks.
     */
    void wireEthernetCallbacks() noexcept;

    /**
     * Resolves the broker config from settings and connects to mqtt, if configured.
     */
    void beginMqtt() noexcept;

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

    /**
     * Called by ethernetMan once the link is up and an IP has been acquired.
     */
    void onEthernetConnected() noexcept;

    /**
     * Called by ethernetMan once a previously established connection is lost.
     */
    void onEthernetDisconnected() noexcept;

    /**
     * Called by stateMachine whenever setState() applies a new state.
     */
    void onStateChange() noexcept;

    bool isInitialized = false;

    Gate gate;
    StateMachine& stateMachine;
    GpioPinRegister& gpioPinRegister;
    IGpio& i_gpio;
    IAdcOneshot& i_adcOneshot;
    IRandom& i_random;
    ITime& i_time;
    INVS& i_nvs;
    SettingsManager& settings;
    IMqtt& i_mqtt;
    IEthernetManager& i_ethernetMan;
    IHttpServer& i_httpServer;

    std::string lwtTopic;
};

#endif //LASERGATE_V2_SYSTEM_H
