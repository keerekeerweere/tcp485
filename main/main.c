#include <stdio.h>
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "rs485_netif.h"
#include "rs485_ping.h"
#include "rs485_tcp_server.h"
#include "rs485_udp_broadcast.h"

static const char *TAG = "TCP485";

#define NODE_ID 5
#define RS485_BAUD_RATE 19200

void app_main(void)
{
    ESP_LOGI(TAG, "TCP/IP over RS485 - Node %d", NODE_ID);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    rs485_netif_config_t netif_config = {
        .node_id = NODE_ID,
        .baud_rate = RS485_BAUD_RATE,
        .uart_tx_pin = 17,
        .uart_rx_pin = 18,
        .uart_rts_pin = 19,
        .ip_addr = "192.168.100.5",
        .netmask = "255.255.255.0",
        .gateway = "192.168.100.1"
    };

    ESP_ERROR_CHECK(rs485_netif_init(&netif_config));
    ESP_LOGI(TAG, "RS485 network interface initialized");
    ESP_LOGI(TAG, "IP: %s, MAC: 12:34:56:78:00:%02X", netif_config.ip_addr, NODE_ID);

    vTaskDelay(pdMS_TO_TICKS(2000));

    rs485_ping_test("192.168.100.10", 4);

    rs485_tcp_server_start(5000);

    rs485_udp_broadcast_start(5000);

    while (1) {
        rs485_netif_print_stats();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
