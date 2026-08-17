#ifndef LASERGATE_V2_ESP_LOG_MACROS_H
#define LASERGATE_V2_ESP_LOG_MACROS_H

// If compiling for host, logging is a no-op, otherwise include idf header
#ifdef HOST_BUILD
#define ESP_LOGE(tag, format, ...) ((void)0)
#define ESP_LOGW(tag, format, ...) ((void)0)
#define ESP_LOGI(tag, format, ...) ((void)0)
#define ESP_LOGD(tag, format, ...) ((void)0)
#define ESP_LOGV(tag, format, ...) ((void)0)
#else
#include "esp_log.h"
#endif

#endif //LASERGATE_V2_ESP_LOG_MACROS_H
