#include <catch2/catch_test_macros.hpp>
#include "GpioPinRegister.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/TimeStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/MqttStub.h"
#include "test/stubs/EthernetManagerStub.h"
#include "StateMachine.h"
#include "SettingsManager.h"
#include "ApiController.h"

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
    StateMachine stateMachine;

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
        REQUIRE(ApiController::applySettingsForm(settings, stateMachine, form));
        REQUIRE(*settings.retrieveTitle() == "Lasergate");
        REQUIRE(settings.retrieveMqttBrokerConfig()->uri == "mqtt://broker:1883");
        REQUIRE(*settings.retrieveMqttNodeId() == "lasergate_node");
    }

    SECTION("applySettingsForm requests a shutdown on success") {
        const std::unordered_map<std::string, std::string> form = {{"device_name", "Lasergate"}};
        REQUIRE(ApiController::applySettingsForm(settings, stateMachine, form));
        REQUIRE(stateMachine.getState() == STATE::SHUTTING_DOWN);
    }

    SECTION("applySettingsForm fails when device_name is missing") {
        const std::unordered_map<std::string, std::string> form = {{"mqtt_uri", "mqtt://broker:1883"}};
        REQUIRE_FALSE(ApiController::applySettingsForm(settings, stateMachine, form));
    }

    SECTION("applySettingsForm does not request a shutdown when it fails") {
        const std::unordered_map<std::string, std::string> form = {};
        ApiController::applySettingsForm(settings, stateMachine, form);
        REQUIRE(stateMachine.getState() == STATE::INITIALIZING);
    }

    SECTION("a blank mqtt password keeps the previously stored one") {
        const std::unordered_map<std::string, std::string> initial = {
            {"device_name", "Lasergate"},
            {"mqtt_uri", "mqtt://broker:1883"},
            {"mqtt_pass", "hunter2"},
        };
        REQUIRE(ApiController::applySettingsForm(settings, stateMachine, initial));

        const std::unordered_map<std::string, std::string> update = {
            {"device_name", "Lasergate"},
            {"mqtt_uri", "mqtt://broker:1883"},
        };
        REQUIRE(ApiController::applySettingsForm(settings, stateMachine, update));

        REQUIRE(settings.retrieveMqttBrokerConfig()->password == "hunter2");
    }
}

TEST_CASE("ApiController: advanced settings report/form", "[ApiController]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine;

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
            {"mqtt_led_gpio", "3"},
            {"enable_conn_leds", "1"},
        };
        REQUIRE(ApiController::applyAdvancedSettingsForm(settings, stateMachine, form));
        REQUIRE(*settings.retrieveEthernetLedGpioPin() == GPIO_NUM_2);
        REQUIRE(*settings.retrieveMqttLedGpioPin() == GPIO_NUM_3);
        REQUIRE(*settings.retrieveConnLedsEnabled());
    }

    SECTION("applyAdvancedSettingsForm disables the conn leds master switch when its checkbox is absent") {
        const std::unordered_map<std::string, std::string> form = {};
        REQUIRE(ApiController::applyAdvancedSettingsForm(settings, stateMachine, form));
        REQUIRE_FALSE(*settings.retrieveConnLedsEnabled());
    }

    SECTION("applyAdvancedSettingsForm leaves pins unset for blank gpio fields") {
        const std::unordered_map<std::string, std::string> form = {};
        REQUIRE(ApiController::applyAdvancedSettingsForm(settings, stateMachine, form));
        REQUIRE(*settings.retrieveEthernetLedGpioPin() == GPIO_NUM_NC);
        REQUIRE(*settings.retrieveMqttLedGpioPin() == GPIO_NUM_NC);
    }

    SECTION("applyAdvancedSettingsForm accepts the highest esp32s3 gpio number") {
        const std::unordered_map<std::string, std::string> form = {
            {"ethernet_led_gpio", "48"},
        };
        REQUIRE(ApiController::applyAdvancedSettingsForm(settings, stateMachine, form));
        REQUIRE(*settings.retrieveEthernetLedGpioPin() == GPIO_NUM_48);
    }

    SECTION("applyAdvancedSettingsForm rejects gpio numbers beyond the esp32s3 range") {
        const std::unordered_map<std::string, std::string> form = {
            {"ethernet_led_gpio", "49"},
        };
        REQUIRE(ApiController::applyAdvancedSettingsForm(settings, stateMachine, form));
        REQUIRE(*settings.retrieveEthernetLedGpioPin() == GPIO_NUM_NC);
    }

    SECTION("applyAdvancedSettingsForm requests a shutdown on success") {
        const std::unordered_map<std::string, std::string> form = {};
        REQUIRE(ApiController::applyAdvancedSettingsForm(settings, stateMachine, form));
        REQUIRE(stateMachine.getState() == STATE::SHUTTING_DOWN);
    }
}
