#include "ApiController.h"
#include "JsonWriter.h"
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

bool ApiController::applyAdvancedSettingsForm(
    SettingsManager& settings,
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

    requestSystemState(STATE::SHUTTING_DOWN);
    return true;
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
