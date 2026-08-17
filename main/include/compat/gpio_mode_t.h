//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_GPIO_MODE_T_H
#define LASERGATE_V2_GPIO_MODE_T_H

// If compiling for host, define gpio_mode_t, otherwise include idf header
// This data type is just meant as a shim for the native idf datatype
#ifdef HOST_BUILD
typedef enum {
    GPIO_MODE_DISABLE,
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_OUTPUT_OD,
    GPIO_MODE_INPUT_OUTPUT_OD,
    GPIO_MODE_INPUT_OUTPUT
} gpio_mode_t;
#else
#include "hal/gpio_types.h"
#endif

#endif //LASERGATE_V2_GPIO_MODE_T_H
