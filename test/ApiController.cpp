#include <catch2/catch_test_macros.hpp>
#include "Gate.h"
#include "GpioDiscovery.h"
#include "GpioPinRegister.h"
#include "hal/AdcGpioMapping.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/TimeStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/MqttStub.h"
#include "test/stubs/EthernetManagerStub.h"
#include "SettingsManager.h"
#include "ApiController.h"
#include <algorithm>

TEST_CASE("ApiController: buildStatusReport", "[ApiController]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_NC, gpioStub, pr, timeStub);
    EthernetManagerStub ethernetStub(GPIO_NUM_NC, gpioStub, pr, timeStub);

    SECTION("reports blank device name and disconnected states by default") {
        REQUIRE(ApiController::buildStatusReport(ethernetStub, mqttStub, settings) ==
            "device_name=\nethernet_state=disconnected\nmqtt_state=disconnected\n");
    }

    SECTION("reports the stored device name") {
        REQUIRE(settings.storeTitle("Lasergate"));
        REQUIRE(ApiController::buildStatusReport(ethernetStub, mqttStub, settings) ==
            "device_name=Lasergate\nethernet_state=disconnected\nmqtt_state=disconnected\n");
    }

    SECTION("reflects a connected ethernet interface") {
        ethernetStub.simulateConnected();
        REQUIRE(ApiController::buildStatusReport(ethernetStub, mqttStub, settings) ==
            "device_name=\nethernet_state=connected\nmqtt_state=disconnected\n");
    }

    SECTION("reflects a connected mqtt broker") {
        mqttStub.simulateConnected();
        REQUIRE(ApiController::buildStatusReport(ethernetStub, mqttStub, settings) ==
            "device_name=\nethernet_state=disconnected\nmqtt_state=connected\n");
    }
}

TEST_CASE("ApiController: settings report/form", "[ApiController]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    ApiController apiController;

    SECTION("buildSettingsReport reports blank fields when nothing was stored") {
        REQUIRE(ApiController::buildSettingsReport(settings) ==
            "device_name=\nmqtt_uri=\nmqtt_user=\nmqtt_password_set=0\nnode_id=\n");
    }

    SECTION("buildSettingsReport reports stored values") {
        REQUIRE(settings.storeTitle("Lasergate"));
        REQUIRE(settings.storeMqttBrokerConfig(MqttBrokerConfig{"mqtt://broker:1883", "mqttuser", "mqttpass"}));
        REQUIRE(settings.storeMqttNodeId("lasergate_node"));
        REQUIRE(ApiController::buildSettingsReport(settings) ==
            "device_name=Lasergate\nmqtt_uri=mqtt://broker:1883\nmqtt_user=mqttuser\n"
            "mqtt_password_set=1\nnode_id=lasergate_node\n");
    }

    SECTION("applySettingsForm stores the submitted values") {
        const std::unordered_map<std::string, std::string> form = {
            {"device_name", "Lasergate"},
            {"mqtt_uri", "mqtt://broker:1883"},
            {"mqtt_user", "mqttuser"},
            {"mqtt_pass", "mqttpass"},
            {"node_id", "lasergate_node"},
        };
        REQUIRE(apiController.applySettingsForm(settings, form));
        REQUIRE(*settings.retrieveTitle() == "Lasergate");
        REQUIRE(settings.retrieveMqttBrokerConfig()->uri == "mqtt://broker:1883");
        REQUIRE(*settings.retrieveMqttNodeId() == "lasergate_node");
    }

    SECTION("applySettingsForm requests a shutdown on success") {
        const std::unordered_map<std::string, std::string> form = {{"device_name", "Lasergate"}};
        REQUIRE(apiController.applySettingsForm(settings, form));
        REQUIRE(apiController.consumeDesiredSystemState() == STATE::SHUTTING_DOWN);
    }

    SECTION("applySettingsForm fails when device_name is missing") {
        const std::unordered_map<std::string, std::string> form = {{"mqtt_uri", "mqtt://broker:1883"}};
        REQUIRE_FALSE(apiController.applySettingsForm(settings, form));
    }

    SECTION("applySettingsForm does not request a shutdown when it fails") {
        const std::unordered_map<std::string, std::string> form = {};
        apiController.applySettingsForm(settings, form);
        REQUIRE_FALSE(apiController.consumeDesiredSystemState().has_value());
    }

    SECTION("a blank mqtt password keeps the previously stored one") {
        const std::unordered_map<std::string, std::string> initial = {
            {"device_name", "Lasergate"},
            {"mqtt_uri", "mqtt://broker:1883"},
            {"mqtt_pass", "hunter2"},
        };
        REQUIRE(apiController.applySettingsForm(settings, initial));

        const std::unordered_map<std::string, std::string> update = {
            {"device_name", "Lasergate"},
            {"mqtt_uri", "mqtt://broker:1883"},
        };
        REQUIRE(apiController.applySettingsForm(settings, update));

        REQUIRE(settings.retrieveMqttBrokerConfig()->password == "hunter2");
    }
}

TEST_CASE("ApiController: desired system state", "[ApiController]") {
    ApiController apiController;

    SECTION("consumeDesiredSystemState returns nullopt when nothing was requested") {
        REQUIRE_FALSE(apiController.consumeDesiredSystemState().has_value());
    }

    SECTION("consumeDesiredSystemState returns the requested state") {
        apiController.requestSystemState(STATE::SHUTTING_DOWN);
        REQUIRE(apiController.consumeDesiredSystemState() == STATE::SHUTTING_DOWN);
    }

    SECTION("consumeDesiredSystemState is single-shot") {
        apiController.requestSystemState(STATE::SHUTTING_DOWN);
        REQUIRE(apiController.consumeDesiredSystemState() == STATE::SHUTTING_DOWN);
        REQUIRE_FALSE(apiController.consumeDesiredSystemState().has_value());
    }
}

TEST_CASE("ApiController: advanced settings report/form", "[ApiController]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    ApiController apiController;

    SECTION("buildAdvancedSettingsReport reports blank fields when nothing was stored") {
        REQUIRE(ApiController::buildAdvancedSettingsReport(settings) ==
            "ethernet_led_gpio=\nmqtt_led_gpio=\nconn_leds_enabled=\n");
    }

    SECTION("buildAdvancedSettingsReport reports stored values") {
        REQUIRE(settings.storeEthernetLedGpioPin(GPIO_NUM_2));
        REQUIRE(settings.storeMqttLedGpioPin(GPIO_NUM_3));
        REQUIRE(settings.storeConnLedsEnabled(false));

        REQUIRE(ApiController::buildAdvancedSettingsReport(settings) ==
            "ethernet_led_gpio=2\nmqtt_led_gpio=3\nconn_leds_enabled=0\n");
    }

    SECTION("applyAdvancedSettingsForm stores the submitted values") {
        const std::unordered_map<std::string, std::string> form = {
            {"ethernet_led_gpio", "2"},
            {"mqtt_led_gpio", "15"},
            {"enable_conn_leds", "1"},
        };
        REQUIRE(apiController.applyAdvancedSettingsForm(settings, form));
        REQUIRE(*settings.retrieveEthernetLedGpioPin() == GPIO_NUM_2);
        REQUIRE(*settings.retrieveMqttLedGpioPin() == GPIO_NUM_15);
        REQUIRE(*settings.retrieveConnLedsEnabled());
    }

    SECTION("applyAdvancedSettingsForm disables the conn leds master switch when its checkbox is absent") {
        const std::unordered_map<std::string, std::string> form = {};
        REQUIRE(apiController.applyAdvancedSettingsForm(settings, form));
        REQUIRE_FALSE(*settings.retrieveConnLedsEnabled());
    }

    SECTION("applyAdvancedSettingsForm leaves pins unset for blank gpio fields") {
        const std::unordered_map<std::string, std::string> form = {};
        REQUIRE(apiController.applyAdvancedSettingsForm(settings, form));
        REQUIRE(*settings.retrieveEthernetLedGpioPin() == GPIO_NUM_NC);
        REQUIRE(*settings.retrieveMqttLedGpioPin() == GPIO_NUM_NC);
    }

    SECTION("applyAdvancedSettingsForm accepts the highest esp32s3 gpio number") {
        const std::unordered_map<std::string, std::string> form = {
            {"ethernet_led_gpio", "48"},
        };
        REQUIRE(apiController.applyAdvancedSettingsForm(settings, form));
        REQUIRE(*settings.retrieveEthernetLedGpioPin() == GPIO_NUM_48);
    }

    SECTION("applyAdvancedSettingsForm rejects gpio numbers beyond the esp32s3 range") {
        const std::unordered_map<std::string, std::string> form = {
            {"ethernet_led_gpio", "49"},
        };
        REQUIRE(apiController.applyAdvancedSettingsForm(settings, form));
        REQUIRE(*settings.retrieveEthernetLedGpioPin() == GPIO_NUM_NC);
    }

    SECTION("applyAdvancedSettingsForm rejects a board reserved pin") {
        const std::unordered_map<std::string, std::string> form = {
            {"ethernet_led_gpio", "3"},
            {"enable_conn_leds", "1"},
        };
        REQUIRE_FALSE(apiController.applyAdvancedSettingsForm(settings, form));
        REQUIRE_FALSE(settings.retrieveEthernetLedGpioPin().has_value());
        REQUIRE_FALSE(apiController.consumeDesiredSystemState().has_value());
    }

    SECTION("applyAdvancedSettingsForm rejects both leds on the same pin") {
        const std::unordered_map<std::string, std::string> form = {
            {"ethernet_led_gpio", "2"},
            {"mqtt_led_gpio", "2"},
            {"enable_conn_leds", "1"},
        };
        REQUIRE_FALSE(apiController.applyAdvancedSettingsForm(settings, form));
    }

    SECTION("applyAdvancedSettingsForm rejects a pin used by a configured module") {
        REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_48));
        REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_16));

        const std::unordered_map<std::string, std::string> form = {
            {"ethernet_led_gpio", "16"},
            {"enable_conn_leds", "1"},
        };
        const auto result = apiController.applyAdvancedSettingsForm(settings, form);
        REQUIRE_FALSE(result);
        REQUIRE(result.error() == "Ethernet LED GPIO 16 is already used by module 0 LDR");
    }

    SECTION("applyAdvancedSettingsForm skips pin checks while the conn leds are disabled") {
        REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_48));
        REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_16));

        const std::unordered_map<std::string, std::string> form = {{"ethernet_led_gpio", "16"}};
        REQUIRE(apiController.applyAdvancedSettingsForm(settings, form));
    }

    SECTION("applyAdvancedSettingsForm requests a shutdown on success") {
        const std::unordered_map<std::string, std::string> form = {};
        REQUIRE(apiController.applyAdvancedSettingsForm(settings, form));
        REQUIRE(apiController.consumeDesiredSystemState() == STATE::SHUTTING_DOWN);
    }
}

TEST_CASE("ApiController: state snapshot", "[ApiController]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    AdcOneshotStub adcStub2(ADC_UNIT_2);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine{};
    ApiController apiController;
    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, adcStub2, randomStub, timeStub);
    GpioDiscovery gpioDiscovery(pr, gpioStub, adcStub, adcStub2);

    SECTION("getSnapshot defaults to STATE::NONE") {
        REQUIRE(apiController.getSnapshot().state == STATE::NONE);
    }

    SECTION("publishSnapshot captures the current state and uptime") {
        stateMachine.setState(STATE::DISARMED);
        apiController.publishSnapshot(stateMachine, gate, gpioDiscovery, 12345);

        const auto snapshot = apiController.getSnapshot();
        REQUIRE(snapshot.state == STATE::DISARMED);
        REQUIRE(snapshot.uptimeMs == 12345);
        REQUIRE(snapshot.faultReason.empty());
    }

    SECTION("publishSnapshot captures the fault reason while in FAULT") {
        stateMachine.setState(STATE::FAULT, "sensor read failure");
        apiController.publishSnapshot(stateMachine, gate, gpioDiscovery, 0);

        REQUIRE(apiController.getSnapshot().faultReason == "sensor read failure");
    }

    SECTION("publishSnapshot reports every module as unconfigured by default") {
        apiController.publishSnapshot(stateMachine, gate, gpioDiscovery, 0);

        const auto snapshot = apiController.getSnapshot();
        for (const auto& m : snapshot.modules) {
            REQUIRE_FALSE(m.configured);
            REQUIRE(m.laserPin == GPIO_NUM_NC);
            REQUIRE(m.statusLedPin == GPIO_NUM_NC);
            REQUIRE(m.ldrPin == GPIO_NUM_NC);
        }
    }

    SECTION("publishSnapshot reports a configured module's pins") {
        REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_41));
        REQUIRE(settings.storeGateModuleLedGpioPin(0, GPIO_NUM_42));
        REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_1));
        Gate configuredGate(stateMachine, settings, pr, gpioStub, adcStub, adcStub2, randomStub, timeStub);

        apiController.publishSnapshot(stateMachine, configuredGate, gpioDiscovery, 0);

        const auto snapshot = apiController.getSnapshot();
        const auto& m0 = snapshot.modules[0];
        REQUIRE(m0.configured);
        REQUIRE(m0.laserPin == GPIO_NUM_41);
        REQUIRE(m0.statusLedPin == GPIO_NUM_42);
        REQUIRE(m0.ldrPin == GPIO_NUM_1);
    }

    SECTION("publishSnapshot leaves gpioDiscoveryPins empty outside STATE::DIAGNOSTIC_GPIO_DISCOVERY") {
        stateMachine.setState(STATE::DISARMED);
        gpioDiscovery.begin();

        apiController.publishSnapshot(stateMachine, gate, gpioDiscovery, 0);

        REQUIRE(apiController.getSnapshot().gpioDiscoveryPins.empty());
    }

    SECTION("publishSnapshot reports adc1/adc2 readings and free output levels while active") {
        stateMachine.setState(STATE::DISARMED);
        stateMachine.setState(STATE::DIAGNOSTIC_GPIO_DISCOVERY);
        gpioDiscovery.begin();
        const auto channel1 = AdcGpioMapping::gpioToChannel(GPIO_NUM_1);
        REQUIRE(channel1.has_value());
        adcStub.test_setChannelValue(channel1->second, 999);
        const auto channel2 = AdcGpioMapping::gpioToChannel(GPIO_NUM_15);
        REQUIRE(channel2.has_value());
        adcStub2.test_setChannelValue(channel2->second, 555);
        REQUIRE(gpioDiscovery.setPinLevel(GPIO_NUM_21, true));

        apiController.publishSnapshot(stateMachine, gate, gpioDiscovery, 0);
        const auto& pins = apiController.getSnapshot().gpioDiscoveryPins;

        const auto adc1Entry = std::find_if(pins.begin(), pins.end(), [](const auto& p) { return p.pin == GPIO_NUM_1; });
        REQUIRE(adc1Entry != pins.end());
        REQUIRE(adc1Entry->role == ApiController::GpioDiscoveryPinRole::ADC1);
        REQUIRE(adc1Entry->adcRaw == 999);

        const auto adc2Entry = std::find_if(pins.begin(), pins.end(), [](const auto& p) { return p.pin == GPIO_NUM_15; });
        REQUIRE(adc2Entry != pins.end());
        REQUIRE(adc2Entry->role == ApiController::GpioDiscoveryPinRole::ADC2);
        REQUIRE(adc2Entry->adcRaw == 555);

        const auto outputEntry = std::find_if(pins.begin(), pins.end(), [](const auto& p) { return p.pin == GPIO_NUM_21; });
        REQUIRE(outputEntry != pins.end());
        REQUIRE(outputEntry->role == ApiController::GpioDiscoveryPinRole::OUTPUT);
        REQUIRE(outputEntry->level);
    }
}

TEST_CASE("ApiController: buildStateJson", "[ApiController]") {
    SECTION("serializes a fully default snapshot") {
        const ApiController::SystemSnapshot snapshot;

        REQUIRE(ApiController::buildStateJson(snapshot) ==
            "{\"state\":\"NONE\",\"fault_reason\":\"\",\"uptime_ms\":0,\"allowed_transitions\":[],"
            "\"modules\":["
            "{\"index\":0,\"configured\":false,\"ready\":false,\"laser_gpio\":null,\"led_gpio\":null,\"ldr_gpio\":null,"
            "\"ldr_threshold\":0,\"pulse_frequency_ms\":0,\"pulse_batch_acceptable\":null,\"batch_time_ms\":null,"
            "\"self_test_error_count\":null,\"self_test_finished\":null},"
            "{\"index\":1,\"configured\":false,\"ready\":false,\"laser_gpio\":null,\"led_gpio\":null,\"ldr_gpio\":null,"
            "\"ldr_threshold\":0,\"pulse_frequency_ms\":0,\"pulse_batch_acceptable\":null,\"batch_time_ms\":null,"
            "\"self_test_error_count\":null,\"self_test_finished\":null},"
            "{\"index\":2,\"configured\":false,\"ready\":false,\"laser_gpio\":null,\"led_gpio\":null,\"ldr_gpio\":null,"
            "\"ldr_threshold\":0,\"pulse_frequency_ms\":0,\"pulse_batch_acceptable\":null,\"batch_time_ms\":null,"
            "\"self_test_error_count\":null,\"self_test_finished\":null},"
            "{\"index\":3,\"configured\":false,\"ready\":false,\"laser_gpio\":null,\"led_gpio\":null,\"ldr_gpio\":null,"
            "\"ldr_threshold\":0,\"pulse_frequency_ms\":0,\"pulse_batch_acceptable\":null,\"batch_time_ms\":null,"
            "\"self_test_error_count\":null,\"self_test_finished\":null}"
            "],\"gpio_pins\":[]}"
        );
    }

    SECTION("lists allowed_transitions for the current state, in STATE declaration order") {
        ApiController::SystemSnapshot snapshot;
        snapshot.state = STATE::DISARMED;

        const std::string json = ApiController::buildStateJson(snapshot);
        REQUIRE(json.find(
            "\"allowed_transitions\":[\"USER_ADJUSTING_BEAMS\",\"CALIBRATION_LDR_THRESH\","
            "\"CALIBRATION_MODULATION_FREQUENCY\",\"OBSERVING\",\"DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST\","
            "\"DIAGNOSTIC_GPIO_DISCOVERY\",\"FAULT\",\"SHUTTING_DOWN\"]"
        ) != std::string::npos);
    }

    SECTION("lists no allowed_transitions from NONE") {
        ApiController::SystemSnapshot snapshot;
        snapshot.state = STATE::NONE;

        REQUIRE(ApiController::buildStateJson(snapshot).find("\"allowed_transitions\":[]") != std::string::npos);
    }

    SECTION("serializes populated module fields, including present optionals") {
        ApiController::SystemSnapshot snapshot;
        snapshot.modules[2].configured = true;
        snapshot.modules[2].ready = true;
        snapshot.modules[2].laserPin = GPIO_NUM_4;
        snapshot.modules[2].ldrThreshold = 1820;
        snapshot.modules[2].pulseFrequency = 8;
        snapshot.modules[2].pulseBatchAcceptable = true;
        snapshot.modules[2].batchTimeMs = 256;
        snapshot.modules[2].selfTestErrorCount = 0;
        snapshot.modules[2].selfTestFinished = false;

        const std::string json = ApiController::buildStateJson(snapshot);
        REQUIRE(json.find(
            "{\"index\":2,\"configured\":true,\"ready\":true,\"laser_gpio\":4,\"led_gpio\":null,\"ldr_gpio\":null,"
            "\"ldr_threshold\":1820,\"pulse_frequency_ms\":8,\"pulse_batch_acceptable\":true,\"batch_time_ms\":256,"
            "\"self_test_error_count\":0,\"self_test_finished\":false}"
        ) != std::string::npos);
    }

    SECTION("escapes the fault reason") {
        ApiController::SystemSnapshot snapshot;
        snapshot.faultReason = "sensor \"read\" failure";

        REQUIRE(ApiController::buildStateJson(snapshot).find("\"fault_reason\":\"sensor \\\"read\\\" failure\"") != std::string::npos);
    }

    SECTION("serializes an adc1 pin, an adc2 pin and an output pin") {
        ApiController::SystemSnapshot snapshot;
        snapshot.gpioDiscoveryPins.push_back({GPIO_NUM_1, ApiController::GpioDiscoveryPinRole::ADC1, 2048, false});
        snapshot.gpioDiscoveryPins.push_back({GPIO_NUM_15, ApiController::GpioDiscoveryPinRole::ADC2, 1024, false});
        snapshot.gpioDiscoveryPins.push_back({GPIO_NUM_21, ApiController::GpioDiscoveryPinRole::OUTPUT, 0, true});

        REQUIRE(ApiController::buildStateJson(snapshot).find(
            "\"gpio_pins\":["
            "{\"gpio\":1,\"role\":\"adc1\",\"adc_raw\":2048,\"level\":null},"
            "{\"gpio\":15,\"role\":\"adc2\",\"adc_raw\":1024,\"level\":null},"
            "{\"gpio\":21,\"role\":\"output\",\"adc_raw\":null,\"level\":true}"
            "]}"
        ) != std::string::npos);
    }
}

TEST_CASE("ApiController: requestStateIfAllowed", "[ApiController]") {
    ApiController apiController;

    SECTION("queues the request when the transition is allowed") {
        REQUIRE(apiController.requestStateIfAllowed(STATE::DISARMED, STATE::OBSERVING));
        REQUIRE(apiController.consumeDesiredSystemState() == STATE::OBSERVING);
    }

    SECTION("rejects a disallowed transition") {
        REQUIRE_FALSE(apiController.requestStateIfAllowed(STATE::INITIALIZING, STATE::CALIBRATION_LDR_THRESH));
        REQUIRE_FALSE(apiController.consumeDesiredSystemState().has_value());
    }
}
