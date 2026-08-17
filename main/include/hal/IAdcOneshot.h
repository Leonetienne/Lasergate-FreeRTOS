//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_IADCONESHOT_H
#define LASERGATE_V2_IADCONESHOT_H

#include "compat/adc_channel_t.h"
#include "compat/esp_err_t.h"
#include <expected>

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

    virtual esp_err_t initialize() noexcept = 0;

    /**
     * Will prepare an adc channel to be used by this ADC driver wrapper
     */
    virtual esp_err_t registerChannel(const adc_channel_t adcChannel) noexcept = 0;

    /**
     * Will read the value at an ADC channel
     * @returns the retrieved value or an error
     */
    [[ nodiscard ]] virtual std::expected<int, esp_err_t> readChannel(const adc_channel_t adcChannel) const noexcept = 0;

    /**
     * @returns whether this ADC driver wrapper is fully initialized and ready
     */
    [[ nodiscard ]] bool isReady() const noexcept { return ready; };

    /**
     * @returns the ADC unit managed by this ADC driver wrapper
     */
    [[ nodiscard ]] adc_unit_t getAdcUnit() const noexcept { return adcUnit; };

protected:
    const adc_unit_t adcUnit;
    bool ready = false;
    [[nodiscard]] bool isAdcChannelOnCurrentUnit(const adc_channel_t adcChannel) const noexcept;
};


#endif //LASERGATE_V2_IADCONESHOT_H
