#ifndef LASERGATE_TESTS_ADCONESHOT_H
#define LASERGATE_TESTS_ADCONESHOT_H
#include <unordered_set>

#include "hal/IAdcOneshot.h"
#include "esp_adc/adc_oneshot.h"

class AdcOneshot : public IAdcOneshot {
public:
    /**
     * @param adcUnit The adc unit this ADC driver wrapper should manage
     */
    AdcOneshot(const adc_unit_t adcUnit) noexcept;
    AdcOneshot(const AdcOneshot&) = delete;
    AdcOneshot(AdcOneshot&& other) noexcept;
    AdcOneshot& operator=(const AdcOneshot&) = delete;
    ~AdcOneshot() override = default;

    esp_err_t initialize() noexcept override;

    /**
     * Will prepare an adc channel to be used by this ADC driver wrapper
     */
    esp_err_t registerChannel(adc_channel_t adcChannel) noexcept override;

    /**
    * Will read the value at an ADC channel
    * @returns the retrieved value or an error
    */
    [[ nodiscard ]] std::expected<int, esp_err_t> readChannel(adc_channel_t adcChannel) const noexcept override;

private:
    adc_oneshot_unit_handle_t adcHandle;
    std::unordered_set<adc_channel_t> readyChannels;
};


#endif //LASERGATE_TESTS_ADCONESHOT_H
