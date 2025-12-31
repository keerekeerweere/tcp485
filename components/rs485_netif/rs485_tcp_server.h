#ifndef RS485_TCP_SERVER_H
#define RS485_TCP_SERVER_H

#include <stdint.h>
#include "esp_err.h"

esp_err_t rs485_tcp_server_start(uint16_t port);

#endif
