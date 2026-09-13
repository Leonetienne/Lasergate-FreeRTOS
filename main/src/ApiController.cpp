#include "ApiController.h"
#include "JsonWriter.h"
#include "compat/gpio_num_t.h"
#include "hal/AdcGpioMapping.h"
#include "hal/BoardReservedPins.h"
#include <algorithm>
#include <span>

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
    if (!parseInt32(it->second, value) || value < 0 || value >= GPIO_NUM_MAX) {
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

using PinClaims = std::unordered_map<int32_t, std::string>;

std::string pinLabel(gpio_num_t pin) {
    return "GPIO " + std::to_string(static_cast<int32_t>(pin));
}

// only modules with a laser and an ldr pin bind anything at boot
void claimModulePins(PinClaims& claims, const SettingsManager& settings, std::optional<std::size_t> exceptModule) {
    for (std::size_t i = 0; i < Gate::MODULE_COUNT; ++i) {
        if (i == exceptModule) {
            continue;
        }

        const gpio_num_t laserPin = settings.retrieveGateModuleLaserGpioPin(i).value_or(GPIO_NUM_NC);
        const gpio_num_t ledPin = settings.retrieveGateModuleLedGpioPin(i).value_or(GPIO_NUM_NC);
        const gpio_num_t ldrPin = settings.retrieveGateModuleLdrGpioPin(i).value_or(GPIO_NUM_NC);
        if (laserPin == GPIO_NUM_NC || ldrPin == GPIO_NUM_NC) {
            continue;
        }

        const std::string owner = "module " + std::to_string(i);
        claims.try_emplace(laserPin, owner + " laser");
        claims.try_emplace(ldrPin, owner + " LDR");
        if (ledPin != GPIO_NUM_NC) {
            claims.try_emplace(ledPin, owner + " status LED");
        }
    }
}

void claimConnLedPins(PinClaims& claims, const SettingsManager& settings) {
    if (!settings.retrieveConnLedsEnabled().value_or(true)) {
        return;
    }

    const gpio_num_t ethernetLedPin = settings.retrieveEthernetLedGpioPin().value_or(GPIO_NUM_NC);
    const gpio_num_t mqttLedPin = settings.retrieveMqttLedGpioPin().value_or(GPIO_NUM_NC);
    if (ethernetLedPin != GPIO_NUM_NC) {
        claims.try_emplace(ethernetLedPin, "the Ethernet status LED");
    }
    if (mqttLedPin != GPIO_NUM_NC) {
        claims.try_emplace(mqttLedPin, "the MQTT status LED");
    }
}

std::string pinUnusableReason(gpio_num_t pin) {
    if (std::ranges::find(BoardReservedPins::ALL_PINS, pin) == BoardReservedPins::ALL_PINS.end()) {
        return "does not exist on this chip";
    }
    if (BoardReservedPins::isReserved(pin)) {
        return "is reserved by the board";
    }
    return {};
}

struct PinRole {
    gpio_num_t pin;
    const char* name;
    bool needsAdc;
};

std::expected<void, std::string> validatePinRoles(std::span<const PinRole> roles, const PinClaims& claims) {
    for (std::size_t i = 0; i < roles.size(); ++i) {
        const PinRole& role = roles[i];
        if (role.pin == GPIO_NUM_NC) {
            continue;
        }

        const std::string subject = std::string(role.name) + " " + pinLabel(role.pin);

        if (const std::string reason = pinUnusableReason(role.pin); !reason.empty()) {
            return std::unexpected(subject + " " + reason);
        }
        if (role.needsAdc && !AdcGpioMapping::gpioToChannel(role.pin).has_value()) {
            return std::unexpected(subject + " needs an ADC pin");
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (roles[j].pin == role.pin) {
                return std::unexpected(std::string(roles[j].name) + " and " + role.name + " can't share " + pinLabel(role.pin));
            }
        }
        if (const auto claim = claims.find(role.pin); claim != claims.end()) {
            return std::unexpected(subject + " is already used by " + claim->second);
        }
    }
    return {};
}

std::string formValue(const std::unordered_map<std::string, std::string>& form, const std::string& key) noexcept {
    const auto it = form.find(key);
    return it != form.end() ? it->second : std::string();
}

// blank/invalid/out-of-range yields 0, the "unset" value
uint16_t parseUint16Field(const std::unordered_map<std::string, std::string>& form, const std::string& key) noexcept {
    const auto it = form.find(key);
    if (it == form.end() || it->second.empty()) {
        return 0;
    }

    int32_t value = 0;
    if (!parseInt32(it->second, value) || value < 0 || value > UINT16_MAX) {
        return 0;
    }

    return static_cast<uint16_t>(value);
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
    const IEthernetManager& i_ethernetMan,
    const IMqtt& i_mqtt,
    const SettingsManager& settings
) noexcept {
    std::string report;

    report += "device_name=";
    if (const auto title = settings.retrieveTitle(); title.has_value()) {
        report += *title;
    }
    report += '\n';

    report += "ethernet_state=";
    report += toString(i_ethernetMan.getState());
    report += '\n';

    report += "mqtt_state=";
    report += toString(i_mqtt.getState());
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

    requestSystemState(STATE::SHUTTING_DOWN);
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

std::expected<void, std::string> ApiController::applyAdvancedSettingsForm(
    SettingsManager& settings,
    const std::unordered_map<std::string, std::string>& form
) noexcept {
    const gpio_num_t ethernetLedPin = parseGpioField(form, "ethernet_led_gpio");
    const gpio_num_t mqttLedPin = parseGpioField(form, "mqtt_led_gpio");
    const bool connLedsEnabled = form.contains("enable_conn_leds");

    if (connLedsEnabled) {
        PinClaims claims;
        claimModulePins(claims, settings, std::nullopt);

        const std::array<PinRole, 2> roles{{
            {ethernetLedPin, "Ethernet LED", false},
            {mqttLedPin, "MQTT LED", false},
        }};
        if (const auto valid = validatePinRoles(roles, claims); !valid) {
            return valid;
        }
    }

    if (!settings.storeEthernetLedGpioPin(ethernetLedPin) ||
        !settings.storeMqttLedGpioPin(mqttLedPin) ||
        !settings.storeConnLedsEnabled(connLedsEnabled)) {
        return std::unexpected("Failed to write settings");
    }

    requestSystemState(STATE::SHUTTING_DOWN);
    return {};
}

void ApiController::requestSystemState(STATE state) noexcept {
    desiredSystemState.store(state);
}

std::optional<STATE> ApiController::consumeDesiredSystemState() noexcept {
    const STATE state = desiredSystemState.exchange(STATE::NONE);
    if (state == STATE::NONE) {
        return std::nullopt;
    }
    return state;
}

void ApiController::publishSnapshot(
    const StateMachine& stateMachine,
    const Gate& gate,
    const GpioDiscovery& gpioDiscovery,
    int64_t uptimeMs
) noexcept {
    SystemSnapshot next;
    next.state = stateMachine.getState();
    next.faultReason = stateMachine.getLastFaultReason();
    next.uptimeMs = uptimeMs;

    for (std::size_t i = 0; i < Gate::MODULE_COUNT; ++i) {
        const GateModule& module = gate.getModule(i);
        GateModuleSnapshot& moduleSnapshot = next.modules[i];

        moduleSnapshot.configured = module.isConfigured();
        moduleSnapshot.ready = module.isReady();
        moduleSnapshot.laserPin = module.getLaserPin();
        moduleSnapshot.statusLedPin = module.getStatusLedPin();
        moduleSnapshot.ldrPin = module.getLdrPin();
        moduleSnapshot.ldrThreshold = module.getLdrThreshold();
        moduleSnapshot.pulseFrequency = module.getPulseFrequency();
        moduleSnapshot.pulseBatchAcceptable = module.isPulseBatchAcceptable();
        moduleSnapshot.batchTimeMs = module.getBatchTime();
        moduleSnapshot.selfTestErrorCount = module.getLastDiagnosticSignalNoiseSelfTestErrorCount();
        moduleSnapshot.selfTestFinished = module.isDiagnosticSignalNoiseSelfTestFinished();
    }

    if (next.state == STATE::DIAGNOSTIC_GPIO_DISCOVERY) {
        for (const auto& reading : gpioDiscovery.readAdc1()) {
            if (reading.has_value()) {
                next.gpioDiscoveryPins.push_back({reading->first, GpioDiscoveryPinRole::ADC1, reading->second, false});
            }
        }
        for (const auto& reading : gpioDiscovery.readAdc2()) {
            if (reading.has_value()) {
                next.gpioDiscoveryPins.push_back({reading->first, GpioDiscoveryPinRole::ADC2, reading->second, false});
            }
        }
        for (const auto& pin : gpioDiscovery.getDrivenPins()) {
            if (pin.has_value()) {
                next.gpioDiscoveryPins.push_back({
                    *pin, GpioDiscoveryPinRole::OUTPUT, 0, gpioDiscovery.getPinLevel(*pin).value_or(false)
                });
            }
        }
    }

    const std::lock_guard<std::mutex> lock(snapshotMutex);
    snapshot = std::move(next);
}

ApiController::SystemSnapshot ApiController::getSnapshot() const noexcept {
    const std::lock_guard<std::mutex> lock(snapshotMutex);
    return snapshot;
}

std::string ApiController::buildStateJson(const SystemSnapshot& snapshot) noexcept {
    std::string json = "{";

    json += "\"state\":";
    json += JsonWriter::string(StateMachine::toString(snapshot.state));
    json += ",\"fault_reason\":";
    json += JsonWriter::string(snapshot.faultReason);
    json += ",\"uptime_ms\":";
    json += JsonWriter::number(snapshot.uptimeMs);

    json += ",\"allowed_transitions\":[";
    bool firstTransition = true;
    for (const STATE candidate : {
        STATE::INITIALIZING, STATE::USER_ADJUSTING_BEAMS, STATE::CALIBRATION_LDR_THRESH,
        STATE::CALIBRATION_MODULATION_FREQUENCY, STATE::OBSERVING, STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST,
        STATE::DIAGNOSTIC_GPIO_DISCOVERY, STATE::DISARMED, STATE::ALARM, STATE::FAULT, STATE::SHUTTING_DOWN
    }) {
        if (!StateMachine::isTransitionAllowed(snapshot.state, candidate)) {
            continue;
        }
        if (!firstTransition) {
            json += ',';
        }
        firstTransition = false;
        json += JsonWriter::string(StateMachine::toString(candidate));
    }
    json += ']';

    json += ",\"modules\":[";
    for (std::size_t i = 0; i < snapshot.modules.size(); ++i) {
        const GateModuleSnapshot& m = snapshot.modules[i];
        if (i > 0) {
            json += ',';
        }

        json += "{\"index\":";
        json += JsonWriter::number(static_cast<int64_t>(i));
        json += ",\"configured\":";
        json += JsonWriter::boolean(m.configured);
        json += ",\"ready\":";
        json += JsonWriter::boolean(m.ready);
        json += ",\"laser_gpio\":";
        json += m.laserPin != GPIO_NUM_NC ? JsonWriter::number(m.laserPin) : std::string(JsonWriter::null());
        json += ",\"led_gpio\":";
        json += m.statusLedPin != GPIO_NUM_NC ? JsonWriter::number(m.statusLedPin) : std::string(JsonWriter::null());
        json += ",\"ldr_gpio\":";
        json += m.ldrPin != GPIO_NUM_NC ? JsonWriter::number(m.ldrPin) : std::string(JsonWriter::null());
        json += ",\"ldr_threshold\":";
        json += JsonWriter::number(m.ldrThreshold);
        json += ",\"pulse_frequency_ms\":";
        json += JsonWriter::number(m.pulseFrequency);
        json += ",\"pulse_batch_acceptable\":";
        json += JsonWriter::optBoolean(m.pulseBatchAcceptable);
        json += ",\"batch_time_ms\":";
        json += JsonWriter::optNumber(m.batchTimeMs);
        json += ",\"self_test_error_count\":";
        json += JsonWriter::optNumber(m.selfTestErrorCount);
        json += ",\"self_test_finished\":";
        json += JsonWriter::optBoolean(m.selfTestFinished);
        json += '}';
    }
    json += ']';

    json += ",\"gpio_pins\":[";
    for (std::size_t i = 0; i < snapshot.gpioDiscoveryPins.size(); ++i) {
        const GpioDiscoveryPinSnapshot& p = snapshot.gpioDiscoveryPins[i];
        if (i > 0) {
            json += ',';
        }

        const bool isAdc = p.role != GpioDiscoveryPinRole::OUTPUT;

        json += "{\"gpio\":";
        json += JsonWriter::number(p.pin);
        json += ",\"role\":";
        json += JsonWriter::string(
            p.role == GpioDiscoveryPinRole::ADC1 ? "adc1" : p.role == GpioDiscoveryPinRole::ADC2 ? "adc2" : "output"
        );
        json += ",\"adc_raw\":";
        json += isAdc ? JsonWriter::number(p.adcRaw) : std::string(JsonWriter::null());
        json += ",\"level\":";
        json += isAdc ? std::string(JsonWriter::null()) : std::string(JsonWriter::boolean(p.level));
        json += '}';
    }
    json += ']';

    json += '}';
    return json;
}

bool ApiController::requestStateIfAllowed(STATE current, STATE requested) noexcept {
    if (!StateMachine::isTransitionAllowed(current, requested)) {
        return false;
    }

    requestSystemState(requested);
    return true;
}

std::expected<void, std::string> ApiController::applyModuleConfigForm(
    SettingsManager& settings,
    std::size_t moduleIndex,
    const std::unordered_map<std::string, std::string>& form
) noexcept {
    const gpio_num_t laserPin = parseGpioField(form, "laser_gpio");
    const gpio_num_t ledPin = parseGpioField(form, "led_gpio");
    const gpio_num_t ldrPin = parseGpioField(form, "ldr_gpio");
    const uint16_t ldrThreshold = parseUint16Field(form, "ldr_thresh");
    const uint16_t pulseFrequency = parseUint16Field(form, "pulse_freq");

    PinClaims claims;
    claimModulePins(claims, settings, moduleIndex);
    claimConnLedPins(claims, settings);

    const std::array<PinRole, 3> roles{{
        {laserPin, "Laser", false},
        {ledPin, "Status LED", false},
        {ldrPin, "LDR", true},
    }};
    if (const auto valid = validatePinRoles(roles, claims); !valid) {
        return valid;
    }

    if (!settings.storeGateModuleLaserGpioPin(moduleIndex, laserPin) ||
        !settings.storeGateModuleLedGpioPin(moduleIndex, ledPin) ||
        !settings.storeGateModuleLdrGpioPin(moduleIndex, ldrPin) ||
        !settings.storeGateModuleLdrThreshold(moduleIndex, ldrThreshold) ||
        !settings.storeGateModuleLaserPulseFrequency(moduleIndex, pulseFrequency)) {
        return std::unexpected("Failed to write settings");
    }

    return {};
}

std::expected<void, std::string> ApiController::resetSettingsToDefaults(
    SettingsManager& settings,
    STATE current
) noexcept {
    if (current != STATE::DISARMED && current != STATE::FAULT) {
        return std::unexpected("Settings can only be reset while disarmed or in fault");
    }

    if (!settings.resetToDefaults()) {
        return std::unexpected("Failed to erase settings");
    }

    requestSystemState(STATE::SHUTTING_DOWN);
    return {};
}
