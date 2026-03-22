//
// Created by nixmage on 3/22/26.
//

#ifndef LASERGATE_TESTS_ADCANALOGREADPIN_H
#define LASERGATE_TESTS_ADCANALOGREADPIN_H

#include "GpioPinRegister.h"
#include "hal/IAdcOneshot.h"
#include "compat/gpio_num_t.h"
#include "compat/adc_channel_t.h"
#include "compat/adc_unit_t.h"
#include <expected>

class AdcAnalogReadPin {
public:
    AdcAnalogReadPin(GpioPinRegister& pinRegister, IAdcOneshot& adcOS, const gpio_num_t pinNum) noexcept;
    AdcAnalogReadPin(const AdcAnalogReadPin&) = delete;
    AdcAnalogReadPin(AdcAnalogReadPin&& other) noexcept;
    AdcAnalogReadPin& operator=(const AdcAnalogReadPin&) = delete;
    ~AdcAnalogReadPin();

    /**
    * Will initialize the pin
    * @return Success state
    */
    esp_err_t initialize() noexcept;

    /**
     * Will unbind the pin and defunc this object
     *
     * @return Success state
     */
    bool free() noexcept;

    /**
     * @return Whether this object is ready to be used
     */
    [[nodiscard]] bool isReady() const noexcept { return ready; };

    /**
     * @return The gpio pin
     */
    [[nodiscard]] gpio_num_t getGpioNum() const noexcept { return gpioPinNum; };

    /**
     * @return The current value at the pin
     */
    [[nodiscard]] std::expected<int, esp_err_t> read() const noexcept;

private:
    GpioPinRegister& pinRegister;
    IAdcOneshot& adcOneshot;
    const gpio_num_t gpioPinNum;
    adc_unit_t adcUnit = ADC_UNIT_1;
    adc_channel_t adcChannel = ADC_CHANNEL_0;
    bool ready;
};


#endif //LASERGATE_TESTS_ADCANALOGREADPIN_H