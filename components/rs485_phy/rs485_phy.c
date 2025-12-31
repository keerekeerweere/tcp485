#include "rs485_phy.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "RS485_PHY";

esp_err_t rs485_phy_init(rs485_phy_t *phy)
{
    if (phy == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uart_config_t uart_config = {
        .baud_rate = phy->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    ESP_ERROR_CHECK(uart_driver_install(phy->uart_num, RS485_UART_BUF_SIZE, RS485_UART_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(phy->uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(phy->uart_num, phy->tx_pin, phy->rx_pin, phy->rts_pin, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(phy->uart_num, UART_MODE_RS485_HALF_DUPLEX));
    
    ESP_LOGI(TAG, "RS485 PHY initialized: UART%d, TX=%d, RX=%d, RTS=%d, %d baud", 
             phy->uart_num, phy->tx_pin, phy->rx_pin, phy->rts_pin, phy->baud_rate);

    phy->initialized = true;
    return ESP_OK;
}

esp_err_t rs485_phy_transmit(rs485_phy_t *phy, const uint8_t *data, size_t len)
{
    if (phy == NULL || !phy->initialized || data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    int written = uart_write_bytes(phy->uart_num, (const char *)data, len);
    if (written != len) {
        ESP_LOGE(TAG, "TX error: expected %d bytes, wrote %d", len, written);
        return ESP_FAIL;
    }

    uart_wait_tx_done(phy->uart_num, pdMS_TO_TICKS(100));
    return ESP_OK;
}

int rs485_phy_receive(rs485_phy_t *phy, uint8_t *buf, size_t max_len, TickType_t timeout)
{
    if (phy == NULL || !phy->initialized || buf == NULL || max_len == 0) {
        return -1;
    }

    int len = uart_read_bytes(phy->uart_num, buf, max_len, timeout);
    return len;
}

bool rs485_phy_is_busy(rs485_phy_t *phy)
{
    if (phy == NULL || !phy->initialized) {
        return false;
    }

    size_t avail = 0;
    uart_get_buffered_data_len(phy->uart_num, &avail);
    return avail > 0;
}

esp_err_t rs485_phy_deinit(rs485_phy_t *phy)
{
    if (phy == NULL || !phy->initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    uart_driver_delete(phy->uart_num);
    phy->initialized = false;
    ESP_LOGI(TAG, "RS485 PHY deinitialized");
    return ESP_OK;
}
