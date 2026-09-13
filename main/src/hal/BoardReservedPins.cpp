#include "hal/BoardReservedPins.h"

bool BoardReservedPins::isReserved(const gpio_num_t pin) noexcept {
    switch (pin) {
        // strapping pins, esp32-s3 datasheet boot configuration chapter
        case GPIO_NUM_0:
        case GPIO_NUM_3:
        case GPIO_NUM_45:
        case GPIO_NUM_46:
        // in-package spi flash (spics0/1, spiclk, spid, spiq, spihd, spiwp)
        case GPIO_NUM_26:
        case GPIO_NUM_27:
        case GPIO_NUM_28:
        case GPIO_NUM_29:
        case GPIO_NUM_30:
        case GPIO_NUM_31:
        case GPIO_NUM_32:
        // internally occupied on the waveshare esp32-s3-eth module regardless of psram config
        case GPIO_NUM_33:
        case GPIO_NUM_34:
        case GPIO_NUM_35:
        case GPIO_NUM_36:
        case GPIO_NUM_37:
        // native usb, used as secondary console/jtag (sdkconfig CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG)
        case GPIO_NUM_19:
        case GPIO_NUM_20:
        // uart0 console (sdkconfig CONFIG_ESP_CONSOLE_UART_NUM)
        case GPIO_NUM_43:
        case GPIO_NUM_44:
        // onboard w5500 spi bus, interrupt and reset, see EthernetManagerEsp32.h
        case GPIO_NUM_9:
        case GPIO_NUM_10:
        case GPIO_NUM_11:
        case GPIO_NUM_12:
        case GPIO_NUM_13:
        case GPIO_NUM_14:
        // not broken out to either header on this board
        case GPIO_NUM_4:
        case GPIO_NUM_5:
        case GPIO_NUM_6:
        case GPIO_NUM_7:
        case GPIO_NUM_8:
            return true;
        default:
            return false;
    }
}

bool BoardReservedPins::isInputOnly(const gpio_num_t pin) noexcept {
    // every esp32-s3 gpio pad has an output driver
    (void)pin;
    return false;
}
