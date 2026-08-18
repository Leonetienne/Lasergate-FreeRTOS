#include "ApiController.h"
#include "compat/gpio_num_t.h"

namespace {

// manual atoi, no exceptions on embedded
bool parseInt32(std::string_view str, int32_t& out) noexcept {
    if (str.empty()) {
        return false;
    }

    int32_t value = 0;
    for (const char c : str) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + (c - '0');
    }

    out = value;
    return true;
}

// blank means "not connected", matching the GPIO_NUM_NC sentinel
gpio_num_t parseGpioField(const std::unordered_map<std::string, std::string>& form, const std::string& key) noexcept {
    const auto it = form.find(key);
    if (it == form.end() || it->second.empty()) {
        return GPIO_NUM_NC;
    }

    int32_t value = 0;
    if (!parseInt32(it->second, value)) {
        return GPIO_NUM_NC;
    }

    return static_cast<gpio_num_t>(value);
}

void appendGpioField(std::string& report, const std::string& key, gpio_num_t pin) noexcept {
    report += key;
    report += '=';
    if (pin != GPIO_NUM_NC) {
        report += std::to_string(static_cast<int32_t>(pin));
    }
    report += '\n';
}

std::string formValue(const std::unordered_map<std::string, std::string>& form, const std::string& key) noexcept {
    const auto it = form.find(key);
    return it != form.end() ? it->second : std::string();
}

const char* toString(EthernetConnectionState state) noexcept {
    switch (state) {
        case EthernetConnectionState::Connecting: return "connecting";
        case EthernetConnectionState::Connected: return "connected";
        case EthernetConnectionState::Disconnected: default: return "disconnected";
    }
}

const char* toString(MqttConnectionState state) noexcept {
    switch (state) {
        case MqttConnectionState::Connecting: return "connecting";
        case MqttConnectionState::Connected: return "connected";
        case MqttConnectionState::Failed: return "failed";
        case MqttConnectionState::Disconnected: default: return "disconnected";
    }
}

}

std::string ApiController::buildStatusReport(
    const IEthernetManager& ethernetMan,
    const IMqtt& mqtt,
    const SettingsManager& settings
) noexcept {
    std::string report;

    report += "device_name=";
    if (const auto title = settings.retrieveTitle(); title.has_value()) {
        report += *title;
    }
    report += '\n';

    report += "ethernet_state=";
    report += toString(ethernetMan.getState());
    report += '\n';

    report += "mqtt_state=";
    report += toString(mqtt.getState());
    report += '\n';

    return report;
}

std::string ApiController::buildSettingsReport(const SettingsManager& settings) noexcept {
    std::string report;

    report += "device_name=";
    if (const auto title = settings.retrieveTitle(); title.has_value()) {
        report += *title;
    }
    report += '\n';

    const auto mqttConfig = settings.retrieveMqttBrokerConfig();

    report += "mqtt_uri=";
    if (mqttConfig.has_value()) {
        report += mqttConfig->uri;
    }
    report += '\n';

    report += "mqtt_user=";
    if (mqttConfig.has_value()) {
        report += mqttConfig->username;
    }
    report += '\n';

    report += "mqtt_password_set=";
    report += (mqttConfig.has_value() && !mqttConfig->password.empty()) ? '1' : '0';
    report += '\n';

    report += "node_id=";
    if (const auto nodeId = settings.retrieveMqttNodeId(); nodeId.has_value()) {
        report += *nodeId;
    }
    report += '\n';

    return report;
}

bool ApiController::applySettingsForm(
    SettingsManager& settings,
    StateMachine& stateMachine,
    const std::unordered_map<std::string, std::string>& form
) noexcept {
    const auto nameIt = form.find("device_name");
    if (nameIt == form.end() || nameIt->second.empty()) {
        return false;
    }

    MqttBrokerConfig mqttConfig{
        formValue(form, "mqtt_uri"),
        formValue(form, "mqtt_user"),
        formValue(form, "mqtt_pass")
    };

    // a blank password means "keep the current one", unless there isn't one to keep
    if (mqttConfig.password.empty()) {
        if (const auto existing = settings.retrieveMqttBrokerConfig(); existing.has_value()) {
            mqttConfig.password = existing->password;
        }
    }

    if (!settings.storeTitle(nameIt->second) ||
        !settings.storeMqttBrokerConfig(mqttConfig) ||
        !settings.storeMqttNodeId(formValue(form, "node_id"))) {
        return false;
    }

    stateMachine.setState(STATE::SHUTTING_DOWN);
    return true;
}

std::string ApiController::buildAdvancedSettingsReport(const SettingsManager& settings) noexcept {
    std::string report;

    const auto ethernetLedPin = settings.retrieveEthernetLedGpioPin();
    appendGpioField(report, "ethernet_led_gpio", ethernetLedPin.value_or(GPIO_NUM_NC));

    const auto mqttLedPin = settings.retrieveMqttLedGpioPin();
    appendGpioField(report, "mqtt_led_gpio", mqttLedPin.value_or(GPIO_NUM_NC));

    report += "conn_leds_enabled=";
    if (const auto connLedsEnabled = settings.retrieveConnLedsEnabled(); connLedsEnabled.has_value()) {
        report += *connLedsEnabled ? '1' : '0';
    }
    report += '\n';

    return report;
}

bool ApiController::applyAdvancedSettingsForm(
    SettingsManager& settings,
    StateMachine& stateMachine,
    const std::unordered_map<std::string, std::string>& form
) noexcept {
    const gpio_num_t ethernetLedPin = parseGpioField(form, "ethernet_led_gpio");
    const gpio_num_t mqttLedPin = parseGpioField(form, "mqtt_led_gpio");
    const bool connLedsEnabled = form.contains("enable_conn_leds");

    if (!settings.storeEthernetLedGpioPin(ethernetLedPin) ||
        !settings.storeMqttLedGpioPin(mqttLedPin) ||
        !settings.storeConnLedsEnabled(connLedsEnabled)) {
        return false;
    }

    stateMachine.setState(STATE::SHUTTING_DOWN);
    return true;
}
