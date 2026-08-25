//
// Created by Leon Etienne on 18.03.26.
//

#include "System.h"
#include "compat/esp_log_macros.h"
#ifndef HOST_BUILD
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static const char* LOG_TAG = "System";

System::System(
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
) noexcept :
    gate(stateMachine, settings, gpioPinRegister, i_gpio, i_adcOneshot, i_random, i_time),
    stateMachine(stateMachine),
    gpioPinRegister(gpioPinRegister),
    i_gpio(i_gpio),
    i_adcOneshot(i_adcOneshot),
    i_random(i_random),
    i_time(i_time),
    i_nvs(i_nvs),
    settings(settings),
    i_mqtt(i_mqtt),
    i_ethernetMan(i_ethernetMan),
    i_httpServer(i_httpServer)
{ }

System::~System() noexcept {
    if (isInitialized) {
        free();
    }
}

void System::initialize() noexcept {
    // nvs is expected to already be begun by the caller at this point: the mqtt
    // indicator LED pin is resolved from settings at construction time (see
    // getSystem()), which requires nvs to be readable before this System is even built.

    wireMqttCallbacks();
    wireEthernetCallbacks();
    stateMachine.setOnStateChange([this]() { onStateChange(); });

    if (i_adcOneshot.initialize() != ESP_OK) {
        stateMachine.setState(STATE::FAULT, "System::initialize: adcOneshot.initialize() failed");
    }
    if (!gate.initialize()) {
        stateMachine.setState(STATE::FAULT, "System::initialize: gate.initialize() failed");
    }
    if (!i_ethernetMan.begin()) {
        stateMachine.setState(STATE::FAULT, "System::initialize: ethernetMan.begin() failed");
    }
    if (!i_httpServer.begin()) {
        stateMachine.setState(STATE::FAULT, "System::initialize: httpServer.begin() failed");
    }

    // esp_mqtt_client reconnects automatically until it succeeds, so it's fine
    // to begin it before ethernet has actually acquired a link/ip.
    beginMqtt();

    isInitialized = true;
}

void System::wireMqttCallbacks() noexcept {
    i_mqtt.setOnConnected([this]() { onMqttConnected(); });
    i_mqtt.setOnDisconnected([this]() { onMqttDisconnected(); });
    i_mqtt.setOnMessage([this](const std::string& topic, const std::string& payload) {
        onMqttMessage(topic, payload);
    });
}

void System::wireEthernetCallbacks() noexcept {
    i_ethernetMan.setOnConnected([this]() { onEthernetConnected(); });
    i_ethernetMan.setOnDisconnected([this]() { onEthernetDisconnected(); });
}

void System::beginMqtt() noexcept {
    const auto brokerConfig = settings.retrieveMqttBrokerConfig();
    if (!brokerConfig.has_value() || brokerConfig->uri.empty()) {
        ESP_LOGI(LOG_TAG, "no broker uri configured, skipping mqtt connect");
        return;
    }

    std::string nodeId = settings.retrieveMqttNodeId().value_or("lasergate");
    if (nodeId.empty()) {
        nodeId = "lasergate";
    }
    lwtTopic = "tele/lasergate/" + nodeId + "/LWT";

    const MqttConnectOptions options{
        brokerConfig->uri,
        brokerConfig->username,
        brokerConfig->password,
        lwtTopic,
        "Offline"
    };

    if (!i_mqtt.begin(options)) {
        ESP_LOGW(LOG_TAG, "mqtt.begin() failed for broker '%s'", options.brokerUri.c_str());
    }
}

void System::loop() noexcept {
    while (stateMachine.getState() != STATE::SHUTTING_DOWN) {
        update();
#ifndef HOST_BUILD
        // this results in fixedUpdate being called every 10+n ms, n being overhead, instead of every 10,
        // but it's fine, timing is not that sensitive.
        vTaskDelay(pdMS_TO_TICKS(10));
#endif
    }
    beforeShutdown();
}

bool System::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    // Tear everything down even if an individual step fails, so pins stay
    // unbound and nvs gets closed; report the aggregated outcome.
    bool success = true;
    if (i_mqtt.isReady() && !i_mqtt.free()) {
        success = false;
    }

    // httpServer may already be stopped (e.g. ethernet never came up). that's fine
    i_httpServer.free();

    if (gate.isReady() && !gate.free()) {
        success = false;
    }

    if (i_ethernetMan.isReady() && !i_ethernetMan.free()) {
        success = false;
    }

    if (!i_nvs.free()) {
        success = false;
    }

    isInitialized = false;
    return success;
}

void System::beforeShutdown() noexcept {
}

void System::update() noexcept {
    i_mqtt.updateActivityLedPulse();
    gate.fixedUpdate();
}

void System::onMqttConnected() noexcept {
    ESP_LOGI(LOG_TAG, "mqtt connected, publishing availability");
    if (lwtTopic.empty()) {
        return;
    }
    i_mqtt.publish(lwtTopic, "Online", 1, true);
}

void System::onMqttDisconnected() noexcept {
    ESP_LOGW(LOG_TAG, "mqtt disconnected");
}

void System::onMqttMessage(const std::string& topic, const std::string& payload) noexcept {
    (void)topic;
    (void)payload;
    ESP_LOGI(LOG_TAG, "mqtt message on topic '%s': '%.*s'", topic.c_str(), static_cast<int>(payload.size()), payload.c_str());
}

void System::onEthernetConnected() noexcept {
    ESP_LOGI(LOG_TAG, "ethernet connected");
}

void System::onEthernetDisconnected() noexcept {
    ESP_LOGW(LOG_TAG, "ethernet disconnected");
}

void System::onStateChange() noexcept {
    gate.onStateChange();
}
