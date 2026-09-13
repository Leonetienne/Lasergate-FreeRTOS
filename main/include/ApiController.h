#ifndef LASERGATE_V2_APICONTROLLER_H
#define LASERGATE_V2_APICONTROLLER_H

#include "Gate.h"
#include "GpioDiscovery.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "hal/IEthernetManager.h"
#include "hal/IMqtt.h"
#include <array>
#include <atomic>
#include <expected>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Executes parsed api commands against the application state.
 */
class ApiController {
public:
    ApiController() noexcept = default;
    ApiController(const ApiController&) = delete;
    ApiController(ApiController&&) = delete;
    ApiController& operator=(const ApiController&) = delete;

    /**
     * @param i_ethernetMan
     * @param i_mqtt
     * @param settings
     * @return "key=value\n" lines describing the device name and live connectivity state
     */
    [[nodiscard]] static std::string buildStatusReport(
        const IEthernetManager& i_ethernetMan,
        const IMqtt& i_mqtt,
        const SettingsManager& settings
    ) noexcept;

    /**
     * @param settings
     * @return "key=value\n" lines describing the current settings
     */
    [[nodiscard]] static std::string buildSettingsReport(const SettingsManager& settings) noexcept;

    /**
     * Applies a parsed settings form and requests a shutdown
     * @param settings
     * @param form
     * @return Success state
     */
    [[nodiscard]] bool applySettingsForm(
        SettingsManager& settings,
        const std::unordered_map<std::string, std::string>& form
    ) noexcept;

    /**
     * @param settings
     * @return "key=value\n" lines describing the current advanced settings
     */
    [[nodiscard]] static std::string buildAdvancedSettingsReport(const SettingsManager& settings) noexcept;

    /**
     * Applies a parsed advanced settings form and requests a shutdown
     * @param settings
     * @param form
     * @return void on success, otherwise why the form was rejected
     */
    [[nodiscard]] std::expected<void, std::string> applyAdvancedSettingsForm(
        SettingsManager& settings,
        const std::unordered_map<std::string, std::string>& form
    ) noexcept;

    /**
     * Requests a system state transition. Only records the request; it is applied
     * on the main thread by consumeDesiredSystemState().
     */
    void requestSystemState(STATE state) noexcept;

    /**
     * Takes and clears the pending state request, if any.
     * @return The requested state, or std::nullopt if none is pending
     */
    [[nodiscard]] std::optional<STATE> consumeDesiredSystemState() noexcept;

    /**
     * Live, read-only view of a single gate module, as reported to web clients.
     */
    struct GateModuleSnapshot {
        bool configured = false;
        bool ready = false;
        gpio_num_t laserPin = GPIO_NUM_NC;
        gpio_num_t statusLedPin = GPIO_NUM_NC;
        gpio_num_t ldrPin = GPIO_NUM_NC;
        uint16_t ldrThreshold = 0;
        uint16_t pulseFrequency = 0;
        std::optional<bool> pulseBatchAcceptable;
        std::optional<uint16_t> batchTimeMs;
        std::optional<uint16_t> selfTestErrorCount;
        std::optional<bool> selfTestFinished;
    };

    /**
     * Which role a gpio-discovery pin snapshot was captured under.
     */
    enum class GpioDiscoveryPinRole {
        OUTPUT,
        ADC1,
        ADC2
    };

    /**
     * Live, read-only view of a single gpio-discovery pin, as reported to web clients.
     */
    struct GpioDiscoveryPinSnapshot {
        gpio_num_t pin = GPIO_NUM_NC;
        GpioDiscoveryPinRole role = GpioDiscoveryPinRole::OUTPUT;
        uint16_t adcRaw = 0; // only meaningful when role is ADC1/ADC2
        bool level = false;  // only meaningful when role is OUTPUT
    };

    /**
     * Live, read-only view of overall system state, as reported to web clients.
     */
    struct SystemSnapshot {
        STATE state = STATE::NONE;
        std::string faultReason;
        int64_t uptimeMs = 0;
        std::array<GateModuleSnapshot, Gate::MODULE_COUNT> modules{};
        std::vector<GpioDiscoveryPinSnapshot> gpioDiscoveryPins; // only populated in STATE::DIAGNOSTIC_GPIO_DISCOVERY
    };

    /**
     * Builds a fresh snapshot of live system/module state for getSnapshot() to hand out.
     * Main thread only (System::update()), since it reads stateMachine/gate/gpioDiscovery directly.
     * gpioDiscoveryPins is populated while in STATE::DIAGNOSTIC_GPIO_DISCOVERY.
     */
    void publishSnapshot(
        const StateMachine& stateMachine,
        const Gate& gate,
        const GpioDiscovery& gpioDiscovery,
        int64_t uptimeMs
    ) noexcept;

    /**
     * @return A copy of the most recently published snapshot. Safe to call from any thread.
     */
    [[nodiscard]] SystemSnapshot getSnapshot() const noexcept;

    /**
     * @param snapshot The snapshot to serialize
     * @return snapshot serialized as the /api/state JSON payload (also used for the websocket push)
     */
    [[nodiscard]] static std::string buildStateJson(const SystemSnapshot& snapshot) noexcept;

    /**
     * Requests `requested` only if the transition table currently allows it from `current`.
     * @return Whether the request was queued
     */
    bool requestStateIfAllowed(STATE current, STATE requested) noexcept;

    /**
     * Applies a parsed manual module configuration form. Callers save all modules first,
     * then request one SHUTTING_DOWN.
     * @param settings The settings store to write to
     * @param moduleIndex Index of the gate module (0-based, < Gate::MODULE_COUNT)
     * @param form The submitted form fields
     * @return void on success, otherwise why the form was rejected. Rejected before any write:
     * board reserved pins, a non-analog ldr pin, duplicate pins, pins used elsewhere in the saved config.
     */
    [[nodiscard]] std::expected<void, std::string> applyModuleConfigForm(
        SettingsManager& settings,
        std::size_t moduleIndex,
        const std::unordered_map<std::string, std::string>& form
    ) noexcept;

private:
    std::atomic<STATE> desiredSystemState { STATE::NONE };
    mutable std::mutex snapshotMutex;
    SystemSnapshot snapshot;
};

#endif //LASERGATE_V2_APICONTROLLER_H
