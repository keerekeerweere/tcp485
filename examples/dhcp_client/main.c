#include <stdio.h>
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "rs485_netif.h"

static const char *TAG = "DHCP_CLIENT";

#define NODE_ID 5
#define RS485_BAUD_RATE 19200

void got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP: "IPSTR", IP2STR(&event->ip_info.ip));
}

void app_main(void)
{
    ESP_LOGI(TAG, "DHCP Client Example - Node %d", NODE_ID);
    
    ESP_ERROR_CHECK(nvs_flash_init());
    
    rs485_netif_config_t netif_config = {
        .node_id = NODE_ID,
        .baud_rate = RS485_BAUD_RATE,
        .uart_tx_pin = 17,
        .uart_rx_pin = 18,
        .uart_rts_pin = 19,
        .ip_addr = NULL,          
        .netmask = NULL,         
        .gateway = NULL,         
        .use_dhcp = true,        
        .hostname = "node5",       
        .dns_server1 = "192.168.100.1", 
        .dns_server2 = NULL,
        .search_domain = "rs485.local" 
    };
    
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_handler, NULL));
    
    ESP_ERROR_CHECK(rs485_netif_init(&netif_config));
    ESP_LOGI(TAG, "DHCP client started, waiting for IP assignment from 192.168.100.1...");
    
    while (1) {
        rs485_netif_print_stats();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
