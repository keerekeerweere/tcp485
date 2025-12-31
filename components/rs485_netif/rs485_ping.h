#ifndef RS485_PING_H
#define RS485_PING_H

#include <stdint.h>
#include "esp_err.h"

esp_err_t rs485_ping_test(const char *target_ip, uint32_t count);

#endif
