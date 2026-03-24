//
// Created by Leon Etienne on 22.03.26.
//

#ifndef LASERGATE_V2_ADCONESHOTSTUB_H
#define LASERGATE_V2_ADCONESHOTSTUB_H

#include <map>

#include "hal/IAdcOneshot.h"
#include "compat/adc_unit_t.h"
#include <unordered_map>
#include <unordered_set>

class AdcOneshotStub : public IAdcOneshot {
public:
    /**
     * @param adcUnit The adc unit this ADC driver wrapper should manage
     */
    AdcOneshotStub(const adc_unit_t adcUnit) noexcept;
    AdcOneshotStub(const AdcOneshotStub&) = delete;
    AdcOneshotStub(AdcOneshotStub&& other) noexcept;
    AdcOneshotStub& operator=(const AdcOneshotStub&) = delete;
    ~AdcOneshotStub() override = default;

    /**
     * Will prepare an adc channel to be used by this ADC driver wrapper
     */
    esp_err_t registerChannel(adc_channel_t adcChannel) noexcept override;

    /**
    * Will read the value at an ADC channel
    * @returns the retrieved value or an error
    */
    [[ nodiscard ]] std::expected<int, esp_err_t> readChannel(adc_channel_t adcChannel) const noexcept override;

    /**
     * Will set a specific value to be present at a specific channel for testing purposes
     */
    void test_setChannelValue(adc_channel_t adc_channel, int value) noexcept;

private:
    std::unordered_set<adc_channel_t> registeredChannels;
    std::unordered_map<adc_channel_t, int> inputValueMap;
};


#endif //LASERGATE_V2_ADCONESHOTSTUB_H