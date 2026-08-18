#ifndef LASERGATE_V2_APICONTROLLER_H
#define LASERGATE_V2_APICONTROLLER_H

#include "SettingsManager.h"
#include "StateMachine.h"
#include "hal/IEthernetManager.h"
#include "hal/IMqtt.h"
#include <string>
#include <unordered_map>

/**
 * Executes parsed api commands against the application state.
 */
class ApiController {
public:
    /**
     * @param ethernetMan
     * @param mqtt
     * @param settings
     * @return "key=value\n" lines describing the device name and live connectivity state
     */
    [[nodiscard]] static std::string buildStatusReport(
        const IEthernetManager& ethernetMan,
        const IMqtt& mqtt,
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
     * @param stateMachine
     * @param form
     * @return Success state
     */
    [[nodiscard]] static bool applySettingsForm(
        SettingsManager& settings,
        StateMachine& stateMachine,
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
     * @param stateMachine
     * @param form
     * @return Success state
     */
    [[nodiscard]] static bool applyAdvancedSettingsForm(
        SettingsManager& settings,
        StateMachine& stateMachine,
        const std::unordered_map<std::string, std::string>& form
    ) noexcept;
};

#endif //LASERGATE_V2_APICONTROLLER_H
