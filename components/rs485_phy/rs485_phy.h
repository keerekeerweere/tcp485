#ifndef RS485_PHY_H
#define RS485_PHY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

#define RS485_UART_NUM UART_NUM_2
#define RS485_UART_BUF_SIZE 2048

#define MAX_RETRIES 16
#define JAM_BITS 32
#define DEFAULT_BAUD_RATE 19200
#define DEFAULT_DISTANCE_M 100

#define DEFAULT_BAUD_BIT_TIME_US 52
#define DEFAULT_SLOT_TIME_MS 51
#define DEFAULT_INTER_FRAME_GAP_MS 5
#define DEFAULT_INTER_FRAME_GAP_US 520

#define BIT_TIME_US(baud) ((uint32_t)(8000000ULL / (baud_rate))
#define SLOT_TIME_MS(baud) (BIT_TIME_US(baud) * 16) / 1000)
#define INTER_FRAME_GAP_MS(baud) (INTER_FRAME_GAP_US(baud) / 1000)

typedef struct {
    uart_port_t uart_num;
    int tx_pin;
    int rx_pin;
    int rts_pin;
    uint32_t baud_rate;
    uint32_t distance_m;
    uint32_t slot_time_ms;
    uint32_t inter_frame_gap_ms;
    uint32_t bit_time_us;
    bool initialized;
} rs485_phy_config_t;

typedef struct {
    uint32_t baud_rate;
    uint32_t distance_m;
    uint32_t slot_time_ms;
    uint32_t inter_frame_gap_ms;
    uint32_t bit_time_us;
    bool initialized;
} rs485_timing_t;

struct rs485_phy {
    uart_port_t uart_num;
    int tx_pin;
    int rx_pin;
    int rts_pin;
    uint32_t baud_rate;
    uint32_t distance_m;
    uint32_t slot_time_ms;
    uint32_t inter_frame_gap_ms;
    uint32_t bit_time_us;
    bool initialized;
};

typedef struct rs485_phy rs485_phy_t;

void rs485_calculate_timing(uint32_t baud_rate, uint32_t distance_m, rs485_timing_t *timing);
void rs485_apply_timing(const rs485_timing_t *timing);

esp_err_t rs485_phy_init(rs485_phy_t *phy);
esp_err_t rs485_phy_deinit(rs485_phy_t *phy);
bool rs485_phy_is_busy(rs485_phy_t *phy);
esp_err_t rs485_phy_set_timing(rs485_timing_t *timing);
esp_err_t rs485_phy_transmit(rs485_phy_t *phy, const uint8_t *data, size_t len);
int rs485_phy_receive(rs485_phy_t *phy, uint8_t *buf, size_t max_len, TickType_t timeout);

#endif
