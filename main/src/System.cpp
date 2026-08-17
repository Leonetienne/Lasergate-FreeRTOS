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
    IGpio& gpio,
    ITime& i_time,
    INVS& nvs,
    SettingsManager& settings,
    IMqtt& mqtt
) noexcept :
    stateMachine(stateMachine),
    gpioPinRegister(gpioPinRegister),
    gpio(gpio),
    i_time(i_time),
    nvs(nvs),
    settings(settings),
    mqtt(mqtt),
    laserTestPin(gpioPinRegister, gpio, LASER_TEST_PIN)
{ }

System::~System() noexcept {
    if (isInitialized) {
        free();
    }
}

void System::init() noexcept {
    // nvs is expected to already be begun by the caller at this point: the mqtt
    // indicator LED pin is resolved from settings at construction time (see
    // getSystem()), which requires nvs to be readable before this System is even built.

    laserTestPin.initialize();
    if (laserTestPin.isReady()) {
        lastToggleMillis = i_time.getMillis();
    }

    mqtt.setOnConnected([this]() { onMqttConnected(); });
    mqtt.setOnDisconnected([this]() { onMqttDisconnected(); });
    mqtt.setOnMessage([this](const std::string& topic, const std::string& payload) {
        onMqttMessage(topic, payload);
    });

    // The network interface is expected to be brought up outside this class
    // (ethernet). esp_mqtt_client reconnects automatically until it succeeds.
    const auto brokerConfig = settings.retrieveMqttBrokerConfig();
    if (!brokerConfig.has_value() || brokerConfig->uri.empty()) {
        ESP_LOGI(LOG_TAG, "no broker uri configured, skipping mqtt connect");
        isInitialized = true;
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

    if (!mqtt.begin(options)) {
        ESP_LOGW(LOG_TAG, "mqtt.begin() failed for broker '%s'", options.brokerUri.c_str());
    }

    isInitialized = true;
}

void System::loop() noexcept {
    while (stateMachine.getState() != STATE::SHUTTING_DOWN) {
        update();
#ifndef HOST_BUILD
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
    if (mqtt.isReady() && !mqtt.free()) {
        success = false;
    }

    if (laserTestPin.isReady() && !laserTestPin.free()) {
        success = false;
    }

    if (!nvs.free()) {
        success = false;
    }

    isInitialized = false;
    return success;
}

void System::beforeShutdown() noexcept {
}

void System::update() noexcept {
    mqtt.updateActivityLedPulse();

    if (laserTestPin.isReady()) {
        const int64_t now = i_time.getMillis();
        if (now - lastToggleMillis >= LASER_TEST_TOGGLE_INTERVAL_MS) {
            lastToggleMillis = now;
            if (laserTestPin.getState() == PIN_STATE_DIGITAL::LOW) {
                laserTestPin.setState(PIN_STATE_DIGITAL::HIGH);
            } else {
                laserTestPin.setState(PIN_STATE_DIGITAL::LOW);
            }
        }
    }
}

void System::onMqttConnected() noexcept {
    ESP_LOGI(LOG_TAG, "mqtt connected, publishing availability");
    if (lwtTopic.empty()) {
        return;
    }
    mqtt.publish(lwtTopic, "Online", 1, true);
}

void System::onMqttDisconnected() noexcept {
    ESP_LOGW(LOG_TAG, "mqtt disconnected");
}

void System::onMqttMessage(const std::string& topic, const std::string& payload) noexcept {
    (void)topic;
    (void)payload;
    ESP_LOGI(LOG_TAG, "mqtt message on topic '%s': '%.*s'", topic.c_str(), static_cast<int>(payload.size()), payload.c_str());
}
