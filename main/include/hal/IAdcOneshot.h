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
    IAdcOneshot(adc_unit_t adcUnit) noexcept;
    IAdcOneshot(const IAdcOneshot&) = delete;
    IAdcOneshot(IAdcOneshot&& other) noexcept;
    virtual ~IAdcOneshot() = default;
    virtual esp_err_t registerChannel(adc_channel_t adcChannel) noexcept = 0;
    [[ nodiscard ]] virtual esp_err_t readChannel(adc_channel_t adcChannel, int& buffer) const noexcept = 0;
    [[ nodiscard ]] virtual bool isReady() const noexcept = 0;

protected:
    const adc_unit_t adcUnit;
    bool ready = false;
    bool isAdcChannelOnCurrentUnit(adc_channel_t adcChannel) const noexcept;
};


#endif //LASERGATE_V2_IGPIO_H
