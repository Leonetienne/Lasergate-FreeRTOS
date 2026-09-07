#ifndef LASERGATE_V2_APICONTROLLER_H
#define LASERGATE_V2_APICONTROLLER_H

#include "SettingsManager.h"
#include "StateMachine.h"
#include "hal/IEthernetManager.h"
#include "hal/IMqtt.h"
#include <atomic>
#include <optional>
#include <string>
#include <unordered_map>

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
     * @return Success state
     */
    [[nodiscard]] bool applyAdvancedSettingsForm(
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

private:
    std::atomic<STATE> desiredSystemState { STATE::NONE };
};

#endif //LASERGATE_V2_APICONTROLLER_H
