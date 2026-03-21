//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_ESP_ERR_H
#define LASERGATE_V2_ESP_ERR_H

// If compiling for host, define esp_err_t as int, otherwise include idf header
#ifdef HOST_BUILD
typedef int esp_err_t;
#define ESP_OK 0 /*!< esp_err_t value indicating success (no error) */
#else
#include "esp_err.h"
#endif

#endif //LASERGATE_V2_ESP_ERR_H