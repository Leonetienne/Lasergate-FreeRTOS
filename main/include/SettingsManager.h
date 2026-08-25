#ifndef LASERGATE_V2_SETTINGSMANAGER_H
#define LASERGATE_V2_SETTINGSMANAGER_H

#include "hal/INVS.h"
#include "compat/gpio_num_t.h"
#include "MqttBrokerConfig.h"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>

/**
 * Class to interface with NVS to store and retrieve settings as well as defaults
 */
class SettingsManager {
public:
    SettingsManager(INVS& i_nvs) noexcept;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager(SettingsManager&&) noexcept;
    SettingsManager& operator=(const SettingsManager&) = delete;
    SettingsManager& operator=(SettingsManager&&) = delete;

    /**
     * @return Success state
     */
    bool storeTitle(const std::string& title) const noexcept;
    [[nodiscard]] std::expected<std::string, bool> retrieveTitle() const noexcept;

    /**
     * @return Success state
     */
    bool storeMqttBrokerConfig(const MqttBrokerConfig& config) const noexcept;
    [[nodiscard]] std::expected<MqttBrokerConfig, bool> retrieveMqttBrokerConfig() const noexcept;

    /**
     * @return Success state
     */
    bool storeMqttNodeId(const std::string& nodeId) const noexcept;
    [[nodiscard]] std::expected<std::string, bool> retrieveMqttNodeId() const noexcept;

    /**
     * @return Success state
     */
    bool storeMqttLedGpioPin(gpio_num_t gpioPin) const noexcept;
    [[nodiscard]] std::expected<gpio_num_t, bool> retrieveMqttLedGpioPin() const noexcept;

    /**
     * @return Success state
     */
    bool storeEthernetLedGpioPin(gpio_num_t gpioPin) const noexcept;
    [[nodiscard]] std::expected<gpio_num_t, bool> retrieveEthernetLedGpioPin() const noexcept;

    /**
     * Master switch for the ethernet/mqtt connectivity status LEDs
     * @return Success state
     */
    bool storeConnLedsEnabled(bool enabled) const noexcept;
    [[nodiscard]] std::expected<bool, bool> retrieveConnLedsEnabled() const noexcept;

    /**
     * @param moduleIndex Index of the gate module (0-based)
     * @return Success state
     */
    bool storeGateModuleLaserGpioPin(std::size_t moduleIndex, gpio_num_t gpioPin) const noexcept;
    [[nodiscard]] std::expected<gpio_num_t, bool> retrieveGateModuleLaserGpioPin(std::size_t moduleIndex) const noexcept;

    /**
     * @param moduleIndex Index of the gate module (0-based)
     * @return Success state
     */
    bool storeGateModuleLedGpioPin(std::size_t moduleIndex, gpio_num_t gpioPin) const noexcept;
    [[nodiscard]] std::expected<gpio_num_t, bool> retrieveGateModuleLedGpioPin(std::size_t moduleIndex) const noexcept;

    /**
     * @param moduleIndex Index of the gate module (0-based)
     * @return Success state
     */
    bool storeGateModuleLdrGpioPin(std::size_t moduleIndex, gpio_num_t gpioPin) const noexcept;
    [[nodiscard]] std::expected<gpio_num_t, bool> retrieveGateModuleLdrGpioPin(std::size_t moduleIndex) const noexcept;

    /**
     * @param moduleIndex Index of the gate module (0-based)
     * @return Success state
     */
    bool storeGateModuleLdrThreshold(std::size_t moduleIndex, uint16_t threshold) const noexcept;
    [[nodiscard]] std::expected<uint16_t, bool> retrieveGateModuleLdrThreshold(std::size_t moduleIndex) const noexcept;

    /**
     * @param moduleIndex Index of the gate module (0-based)
     * @return Success state
     */
    bool storeGateModuleLaserPulseFrequency(std::size_t moduleIndex, uint16_t pulseFrequency) const noexcept;
    [[nodiscard]] std::expected<uint16_t, bool> retrieveGateModuleLaserPulseFrequency(std::size_t moduleIndex) const noexcept;

private:
    INVS& i_nvs;
};

#endif //LASERGATE_V2_SETTINGSMANAGER_H
