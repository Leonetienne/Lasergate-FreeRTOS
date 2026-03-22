//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_ADC_UNIT_H
#define LASERGATE_V2_ADC_UNIT_H

#ifdef HOST_BUILD
typedef enum {
    ADC_UNIT_1,
    ADC_UNIT_2
} adc_unit_t;
#else
#include "esp_adc/adc_types.h"
#endif

#endif //LASERGATE_V2_ADC_UNIT_H