#include <stdio.h>
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "rs485_bridge.h"

static const char *TAG = "RS485_BRIDGE_EXAMPLE";

#define BRIDGE_USE_WIFI true

void app_main(void)
{
    ESP_LOGI(TAG, "RS485-WiFi/Ethernet L2 Bridge Example");

    ESP_ERROR_CHECK(nvs_flash_init());
    
    rs485_bridge_config_t config = {
        .use_wifi = BRIDGE_USE_WIFI,
        .use_ethernet = !BRIDGE_USE_WIFI,
        
        .rs485_node_id = 1,
        .rs485_baud_rate = 19200,
        .rs485_tx_pin = 17,
        .rs485_rx_pin = 18,
        .rs485_rts_pin = 19,
        
        .filter_enabled = true,
        .allow_arp = true,
        .allow_icmp = true,
        .allow_tcp = true,
        .allow_udp = false,
        .allowed_tcp_ports = {80, 443, 5000},
        .allowed_udp_ports = {},
        
        .rate_limit_enabled = true,
        .wifi_to_rs485_ratio = 10,
    };
    
    ESP_ERROR_CHECK(rs485_bridge_init(&config));
    
    ESP_LOGI(TAG, "Bridge initialized successfully");
    ESP_LOGI(TAG, "Bridge IP: 192.168.100.1 on RS485 network");
    ESP_LOGI(TAG, "RS485 node ID: %d", config.rs485_node_id);
    ESP_LOGI(TAG, "Filtering: %s", config.filter_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "Rate limit: %d%%", config.wifi_to_rs485_ratio);
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        rs485_bridge_print_stats();
    }
}
