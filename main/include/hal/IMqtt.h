#ifndef LASERGATE_V2_IMQTT_H
#define LASERGATE_V2_IMQTT_H

#include <functional>
#include <string>
#include "enum/MqttConnectionState.h"
#include "MqttConnectOptions.h"
#include "compat/gpio_num_t.h"

/**
 * Abstract interface to a single mqtt broker connection
 */
class IMqtt {
public:
    IMqtt() = default;
    IMqtt(const IMqtt&) = delete;
    IMqtt& operator=(const IMqtt&) = delete;
    IMqtt(IMqtt&&) = delete;
    IMqtt& operator=(IMqtt&&) = delete;
    virtual ~IMqtt() = default;

    /**
     * Connects to the configured broker
     * @param options
     * @return Success state
     */
    virtual bool begin(const MqttConnectOptions& options) noexcept = 0;

    /**
     * Will release the resources acquired by begin()
     * @return Success state
     */
    virtual bool free() noexcept = 0;

    /**
     * Publishes a message. May be called while disconnected; the broker
     * connection is expected to still queue/drop it gracefully.
     * @return Success state
     */
    virtual bool publish(const std::string& topic, const std::string& payload, int qos, bool retain) noexcept = 0;

    /**
     * Subscribes to a topic
     * @return Success state
     */
    virtual bool subscribe(const std::string& topic, int qos) noexcept = 0;

    /**
     * @return The broker connection state
     */
    [[nodiscard]] virtual MqttConnectionState getState() const noexcept = 0;

    /**
     * Callback setter
     * @param callback
     */
    virtual void setOnConnected(std::function<void()> callback) noexcept = 0;

    /**
     * Callback setter
     * @param callback
     */
    virtual void setOnDisconnected(std::function<void()> callback) noexcept = 0;

    /**
     * Callback setter. Fired for every message received on a subscribed topic.
     * @param callback
     */
    virtual void setOnMessage(std::function<void(const std::string& topic, const std::string& payload)> callback) noexcept = 0;

    /**
     * Turns the activity LED back off once its pulse duration of 200ms has
     * elapsed since the last publish() call or received message.
     * Call repeatedly (e.g. every runtime loop tick).
     */
    virtual void updateActivityLedPulse() noexcept = 0;

    /**
     * @return Whether begin() has been called successfully without a matching free() yet
     */
    [[nodiscard]] bool isReady() const noexcept { return isInitialized; }

protected:
    bool isInitialized = false;
};

#endif //LASERGATE_V2_IMQTT_H
