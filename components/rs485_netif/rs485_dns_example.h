#ifndef RS485_DNS_EXAMPLE_H
#define RS485_DNS_EXAMPLE_H

#include <stdint.h>
#include "esp_err.h"

esp_err_t rs485_dns_lookup_example(const char *hostname);
esp_err_t rs485_dns_get_server_info(void);

#endif
