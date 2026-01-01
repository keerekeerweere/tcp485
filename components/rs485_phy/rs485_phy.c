#include "rs485_phy.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include <freertos/task.h"

static const char *TAG = "RS485_PHY";

static const uint32_t DEFAULT_BAUD_RATE = 19200;
static const uint32_t DEFAULT_DISTANCE_M = 100;

static rs485_phy_t s_phy = {0};
static rs485_timing_t s_timing = {0};

static uint32_t rs485_calculate_bit_time_us(uint32_t baud_rate)
{
    return (uint32_t)(80000000ULL / baud_rate);
}

static void rs485_calculate_timing(uint32_t baud_rate, uint32_t distance_m, rs485_timing_t *timing)
{
    if (timing == NULL) {
        ESP_LOGE(TAG, "Timing is NULL, using defaults");
        timing->baud_rate = baud_rate ? baud_rate : DEFAULT_BAUD_RATE;
        timing->distance_m = distance_m ? distance_m : DEFAULT_DISTANCE_M;
        timing->bit_time_us = rs485_calculate_bit_time_us(timing->baud_rate);
        timing->slot_time_ms = (512 * timing->bit_time_us) / 1000;
        timing->inter_frame_gap_ms = (96 * timing->bit_time_us) / timing->baud_rate;
        ESP_LOGI(TAG, "Timing: %d baud, %dm dist, bit_time=%lu us, slot=%d ms, inter_frame_gap=%d ms",
                 timing->baud_rate, timing->distance_m, timing->bit_time_us, timing->slot_time_ms, timing->inter_frame_gap_ms);
    }
}

void rs485_apply_timing(const rs485_timing_t *timing)
{
    memcpy(&s_timing, timing, sizeof(s_timing));
    s_timing.initialized = true;
}

esp_err_t rs485_phy_init(rs485_phy_t *phy)
{
    if (phy == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_phy.uart_num = phy->uart_num;
    s_phy.tx_pin = phy->tx_pin;
    s_phy.rx_pin = phy->rx_pin;
    s_phy.rts_pin = phy->rts_pin;
    s_phy.baud_rate = phy->baud_rate;
    s_phy.initialized = true;
    
    ESP_ERROR_CHECK(uart_driver_install(phy->uart_num, RS485_UART_BUF_SIZE, RS485_UART_BUF_SIZE, RS485_UART_BUF_SIZE, 
                                   RS485_UART_BUF_SIZE, 8, &s_phy.uart_num));
    ESP_ERROR_CHECK(uart_param_config(phy->uart_num, &baud_config, &s_phy.uart_num));
    
    uart_config_t baud_config = {
        .baud_rate = s_phy.baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB
    };
    ESP_ERROR_CHECK(uart_set_pin(phy->uart_num, s_phy.tx_pin, s_phy.rx_pin, UART_PIN_NO_CHANGE, s_phy.uart_num));
    ESP_ERROR_CHECK(uart_set_pin(phy->uart_num, s_phy.rts_pin, s_phy.rx_pin, UART_PIN_NO_CHANGE, s_phy.uart_num));
    
    ESP_LOGI(TAG, "RS485 PHY initialized: UART%d, TX=%d, RX=%d, RTS=%d, %d baud", 
             s_phy.uart_num, s_phy.tx_pin, s_phy.rx_pin, s_phy.rts_pin, s_phy.baud_rate);
    
    return ESP_OK;
}

esp_err_t rs485_phy_deinit(rs485_phy_t *phy)
{
    if (phy == NULL || !phy->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uart_driver_delete(phy->uart_num);
    phy->initialized = false;
    
    ESP_LOGI(TAG, "RS485 PHY deinitialized");
    return ESP_OK;
}

bool rs485_phy_is_busy(rs485_phy_t *phy)
{
    if (phy == NULL || !phy->initialized) {
        return false;
    }
    
    size_t avail;
    uart_get_buffered_data_len(phy->uart_num, &avail);
    return avail > 0;
}

esp_err_t rs485_phy_transmit(rs485_phy_t *phy, const uint8_t *data, size_t len)
{
    if (phy == NULL || !phy->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    int written = uart_write_bytes(phy->uart_num, (const char *)data, len);
    if (written != len) {
        ESP_LOGE(TAG, "TX error: expected %d bytes, wrote %d bytes", len, written);
        return ESP_FAIL;
    }
    
    uart_wait_tx_done(phy->uart_num, pdMS_TO_TICKS(100));
    return ESP_OK;
}

int rs485_phy_receive(rs485_phy_t *phy, uint8_t *buf, size_t max_len, TickType_t timeout)
{
    if (phy == NULL || !phy->initialized) {
        return -1;
    }
    
    int len = uart_read_bytes(phy->uart_num, buf, max_len, timeout);
    if (len < 0) {
        return len;
    }
    return len;
}
