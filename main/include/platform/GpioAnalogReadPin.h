//
// Created by nixmage on 3/22/26.
//

#ifndef LASERGATE_TESTS_GPIOANALOGREADPIN_H
#define LASERGATE_TESTS_GPIOANALOGREADPIN_H


class GpioAnalogReadPin {
public:
    GpioAnalogReadPin(GpioPinRegister& pinRegister, IGpio& gpio, gpio_num_t pinNum);
    GpioAnalogReadPin(const GpioAnalogReadPin&) = delete;
    GpioAnalogReadPin(GpioAnalogReadPin&& other) noexcept;
    ~GpioAnalogReadPin();
};


#endif //LASERGATE_TESTS_GPIOANALOGREADPIN_H