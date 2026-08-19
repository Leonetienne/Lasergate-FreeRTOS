#include "platform/EthernetManagerEsp32.h"

#include <esp_log.h>
#include "esp_netif.h"
#include "esp_mac.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_eth_mac_w5500.h"
#include "esp_eth_phy_w5500.h"

static const char* LOG_TAG = "EthernetManagerEsp32";

EthernetManagerEsp32::EthernetManagerEsp32(
    gpio_num_t indicatorGpioPin,
    IGpio& i_gpio,
    GpioPinRegister& pinRegister,
    const ITime& i_time
) noexcept :
    i_gpio(i_gpio),
    pinRegister(pinRegister),
    i_time(i_time),
    indicatorPin(pinRegister, i_gpio, indicatorGpioPin)
{
    if (indicatorPin.getGpioNum() != GPIO_NUM_NC) {
        indicatorPin.initialize();
        indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
    }
}

EthernetManagerEsp32::~EthernetManagerEsp32() noexcept {
    if (isInitialized) {
        EthernetManagerEsp32::free();
    }
}

bool EthernetManagerEsp32::begin() noexcept {
    if (isInitialized) {
        return false;
    }

    if (esp_netif_init() != ESP_OK) {
        return false;
    }
    if (esp_event_loop_create_default() != ESP_OK) {
        return false;
    }

    esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_ETH();
    netif = esp_netif_new(&netifConfig);
    if (netif == nullptr) {
        return false;
    }

    const esp_err_t isrServiceResult = gpio_install_isr_service(0);
    if (isrServiceResult != ESP_OK && isrServiceResult != ESP_ERR_INVALID_STATE) {
        return false;
    }

    spi_bus_config_t busConfig = {};
    busConfig.mosi_io_num = SPI_MOSI_GPIO;
    busConfig.miso_io_num = SPI_MISO_GPIO;
    busConfig.sclk_io_num = SPI_SCLK_GPIO;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    if (spi_bus_initialize(SPI2_HOST, &busConfig, SPI_DMA_CH_AUTO) != ESP_OK) {
        return false;
    }

    spi_device_interface_config_t spiDeviceConfig = {};
    spiDeviceConfig.mode = 0;
    spiDeviceConfig.clock_speed_hz = SPI_CLOCK_MHZ * 1000 * 1000;
    spiDeviceConfig.queue_size = 20;
    spiDeviceConfig.spics_io_num = SPI_CS_GPIO;

    eth_w5500_config_t w5500Config = ETH_W5500_DEFAULT_CONFIG(SPI2_HOST, &spiDeviceConfig);
    w5500Config.base.int_gpio_num = SPI_INT_GPIO;
    w5500Config.base.poll_period_ms = 0;

    eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
    mac = esp_eth_mac_new_w5500(&w5500Config, &macConfig);
    if (mac == nullptr) {
        return false;
    }

    eth_phy_config_t phyConfig = ETH_PHY_DEFAULT_CONFIG();
    phyConfig.phy_addr = PHY_ADDR;
    phyConfig.reset_gpio_num = PHY_RESET_GPIO;
    phy = esp_eth_phy_new_w5500(&phyConfig);
    if (phy == nullptr) {
        mac->del(mac);
        mac = nullptr;
        return false;
    }

    esp_eth_config_t ethConfig = ETH_DEFAULT_CONFIG(mac, phy);
    if (esp_eth_driver_install(&ethConfig, &ethHandle) != ESP_OK) {
        mac->del(mac);
        mac = nullptr;
        phy->del(phy);
        phy = nullptr;
        return false;
    }

    // the w5500 has no burned-in factory MAC address
    uint8_t macAddr[6] = {};
    esp_read_mac(macAddr, ESP_MAC_ETH);
    if (esp_eth_ioctl(ethHandle, ETH_CMD_S_MAC_ADDR, macAddr) != ESP_OK) {
        return false;
    }

    netifGlue = esp_eth_new_netif_glue(ethHandle);
    if (netifGlue == nullptr) {
        return false;
    }
    if (esp_netif_attach(netif, netifGlue) != ESP_OK) {
        return false;
    }

    if (esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &EthernetManagerEsp32::eventHandler, this) != ESP_OK) {
        return false;
    }
    if (esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &EthernetManagerEsp32::eventHandler, this) != ESP_OK) {
        return false;
    }
    if (esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_LOST_IP, &EthernetManagerEsp32::eventHandler, this) != ESP_OK) {
        return false;
    }
    eventHandlersRegistered = true;

    if (esp_eth_start(ethHandle) != ESP_OK) {
        return false;
    }

    state = EthernetConnectionState::Connecting;
    isInitialized = true;
    return true;
}

bool EthernetManagerEsp32::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    bool success = true;

    if (esp_eth_stop(ethHandle) != ESP_OK) {
        success = false;
    }

    if (eventHandlersRegistered) {
        if (esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, &EthernetManagerEsp32::eventHandler) != ESP_OK) {
            success = false;
        }
        if (esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, &EthernetManagerEsp32::eventHandler) != ESP_OK) {
            success = false;
        }
        if (esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_LOST_IP, &EthernetManagerEsp32::eventHandler) != ESP_OK) {
            success = false;
        }
    }

    if (netifGlue != nullptr) {
        if (esp_eth_del_netif_glue(netifGlue) != ESP_OK) {
            success = false;
        }
        netifGlue = nullptr;
    }

    if (esp_eth_driver_uninstall(ethHandle) != ESP_OK) {
        success = false;
    }
    ethHandle = nullptr;

    if (mac != nullptr) {
        mac->del(mac);
        mac = nullptr;
    }
    if (phy != nullptr) {
        phy->del(phy);
        phy = nullptr;
    }

    if (netif != nullptr) {
        esp_netif_destroy(netif);
        netif = nullptr;
    }

    if (esp_event_loop_delete_default() != ESP_OK) {
        success = false;
    }

    isInitialized = false;
    eventHandlersRegistered = false;
    state = EthernetConnectionState::Disconnected;
    setIndicatorState(PIN_STATE_DIGITAL::LOW);

    return success;
}

EthernetConnectionState EthernetManagerEsp32::getState() const noexcept {
    return state;
}

void EthernetManagerEsp32::setOnConnected(std::function<void()> callback) noexcept {
    onConnected = std::move(callback);
}

void EthernetManagerEsp32::setOnDisconnected(std::function<void()> callback) noexcept {
    onDisconnected = std::move(callback);
}

void EthernetManagerEsp32::setIndicatorState(PIN_STATE_DIGITAL pinState) noexcept {
    if (indicatorPin.isReady()) {
        indicatorPin.setState(pinState);
    }
}

void EthernetManagerEsp32::eventHandler(
    void* arg,
    esp_event_base_t base,
    int32_t id,
    void* data
) noexcept {
    auto* self = static_cast<EthernetManagerEsp32*>(arg);

    if (base == ETH_EVENT && id == ETHERNET_EVENT_CONNECTED) {
        ESP_LOGI(LOG_TAG, "link up");
        if (self->state != EthernetConnectionState::Connected) {
            self->state = EthernetConnectionState::Connecting;
        }

    } else if (base == ETH_EVENT && id == ETHERNET_EVENT_DISCONNECTED) {
        ESP_LOGW(LOG_TAG, "link down");
        const bool wasConnected = self->state == EthernetConnectionState::Connected;
        self->state = EthernetConnectionState::Connecting;
        self->setIndicatorState(PIN_STATE_DIGITAL::LOW);
        if (wasConnected && self->onDisconnected) {
            self->onDisconnected();
        }

    } else if (base == IP_EVENT && id == IP_EVENT_ETH_GOT_IP) {
        self->state = EthernetConnectionState::Connected;
        self->setIndicatorState(PIN_STATE_DIGITAL::HIGH);
        const auto* event = static_cast<ip_event_got_ip_t*>(data);
        ESP_LOGI(LOG_TAG, "connected, got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        if (self->onConnected) {
            self->onConnected();
        }

    } else if (base == IP_EVENT && id == IP_EVENT_ETH_LOST_IP) {
        ESP_LOGW(LOG_TAG, "lost ip");
        const bool wasConnected = self->state == EthernetConnectionState::Connected;
        self->state = EthernetConnectionState::Connecting;
        self->setIndicatorState(PIN_STATE_DIGITAL::LOW);
        if (wasConnected && self->onDisconnected) {
            self->onDisconnected();
        }
    }
}
