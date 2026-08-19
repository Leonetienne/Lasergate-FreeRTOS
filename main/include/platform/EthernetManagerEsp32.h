#ifndef LASERGATE_V2_ETHERNETMANAGERESP32_H
#define LASERGATE_V2_ETHERNETMANAGERESP32_H

#include "hal/IEthernetManager.h"
#include "hal/IGpio.h"
#include "hal/ITime.h"
#include "GpioPinRegister.h"
#include "platform/GpioDigitalWritePin.h"
#include "esp_event.h"
#include "esp_netif_types.h"
#include "esp_eth_driver.h"
#include "esp_eth_netif_glue.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"

/**
 * Esp32-S3 implementation, driving a W5500 SPI ethernet PHY
 */
class EthernetManagerEsp32 : public IEthernetManager {
public:
    EthernetManagerEsp32(gpio_num_t indicatorGpioPin, IGpio& i_gpio, GpioPinRegister& pinRegister, const ITime& i_time) noexcept;
    EthernetManagerEsp32(const EthernetManagerEsp32&) = delete;
    EthernetManagerEsp32& operator=(const EthernetManagerEsp32&) = delete;
    EthernetManagerEsp32(EthernetManagerEsp32&&) = delete;
    EthernetManagerEsp32& operator=(EthernetManagerEsp32&&) = delete;
    ~EthernetManagerEsp32() noexcept override;

    /**
     * Brings up the W5500 ethernet interface and starts DHCP
     * @return Success state
     */
    bool begin() noexcept override;

    /**
     * Stops the interface and releases the resources acquired by begin()
     * @return Success state
     */
    bool free() noexcept override;

    [[nodiscard]] EthernetConnectionState getState() const noexcept override;

    void setOnConnected(std::function<void()> callback) noexcept override;
    void setOnDisconnected(std::function<void()> callback) noexcept override;

private:
    /**
     * Static esp-idf event callback for ETH_EVENT and IP_EVENT
     */
    static void eventHandler(void* arg, esp_event_base_t base, int32_t id, void* data) noexcept;

    void setIndicatorState(PIN_STATE_DIGITAL pinState) noexcept;

    // Fixed SPI wiring of the Waveshare ESP32-S3-ETH board's onboard W5500
    static constexpr int SPI_SCLK_GPIO = 13;
    static constexpr int SPI_MOSI_GPIO = 11;
    static constexpr int SPI_MISO_GPIO = 12;
    static constexpr int SPI_CS_GPIO = 14;
    static constexpr int SPI_INT_GPIO = 10;
    static constexpr int PHY_RESET_GPIO = 9;
    static constexpr int SPI_CLOCK_MHZ = 25;
    static constexpr uint8_t PHY_ADDR = 1;

    IGpio& i_gpio;
    GpioPinRegister& pinRegister;
    const ITime& i_time;

    bool eventHandlersRegistered = false;
    EthernetConnectionState state = EthernetConnectionState::Disconnected;
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;

    esp_netif_t* netif = nullptr;
    esp_eth_handle_t ethHandle = nullptr;
    esp_eth_netif_glue_handle_t netifGlue = nullptr;
    esp_eth_mac_t* mac = nullptr;
    esp_eth_phy_t* phy = nullptr;

    GpioDigitalWritePin indicatorPin;
};

#endif //LASERGATE_V2_ETHERNETMANAGERESP32_H
