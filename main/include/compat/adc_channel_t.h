//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_ADC_CHANNEL_H
#define LASERGATE_V2_ADC_CHANNEL_H

#ifdef HOST_BUILD
typedef enum {
    ADC_CHANNEL_0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
    ADC_CHANNEL_8,
    ADC_CHANNEL_9,
    ADC_CHANNEL_10
} adc_channel_t;
#else
#include "esp_adc/adc_oneshot.h"
#endif

#endif //LASERGATE_V2_ADC_CHANNEL_H