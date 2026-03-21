//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_GPIO_NUM_T_H
#define LASERGATE_V2_GPIO_NUM_T_H

// If compiling for host, define gpio_num_t as int, otherwise include idf header
#ifdef HOST_BUILD
typedef int gpio_num_t;
#else
#include "soc/gpio_num.h"
#endif

#endif //LASERGATE_V2_GPIO_NUM_T_H