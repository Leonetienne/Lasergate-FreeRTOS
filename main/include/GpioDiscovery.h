#ifndef LASERGATE_V2_GPIODISCOVERY_H
#define LASERGATE_V2_GPIODISCOVERY_H

#include "GpioPinRegister.h"
#include "hal/IAdcOneshot.h"
#include "hal/IGpio.h"
#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

/**
 * Hardware probing for freshly assembled boards: reads raw adc1/adc2 values and lets every
 * unreserved gpio pin be individually driven high/low, so wiring can be identified by
 * observation (which led lights up, which ldr reacts). Pins bound in the GpioPinRegister
 * are already outputs, so begin() drives them low as they are and end() drives them low again.
 * Only ever called from the main thread, same as Gate.
 */
class GpioDiscovery {
public:
    /// esp32-s3: adc1 and adc2 each expose 10 channels
    static constexpr std::size_t MAX_PINS = 20;

    /// filled from index 0, unused slots are empty
    using PinArray = std::array<std::optional<gpio_num_t>, MAX_PINS>;
    using AdcReadingArray = std::array<std::optional<std::pair<gpio_num_t, uint16_t>>, MAX_PINS>;

    GpioDiscovery(
        const GpioPinRegister& pinRegister,
        IGpio& i_gpio,
        IAdcOneshot& i_adcOneshot1,
        IAdcOneshot& i_adcOneshot2
    ) noexcept;
    GpioDiscovery(const GpioDiscovery&) = delete;
    GpioDiscovery& operator=(const GpioDiscovery&) = delete;

    /**
     * Registers adc1/adc2 channels and drives every free output pin low, configuring unbound pins as outputs first.
     * Safe to call repeatedly.
     */
    void begin() noexcept;

    /**
     * Resets every pin begin() configured and drives the bound pins it borrowed low again.
     */
    void end() noexcept;

    /**
     * @return raw adc1 reading per free adc1-capable pin, in getAdc1Pins() order.
     * A pin is left out if reading it failed.
     */
    [[nodiscard]] AdcReadingArray readAdc1() const noexcept;

    /**
     * @return raw adc2 reading per free adc2-capable pin, in getAdc2Pins() order.
     * A pin is left out if reading it failed.
     */
    [[nodiscard]] AdcReadingArray readAdc2() const noexcept;

    /**
     * Drives a free output pin high or low.
     * @param pin The pin to drive
     * @param high True for high, false for low
     * @return false if pin isn't a free output pin (reserved/adc1/adc2/input-only/unknown), or begin() wasn't called
     */
    bool setPinLevel(gpio_num_t pin, bool high) noexcept;

    /**
     * @param pin The pin to query
     * @return the level this instance last drove the given pin to, or std::nullopt if it isn't a free output pin
     */
    [[nodiscard]] std::optional<bool> getPinLevel(gpio_num_t pin) const noexcept;

    /**
     * @return the output pins begin() configured, in ascending gpio order
     */
    [[nodiscard]] PinArray getDrivenPins() const noexcept;

    /**
     * @return every free, adc1-capable pin (not reserved), in ascending gpio order
     */
    [[nodiscard]] static const PinArray& getAdc1Pins() noexcept;

    /**
     * @return every free, adc2-capable pin (not reserved), in ascending gpio order
     */
    [[nodiscard]] static const PinArray& getAdc2Pins() noexcept;

    /**
     * @return every free, output-toggleable pin in ascending gpio order, excluding reserved, adc1/adc2-capable and input-only pins
     */
    [[nodiscard]] static const PinArray& getFreeOutputPins() noexcept;

private:
    const GpioPinRegister& pinRegister;
    IGpio& i_gpio;
    IAdcOneshot& i_adcOneshot1;
    IAdcOneshot& i_adcOneshot2;
    std::unordered_map<gpio_num_t, bool> outputLevels;
    std::unordered_set<gpio_num_t> borrowedPins;
};

#endif //LASERGATE_V2_GPIODISCOVERY_H
