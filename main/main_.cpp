#include <algorithm>

#include "freertos/FREERTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_random.h"
#include <array>

constexpr gpio_num_t BLINK_GPIO = GPIO_NUM_13;
constexpr gpio_num_t LASER_CONTROL_GPIO = GPIO_NUM_12;
constexpr gpio_num_t BUZZER_GPIO = GPIO_NUM_14;
// GPIO34 is adc channel 6 on classic esp32
// https://randomnerdtutorials.com/esp-idf-esp32-gpio-analog-adc/
constexpr adc_channel_t ADC_LDR_CHANNEL = ADC_CHANNEL_6;

constexpr uint32_t LASER_PULSE_FREQ = 100; // HZ
constexpr uint32_t LASER_PULSE_DELAY = 1000 / LASER_PULSE_FREQ; // Helper const
constexpr int LDR_LASER_THRESH_OFF = 3200; // int, since that's the type its being compared to returned by adc_oneshot_read()
constexpr std::size_t LASER_PULSE_HIST_SIZE = 20; // How many last pulse status are kept and checked if any was off (to compensate for lasers being off and being blocked would still produce a valid outcome)

void init_gpio_toggles() {
    gpio_reset_pin(BLINK_GPIO);
    gpio_reset_pin(LASER_CONTROL_GPIO);
    gpio_reset_pin(BUZZER_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LASER_CONTROL_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
}

void init_ldrs(adc_oneshot_unit_handle_t& adc1) {
    adc_oneshot_unit_init_cfg_t adc1_init_cfg = {};
    adc1_init_cfg.unit_id = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_init_cfg, &adc1));
    adc_oneshot_chan_cfg_t adc1_chan_cfg = {};
    adc1_chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc1_chan_cfg.atten = ADC_ATTEN_DB_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1, ADC_LDR_CHANNEL, &adc1_chan_cfg));
}

void set_led_status(const bool status) {
    gpio_set_level(BLINK_GPIO, (int)status);
}

void set_laser_status(const bool status) {
    gpio_set_level(LASER_CONTROL_GPIO, (int)status);
}

void set_buzzer_status(const bool status) {
    gpio_set_level(BUZZER_GPIO, (int)status);
}

int read_ldr(adc_oneshot_unit_handle_t& adc1) {
    int ldrRaw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1, ADC_LDR_CHANNEL, &ldrRaw));
    return ldrRaw;
}

bool do_pulse(adc_oneshot_unit_handle_t& adc1) {
    // Decide on laser status
    const bool desiredLaserStatus = esp_random() % 2;

    // Set laser status
    set_laser_status(desiredLaserStatus);

    // Wait for LDR to collect photons
    vTaskDelay(pdMS_TO_TICKS(LASER_PULSE_DELAY));

    // Check LDR status
    int ldrValue = read_ldr(adc1);
    ESP_LOGI("ADC", "raw=%d", ldrValue);
    const bool ldrIndicatedLaserStatus = ldrValue >= LDR_LASER_THRESH_OFF;

    // Does the LDR indicated laser status match the actual status?
    // If not, there's a problem, return false
    return ldrIndicatedLaserStatus == desiredLaserStatus;
}

extern "C" void app_main() {
    init_gpio_toggles();

    adc_oneshot_unit_handle_t adc1;
    init_ldrs(adc1);

    // Assume the first few pulses are all GOOD
    std::array<bool, LASER_PULSE_HIST_SIZE> last_beam_pulse_status{};
    last_beam_pulse_status.fill(true);
    std::size_t i_last_beam_status = 0;

    while (1) {
        const bool beamIntact = do_pulse(adc1);
        last_beam_pulse_status[i_last_beam_status] = beamIntact;
        i_last_beam_status = (i_last_beam_status + 1) % last_beam_pulse_status.size();

        // Was any beam of the last n pulses broken?
        const bool anyBroken = std::ranges::any_of(last_beam_pulse_status, [](const bool status) { return !status; });

        // Activate status led if the beam is broken
        set_led_status(anyBroken);
        set_buzzer_status(anyBroken);
    }
}
