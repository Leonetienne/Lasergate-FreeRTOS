//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_IGPIO_H
#define LASERGATE_V2_IGPIO_H

#include "compat/adc_channel_t.h"
#include "compat/esp_err_t.h"
#include <cstdint>

#include "compat/adc_unit_t.h"

/**
 * Interface class to read analog pins.
 */
class IAdcOneshot {
public:
    /**
     * @param adcUnit The adc unit this ADC driver wrapper should manage
     */
    IAdcOneshot(const adc_unit_t adcUnit) noexcept;
    IAdcOneshot(const IAdcOneshot&) = delete;
    IAdcOneshot(IAdcOneshot&& other) noexcept;
    virtual ~IAdcOneshot() = default;

    /**
     * Will prepare an adc channel to be used by this ADC driver wrapper
     */
    virtual esp_err_t registerChannel(const adc_channel_t adcChannel) noexcept = 0;

    /**
     * Will read the value at an ADC channel
     */
    [[ nodiscard ]] virtual esp_err_t readChannel(const adc_channel_t adcChannel, int& buffer) const noexcept = 0;

    /**
     * @returns whether this ADC driver wrapper is fully initialized and ready
     */
    [[ nodiscard ]] virtual bool isReady() const noexcept = 0;

    /**
     * @returns the ADC unit managed by this ADC driver wrapper
     */
    [[ nodiscard ]] adc_unit_t getAdcUnit() const noexcept { return adcUnit; };

protected:
    const adc_unit_t adcUnit;
    bool ready = false;
    [[nodiscard]] bool isAdcChannelOnCurrentUnit(const adc_channel_t adcChannel) const noexcept;
};


#endif //LASERGATE_V2_IGPIO_H
