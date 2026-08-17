#ifndef LASERGATE_V2_MQTTESP32_H
#define LASERGATE_V2_MQTTESP32_H

#include "hal/IMqtt.h"
#include "hal/IGpio.h"
#include "hal/ITime.h"
#include "GpioPinRegister.h"
#include "platform/GpioDigitalWritePin.h"
#include "mqtt_client.h"

class MqttEsp32 : public IMqtt {
public:
    MqttEsp32(gpio_num_t indicatorGpioPin, IGpio& gpio, GpioPinRegister& pinRegister, const ITime& i_time) noexcept;
    MqttEsp32(const MqttEsp32&) = delete;
    MqttEsp32& operator=(const MqttEsp32&) = delete;
    MqttEsp32(MqttEsp32&&) = delete;
    MqttEsp32& operator=(MqttEsp32&&) = delete;
    ~MqttEsp32() noexcept override;

    /**
     * Connects to the configured broker
     * @param options
     * @return Success state
     */
    bool begin(const MqttConnectOptions& options) noexcept override;

    /**
     * Stops the mqtt client and releases the resources acquired by begin()
     * @return Success state
     */
    bool free() noexcept override;

    bool publish(const std::string& topic, const std::string& payload, int qos, bool retain) noexcept override;
    bool subscribe(const std::string& topic, int qos) noexcept override;

    [[nodiscard]] MqttConnectionState getState() const noexcept override;

    void setOnConnected(std::function<void()> callback) noexcept override;
    void setOnDisconnected(std::function<void()> callback) noexcept override;
    void setOnMessage(std::function<void(const std::string& topic, const std::string& payload)> callback) noexcept override;

    void updateActivityLedPulse() noexcept override;

private:
    /**
     * Static esp-idf event callback. Runs on the esp-mqtt client task, i.e.
     * concurrently with System::update()/getState() on the app task - the
     * connection state/callbacks are intentionally not synchronized beyond the
     * esp-mqtt client's own locks.
     */
    static void eventHandler(void* arg, esp_event_base_t base, int32_t id, void* data) noexcept;

    void triggerActivityPulse() noexcept;

    static constexpr int64_t PULSE_DURATION_MS = 100;

    IGpio& gpio;
    GpioPinRegister& pinRegister;
    const ITime& i_time;

    MqttConnectionState state = MqttConnectionState::Disconnected;
    esp_mqtt_client_handle_t client = nullptr;

    // esp_mqtt_client_config_t only stores pointers into these - keep them alive for the connection's lifetime
    std::string brokerUri;
    std::string username;
    std::string password;
    std::string lwtTopic;
    std::string lwtMessage;

    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(const std::string&, const std::string&)> onMessage;

    GpioDigitalWritePin indicatorPin;
    int64_t lastActivityAtMs = 0;
    bool pulseActive = false;
};

#endif //LASERGATE_V2_MQTTESP32_H
