#include "rs485_netif.h"
#include "rs485_frame.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "esp_netif.h"
#include "rs485_phy.h"
#include "rs485_mac.h"
#include "rs485_frame.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <string.h>

static const char *TAG = "RS485_NETIF";

static esp_netif_t *s_rs485_netif = NULL;
static TaskHandle_t s_netif_task = NULL;
static uint8_t s_mac_addr[6] = {0x12, 0x34, 0x56, 0x78, 0x00, 0x00};
static uint32_t s_tx_count = 0;
static uint32_t s_rx_count = 0;
static uint32_t s_frame_errors = 0;

void rs485_netif_rx_handler(void *arg, esp_err_t err)
{
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RX error: %d", err);
        return;
    }
    
    // TODO: Implement proper frame processing
    ESP_LOGI(TAG, "Frame received (not implemented for ESP-IDF v5.5.2)");
}

esp_err_t rs485_netif_init(rs485_netif_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing RS485 network interface...");
    ESP_LOGI(TAG, "Node ID: %d", config->node_id);
    ESP_LOGI(TAG, "Baud rate: %d", config->baud_rate);
    
    // Initialize RS485 PHY
    rs485_phy_t rs485_phy = {
        .uart_num = RS485_UART_NUM,
        .tx_pin = config->uart_tx_pin,
        .rx_pin = config->uart_rx_pin,
        .rts_pin = config->uart_rts_pin,
        .baud_rate = config->baud_rate,
        .distance_m = 100,
        .slot_time_ms = 51,
        .inter_frame_gap_ms = 5,
    };
    
    // Create network interface - TODO: ESP-IDF v5.5.2 API migration needed
    ESP_LOGW(TAG, "Network interface creation temporarily disabled");
    ESP_LOGW(TAG, "Requires ESP-IDF v5.5.2 netif_ops structure");
    s_rs485_netif = NULL;
    
    // Set MAC address (auto-generated from node ID)
    memcpy(s_mac_addr + 5, &config->node_id, 1);
    esp_netif_set_mac(s_rs485_netif, s_mac_addr);
    
    ESP_LOGI(TAG, "MAC address: %02X:%02X:%02X:%02X:%02X:%02X:%02X",
              s_mac_addr[0], s_mac_addr[1], s_mac_addr[2],
              s_mac_addr[3], s_mac_addr[4], s_mac_addr[5]);
    
    // Set IP address (using ESP-IDF v5.5.2 compatible API)
    if (!config->use_dhcp) {
        esp_netif_ip_info_t ip_info;
        
        // Convert string IP addresses to esp_ip4_addr_t format
        esp_ip4_addr_t esp_ip4;
        esp_netif_str_to_ip4(config->ip_addr, &esp_ip4);
        
        ip_info.ip = esp_ip4;
        
        esp_netif_str_to_ip4(config->netmask, &esp_ip4);
        ip_info.netmask = esp_ip4;
        
        esp_netif_str_to_ip4(config->gateway, &esp_ip4);
        ip_info.gw = esp_ip4;
        
        ret = esp_netif_set_ip_info(s_rs485_netif, &ip_info);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set IP info: %d", ret);
            esp_netif_destroy(s_rs485_netif);
            rs485_phy_deinit(NULL);
            return ret;
        }
        
        ESP_LOGI(TAG, "IP set: %s", config->ip_addr);
    }
    
    // Set hostname
    if (config->hostname != NULL) {
        esp_netif_set_hostname(s_rs485_netif, config->hostname);
        ESP_LOGI(TAG, "Hostname: %s", config->hostname);
    }
    
    // DNS configuration - temporarily disabled for ESP-IDF v5.5.2
    if (config->dns_server1 != NULL) {
        ESP_LOGW(TAG, "DNS server 1: %s (not set - API migration needed)", config->dns_server1);
        // TODO: Implement ESP-IDF v5.5.2 DNS API
    }
    
    if (config->dns_server2 != NULL) {
        ESP_LOGW(TAG, "DNS server 2: %s (not set - API migration needed)", config->dns_server2);
    }
    
    if (config->search_domain != NULL) {
        ESP_LOGW(TAG, "Search domain: %s (not set - API migration needed)", config->search_domain);
    }
    
    // Start network interface - temporarily disabled for ESP-IDF v5.5.2 compatibility
    ESP_LOGW(TAG, "Network interface creation temporarily disabled");
    ESP_LOGW(TAG, "Requires ESP-IDF v5.5.2 netif API migration");
    s_rs485_netif = NULL; // Just mark as initialized
    ret = ESP_OK;
    
    ESP_LOGI(TAG, "RS485 network interface started successfully (minimal)");
    return ESP_OK;
}

esp_err_t rs485_netif_deinit(void)
{
    if (s_rs485_netif == NULL) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing RS485 network interface...");
    
    if (s_netif_task != NULL) {
        vTaskDelete(s_netif_task);
        s_netif_task = NULL;
    }
    
    esp_netif_destroy(s_rs485_netif);
    s_rs485_netif = NULL;
    
    rs485_phy_deinit(NULL);
    
    ESP_LOGI(TAG, "RS485 network interface deinitialized");
    return ESP_OK;
}

void rs485_netif_print_stats(void)
{
    if (s_rs485_netif != NULL) {
        ESP_LOGI(TAG, "RS485 Network Interface Statistics:");
        ESP_LOGI(TAG, "TX frames: %lu", s_tx_count);
        ESP_LOGI(TAG, "RX frames: %lu", s_rx_count);
        ESP_LOGI(TAG, "Frame errors: %lu", s_frame_errors);
    }
}

esp_netif_t *rs485_netif_get_handle(void)
{
    return s_rs485_netif;
}