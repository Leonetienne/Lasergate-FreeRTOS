#include "test/stubs/MqttStub.h"

MqttStub::MqttStub(
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

bool MqttStub::begin(const MqttConnectOptions& options) noexcept {
    lastConnectOptions = options;
    ++beginCallCount;
    isInitialized = true;
    return true;
}

bool MqttStub::free() noexcept {
    state = MqttConnectionState::Disconnected;
    isInitialized = false;
    indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
    pulseActive = false;
    return true;
}

bool MqttStub::publish(const std::string& topic, const std::string& payload, int qos, bool retain) noexcept {
    publishedMessages.push_back({topic, payload, qos, retain});
    triggerActivityPulse();
    return true;
}

bool MqttStub::subscribe(const std::string& topic, int qos) noexcept {
    subscriptions.push_back({topic, qos});
    return true;
}

MqttConnectionState MqttStub::getState() const noexcept {
    return state;
}

void MqttStub::setOnConnected(std::function<void()> callback) noexcept {
    onConnected = std::move(callback);
}

void MqttStub::setOnDisconnected(std::function<void()> callback) noexcept {
    onDisconnected = std::move(callback);
}

void MqttStub::setOnMessage(std::function<void(const std::string& topic, const std::string& payload)> callback) noexcept {
    onMessage = std::move(callback);
}

void MqttStub::updateActivityLedPulse() noexcept {
    if (!indicatorPin.isReady()) {
        return;
    }
    if (!pulseActive) {
        return;
    }

    if (i_time.getMillis() - lastActivityAtMs >= PULSE_DURATION_MS) {
        indicatorPin.setState(PIN_STATE_DIGITAL::HIGH);
        pulseActive = false;
    }
}

void MqttStub::triggerActivityPulse() noexcept {
    if (!indicatorPin.isReady()) {
        return;
    }

    indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
    lastActivityAtMs = i_time.getMillis();
    pulseActive = true;
}

void MqttStub::simulateConnected() {
    state = MqttConnectionState::Connected;
    indicatorPin.setState(PIN_STATE_DIGITAL::HIGH);
    pulseActive = false;
    if (onConnected) onConnected();
}

void MqttStub::simulateDisconnected() {
    state = MqttConnectionState::Disconnected;
    indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
    pulseActive = false;
    if (onDisconnected) onDisconnected();
}

void MqttStub::simulateMessage(const std::string& topic, const std::string& payload) {
    triggerActivityPulse();
    if (onMessage) onMessage(topic, payload);
}

const MqttConnectOptions& MqttStub::getLastConnectOptions() const {
    return lastConnectOptions;
}

int MqttStub::getBeginCallCount() const {
    return beginCallCount;
}

const std::vector<MqttPublishedMessage>& MqttStub::getPublishedMessages() const {
    return publishedMessages;
}

const std::vector<MqttSubscription>& MqttStub::getSubscriptions() const {
    return subscriptions;
}
