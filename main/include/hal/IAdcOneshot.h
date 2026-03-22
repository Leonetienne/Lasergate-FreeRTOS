//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_IGPIO_H
#define LASERGATE_V2_IGPIO_H

#include "compat/adc_channel_t.h"
#include "compat/esp_err_t.h"
#include <cstdint>

/**
 * Interface class to read analog pins.
 */
class IAdcOneshot {
public:
    virtual ~IAdcOneshot() = default;
    virtual esp_err_t registerChannel(adc_channel_t adcChannel) noexcept = 0;
    [[ nodiscard ]] virtual esp_err_t readChannel(adc_channel_t adcChannel, int& buffer) const noexcept = 0;
    [[ nodiscard ]] virtual bool isReady() const noexcept = 0;
};


#endif //LASERGATE_V2_IGPIO_H
