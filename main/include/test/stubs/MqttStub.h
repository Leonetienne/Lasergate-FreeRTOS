#ifndef LASERGATE_V2_MQTTSTUB_H
#define LASERGATE_V2_MQTTSTUB_H

#include "hal/IMqtt.h"
#include "hal/IGpio.h"
#include "hal/ITime.h"
#include "GpioPinRegister.h"
#include "platform/GpioDigitalWritePin.h"
#include <string>
#include <vector>

struct MqttPublishedMessage {
    std::string topic;
    std::string payload;
    int qos;
    bool retain;
};

struct MqttSubscription {
    std::string topic;
    int qos;
};

class MqttStub : public IMqtt {
public:
    MqttStub(gpio_num_t indicatorGpioPin, IGpio& i_gpio, GpioPinRegister& pinRegister, const ITime& i_time) noexcept;
    MqttStub(const MqttStub&) = delete;
    MqttStub& operator=(const MqttStub&) = delete;
    MqttStub(MqttStub&&) = delete;
    MqttStub& operator=(MqttStub&&) = delete;

    bool begin(const MqttConnectOptions& options) noexcept override;
    bool free() noexcept override;
    bool publish(const std::string& topic, const std::string& payload, int qos, bool retain) noexcept override;
    bool subscribe(const std::string& topic, int qos) noexcept override;
    [[nodiscard]] MqttConnectionState getState() const noexcept override;
    void setOnConnected(std::function<void()> callback) noexcept override;
    void setOnDisconnected(std::function<void()> callback) noexcept override;
    void setOnMessage(std::function<void(const std::string& topic, const std::string& payload)> callback) noexcept override;

    void updateActivityLedPulse() noexcept override;

    /**
     * fires callbacks, simulates a real connect/disconnect/incoming message event
     */
    void simulateConnected();
    void simulateDisconnected();
    void simulateMessage(const std::string& topic, const std::string& payload);

    [[nodiscard]] const MqttConnectOptions& getLastConnectOptions() const;
    [[nodiscard]] int getBeginCallCount() const;
    [[nodiscard]] const std::vector<MqttPublishedMessage>& getPublishedMessages() const;
    [[nodiscard]] const std::vector<MqttSubscription>& getSubscriptions() const;

private:
    void triggerActivityPulse() noexcept;

    static constexpr int64_t PULSE_DURATION_MS = 100;

    IGpio& i_gpio;
    GpioPinRegister& pinRegister;
    const ITime& i_time;

    MqttConnectionState state = MqttConnectionState::Disconnected;
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(const std::string&, const std::string&)> onMessage;

    MqttConnectOptions lastConnectOptions;
    int beginCallCount = 0;
    std::vector<MqttPublishedMessage> publishedMessages;
    std::vector<MqttSubscription> subscriptions;

    GpioDigitalWritePin indicatorPin;
    int64_t lastActivityAtMs = 0;
    bool pulseActive = false;
};

#endif //LASERGATE_V2_MQTTSTUB_H
