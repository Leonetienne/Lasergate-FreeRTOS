#include "platform/MqttEsp32.h"

#include <esp_log.h>

static const char* LOG_TAG = "MqttEsp32";

MqttEsp32::MqttEsp32(
    gpio_num_t indicatorGpioPin,
    IGpio& i_gpio,
    GpioPinRegister& pinRegister,
    const ITime& i_time
) noexcept :
    i_gpio(i_gpio),
    pinRegister(pinRegister),
    i_time(i_time),
    indicatorPin(pinRegister, i_gpio, indicatorGpioPin)
{
    if (indicatorPin.getGpioNum() != GPIO_NUM_NC) {
        if (indicatorPin.initialize()) {
            indicatorPin.setState(PIN_STATE_DIGITAL::HIGH);
        }
        else {
            indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
        }
    }
}

MqttEsp32::~MqttEsp32() noexcept {
    if (isInitialized) {
        MqttEsp32::free();
    }
}

bool MqttEsp32::begin(const MqttConnectOptions& options) noexcept {
    if (isInitialized) {
        return false;
    }

    brokerUri = options.brokerUri;
    username = options.username;
    password = options.password;
    lwtTopic = options.lwtTopic;
    lwtMessage = options.lwtMessage;

    esp_mqtt_client_config_t config = {};
    config.broker.address.uri = brokerUri.c_str();
    if (!username.empty()) {
        config.credentials.username = username.c_str();
    }
    if (!password.empty()) {
        config.credentials.authentication.password = password.c_str();
    }
    config.session.last_will.topic = lwtTopic.c_str();
    config.session.last_will.msg = lwtMessage.c_str();
    config.session.last_will.qos = 1;
    config.session.last_will.retain = true;

    client = esp_mqtt_client_init(&config);
    if (client == nullptr) {
        return false;
    }

    if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, &MqttEsp32::eventHandler, this) != ESP_OK) {
        esp_mqtt_client_destroy(client);
        client = nullptr;
        return false;
    }

    if (esp_mqtt_client_start(client) != ESP_OK) {
        esp_mqtt_client_destroy(client);
        client = nullptr;
        return false;
    }

    state = MqttConnectionState::Connecting;
    isInitialized = true;
    return true;
}

bool MqttEsp32::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    bool success = true;
    if (esp_mqtt_client_stop(client) != ESP_OK) {
        success = false;
    }
    if (esp_mqtt_client_destroy(client) != ESP_OK) {
        success = false;
    }

    client = nullptr;
    isInitialized = false;
    state = MqttConnectionState::Disconnected;
    indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
    pulseActive = false;
    return success;
}

bool MqttEsp32::publish(const std::string& topic, const std::string& payload, int qos, bool retain) noexcept {
    if (!isInitialized) {
        return false;
    }

    const int result = esp_mqtt_client_publish(
        client,
        topic.c_str(),
        payload.c_str(),
        static_cast<int>(payload.size()),
        qos,
        retain
    );
    triggerActivityPulse();
    return result >= 0;
}

bool MqttEsp32::subscribe(const std::string& topic, int qos) noexcept {
    if (!isInitialized) {
        return false;
    }

    return esp_mqtt_client_subscribe(client, topic.c_str(), qos) >= 0;
}

MqttConnectionState MqttEsp32::getState() const noexcept {
    return state;
}

void MqttEsp32::setOnConnected(std::function<void()> callback) noexcept {
    onConnected = std::move(callback);
}

void MqttEsp32::setOnDisconnected(std::function<void()> callback) noexcept {
    onDisconnected = std::move(callback);
}

void MqttEsp32::setOnMessage(std::function<void(const std::string& topic, const std::string& payload)> callback) noexcept {
    onMessage = std::move(callback);
}

void MqttEsp32::updateActivityLedPulse() noexcept {
    if (!indicatorPin.isReady() || !pulseActive) {
        return;
    }

    if (i_time.getMillis() - lastActivityAtMs >= PULSE_DURATION_MS) {
        indicatorPin.setState(PIN_STATE_DIGITAL::HIGH);
        pulseActive = false;
    }
}

void MqttEsp32::triggerActivityPulse() noexcept {
    if (!indicatorPin.isReady()) {
        return;
    }

    indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
    lastActivityAtMs = i_time.getMillis();
    pulseActive = true;
}

void MqttEsp32::eventHandler(void* arg, esp_event_base_t /*base*/, int32_t id, void* data) noexcept {
    auto* self = static_cast<MqttEsp32*>(arg);
    auto* event = static_cast<esp_mqtt_event_handle_t>(data);

    switch (id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(LOG_TAG, "connected");
            self->state = MqttConnectionState::Connected;
            self->indicatorPin.setState(PIN_STATE_DIGITAL::HIGH);
            self->pulseActive = false;
            if (self->onConnected) {
                self->onConnected();
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(LOG_TAG, "disconnected");
            self->state = MqttConnectionState::Disconnected;
            self->indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
            self->pulseActive = false;
            if (self->onDisconnected) {
                self->onDisconnected();
            }
            break;

        case MQTT_EVENT_DATA:
            self->triggerActivityPulse();
            if (self->onMessage) {
                self->onMessage(
                    std::string(event->topic, static_cast<std::size_t>(event->topic_len)),
                    std::string(event->data, static_cast<std::size_t>(event->data_len))
                );
            }
            break;

        default:
            break;
    }
}
