#include "../include/SettingsManager.h"
#include <cstdio>

SettingsManager::SettingsManager(INVS &i_nvs) noexcept :
    i_nvs (i_nvs)
{ }

SettingsManager::SettingsManager(SettingsManager && other) noexcept :
    i_nvs(other.i_nvs)
{ }

bool SettingsManager::storeTitle(const std::string &title) const noexcept {
    return i_nvs.setString("title", title.c_str());
}

std::expected<std::string, bool> SettingsManager::retrieveTitle() const noexcept {
    char buf[NVS_MAX_STRING_LENGTH + 1] = {};
    if (!i_nvs.getString("title", buf)) {
        return std::unexpected(false);
    }
    return std::string(buf);
}

bool SettingsManager::storeMqttBrokerConfig(const MqttBrokerConfig& config) const noexcept {
    return
        i_nvs.setString("mqtt_uri", config.uri.c_str()) &&
        i_nvs.setString("mqtt_user", config.username.c_str()) &&
        i_nvs.setString("mqtt_pass", config.password.c_str());
}

std::expected<MqttBrokerConfig, bool> SettingsManager::retrieveMqttBrokerConfig() const noexcept {
    char uri[NVS_MAX_STRING_LENGTH + 1] = {};
    if (!i_nvs.getString("mqtt_uri", uri)) {
        return std::unexpected(false);
    }

    char user[NVS_MAX_STRING_LENGTH + 1] = {};
    char pass[NVS_MAX_STRING_LENGTH + 1] = {};
    i_nvs.getString("mqtt_user", user);
    i_nvs.getString("mqtt_pass", pass);

    return MqttBrokerConfig{uri, user, pass};
}

bool SettingsManager::storeMqttNodeId(const std::string& nodeId) const noexcept {
    return i_nvs.setString("node_id", nodeId.c_str());
}

std::expected<std::string, bool> SettingsManager::retrieveMqttNodeId() const noexcept {
    char buf[NVS_MAX_STRING_LENGTH + 1] = {};
    if (!i_nvs.getString("node_id", buf)) {
        return std::unexpected(false);
    }
    return std::string(buf);
}

bool SettingsManager::storeMqttLedGpioPin(gpio_num_t gpioPin) const noexcept {
    return i_nvs.setInt("mqtt_led_gpio", static_cast<int32_t>(gpioPin));
}

std::expected<gpio_num_t, bool> SettingsManager::retrieveMqttLedGpioPin() const noexcept {
    if (int32_t buf{}; i_nvs.getInt("mqtt_led_gpio", buf)) {
        return static_cast<gpio_num_t>(buf);
    }
    return std::unexpected(false);
}

bool SettingsManager::storeEthernetLedGpioPin(gpio_num_t gpioPin) const noexcept {
    return i_nvs.setInt("eth_led_gpio", static_cast<int32_t>(gpioPin));
}

std::expected<gpio_num_t, bool> SettingsManager::retrieveEthernetLedGpioPin() const noexcept {
    if (int32_t buf{}; i_nvs.getInt("eth_led_gpio", buf)) {
        return static_cast<gpio_num_t>(buf);
    }
    return std::unexpected(false);
}

bool SettingsManager::storeConnLedsEnabled(bool enabled) const noexcept {
    return i_nvs.setInt("conn_leds_en", enabled ? 1 : 0);
}

std::expected<bool, bool> SettingsManager::retrieveConnLedsEnabled() const noexcept {
    if (int32_t buf{}; i_nvs.getInt("conn_leds_en", buf)) {
        return buf != 0;
    }
    return std::unexpected(false);
}

bool SettingsManager::storeGateModuleLaserGpioPin(std::size_t moduleIndex, gpio_num_t gpioPin) const noexcept {
    char key[16];
    snprintf(key, sizeof(key), "gm%zu_laser_gpio", moduleIndex);
    return i_nvs.setInt(key, static_cast<int32_t>(gpioPin));
}

std::expected<gpio_num_t, bool> SettingsManager::retrieveGateModuleLaserGpioPin(std::size_t moduleIndex) const noexcept {
    char key[16];
    snprintf(key, sizeof(key), "gm%zu_laser_gpio", moduleIndex);
    if (int32_t buf{}; i_nvs.getInt(key, buf)) {
        return static_cast<gpio_num_t>(buf);
    }
    return std::unexpected(false);
}

bool SettingsManager::storeGateModuleLedGpioPin(std::size_t moduleIndex, gpio_num_t gpioPin) const noexcept {
    char key[16];
    snprintf(key, sizeof(key), "gm%zu_led_gpio", moduleIndex);
    return i_nvs.setInt(key, static_cast<int32_t>(gpioPin));
}

std::expected<gpio_num_t, bool> SettingsManager::retrieveGateModuleLedGpioPin(std::size_t moduleIndex) const noexcept {
    char key[16];
    snprintf(key, sizeof(key), "gm%zu_led_gpio", moduleIndex);
    if (int32_t buf{}; i_nvs.getInt(key, buf)) {
        return static_cast<gpio_num_t>(buf);
    }
    return std::unexpected(false);
}

bool SettingsManager::storeGateModuleLdrGpioPin(std::size_t moduleIndex, gpio_num_t gpioPin) const noexcept {
    char key[16];
    snprintf(key, sizeof(key), "gm%zu_ldr_gpio", moduleIndex);
    return i_nvs.setInt(key, static_cast<int32_t>(gpioPin));
}

std::expected<gpio_num_t, bool> SettingsManager::retrieveGateModuleLdrGpioPin(std::size_t moduleIndex) const noexcept {
    char key[16];
    snprintf(key, sizeof(key), "gm%zu_ldr_gpio", moduleIndex);
    if (int32_t buf{}; i_nvs.getInt(key, buf)) {
        return static_cast<gpio_num_t>(buf);
    }
    return std::unexpected(false);
}
