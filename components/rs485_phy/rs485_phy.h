#ifndef RS485_PHY_H
#define RS485_PHY_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"

#define RS485_UART_NUM UART_NUM_2
#define RS485_UART_BUF_SIZE 2048

typedef struct {
    uart_port_t uart_num;
    int tx_pin;
    int rx_pin;
    int rts_pin;
    uint32_t baud_rate;
    bool initialized;
} rs485_phy_t;

esp_err_t rs485_phy_init(rs485_phy_t *phy);
esp_err_t rs485_phy_transmit(rs485_phy_t *phy, const uint8_t *data, size_t len);
int rs485_phy_receive(rs485_phy_t *phy, uint8_t *buf, size_t max_len, TickType_t timeout);
bool rs485_phy_is_busy(rs485_phy_t *phy);
esp_err_t rs485_phy_deinit(rs485_phy_t *phy);

#endif
