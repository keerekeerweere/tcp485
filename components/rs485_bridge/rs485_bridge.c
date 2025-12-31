#include "rs485_bridge.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_eth.h"
#include "rs485_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "RS485_BRIDGE";

static esp_netif_t *s_wifi_netif = NULL;
static esp_netif_t *s_ethernet_netif = NULL;
static esp_netif_t *s_rs485_netif = NULL;
static rs485_bridge_config_t s_config;

static uint32_t stats_wifi_to_rs485 = 0;
static uint32_t stats_rs485_to_wifi = 0;
static uint32_t stats_filtered_protocol = 0;
static uint32_t stats_filtered_rate = 0;

static inline bool bridge_is_allowed(uint16_t ethertype, uint16_t port)
{
    if (!s_config.filter_enabled) {
        return true;
    }
    
    if (ethertype == 0x0806) {  // ARP
        return s_config.allow_arp;
    }
    
    if (ethertype == 0x0800) {  // IPv4
        uint8_t proto = ((uint8_t *)port)[9];
        if (proto == 1) {  // ICMP
            return s_config.allow_icmp;
        } else if (proto == 6 && s_config.allow_tcp) {  // TCP
            for (int i = 0; i < 8; i++) {
                if (s_config.allowed_tcp_ports[i] == 0 || s_config.allowed_tcp_ports[i] == port) {
                    return true;
                }
            }
            return false;
        } else if (proto == 17 && s_config.allow_udp) {  // UDP
            for (int i = 0; i < 8; i++) {
                if (s_config.allowed_udp_ports[i] == 0 || s_config.allowed_udp_ports[i] == port) {
                    return true;
                }
            }
            return false;
        }
    }
    
    return true;
}

static inline bool bridge_check_rate_limit(size_t packet_size)
{
    if (!s_config.rate_limit_enabled) {
        return true;
    }
    return true;
}

static void fast_netif_rx_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    esp_netif_receive_t *rx = (esp_netif_receive_t *)event_data;
    
    if (rx->len < 14) {
        return;
    }
    
    uint8_t *eth_frame = (uint8_t *)rx->buffer;
    uint16_t ethertype = (eth_frame[12] << 8) | eth_frame[13];
    uint8_t *payload = &eth_frame[14];
    
    if (ethertype == 0x0800) {  // IPv4
        uint8_t proto = payload[9];
        uint16_t port = 0;
        if (proto == 6 || proto == 17) {  // TCP or UDP
            if (rx->len >= 20) {  // IP header (20) + port (2) = 22
                port = (payload[20] << 8) | payload[21];
            }
        }
        
        if (!bridge_is_allowed(ethertype, port)) {
            stats_filtered_protocol++;
            return;
        }
        
        if (!bridge_check_rate_limit(rx->len)) {
            stats_filtered_rate++;
            return;
        }
    }
    
    if (s_rs485_netif != NULL) {
        esp_netif_transmit(s_rs485_netif, rx->buffer, rx->len);
        stats_wifi_to_rs485++;
    }
}

static void rs485_netif_rx_handler(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data)
{
    esp_netif_receive_t *rx = (esp_netif_receive_t *)event_data;
    
    if (s_wifi_netif != NULL || s_ethernet_netif != NULL) {
        esp_netif_transmit_t target = (s_wifi_netif != NULL) ? s_wifi_netif : s_ethernet_netif;
        esp_netif_transmit(target, rx->buffer, rx->len);
        stats_rs485_to_wifi++;
    }
}

esp_err_t rs485_bridge_init(rs485_bridge_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing RS485-WiFi/Ethernet bridge");
    
    memcpy(&s_config, config, sizeof(rs485_bridge_config_t));
    
    if (config->use_wifi) {
        ESP_LOGI(TAG, "WiFi enabled");
    } else if (config->use_ethernet) {
        ESP_LOGI(TAG, "Ethernet enabled");
    }
    
    ESP_LOGI(TAG, "Bridge IP on RS485: %s", RS485_BRIDGE_IP);
    ESP_LOGI(TAG, "RS485 node ID: %d", config->rs485_node_id);
    
    if (config->use_wifi) {
        esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY, fast_netif_rx_handler, NULL);
        s_wifi_netif = esp_netif_get_default_wifi_netif();
    }
    
    if (config->use_ethernet) {
        esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY, fast_netif_rx_handler, NULL);
        s_ethernet_netif = esp_netif_get_default_eth_netif();
    }
    
    rs485_netif_config_t rs485_cfg = {
        .node_id = config->rs485_node_id,
        .baud_rate = config->rs485_baud_rate,
        .uart_tx_pin = config->rs485_tx_pin,
        .uart_rx_pin = config->rs485_rx_pin,
        .uart_rts_pin = config->rs485_rts_pin,
        .ip_addr = RS485_BRIDGE_IP,
        .netmask = RS485_BRIDGE_NETMASK,
        .gateway = RS485_BRIDGE_IP,
        .use_dhcp = false,
        .hostname = "bridge",
        .dns_server1 = config->use_wifi ? "192.168.1.1" : NULL,
        .dns_server2 = config->use_ethernet ? "192.168.1.1" : NULL,
        .search_domain = NULL
    };
    
    ESP_ERROR_CHECK(rs485_netif_init(&rs485_cfg));
    s_rs485_netif = rs485_netif_get_handle();
    
    esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY, rs485_netif_rx_handler, NULL);
    
    ESP_LOGI(TAG, "RS485-WiFi/Ethernet bridge initialized");
    return ESP_OK;
}

esp_err_t rs485_bridge_stop(void)
{
    ESP_LOGI(TAG, "Stopping RS485-WiFi/Ethernet bridge");
    
    if (s_rs485_netif != NULL) {
        rs485_netif_deinit();
        s_rs485_netif = NULL;
    }
    
    if (s_wifi_netif != NULL) {
        esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY, fast_netif_rx_handler, NULL);
        s_wifi_netif = NULL;
    }
    
    if (s_ethernet_netif != NULL) {
        esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY, fast_netif_rx_handler, NULL);
        s_ethernet_netif = NULL;
    }
    
    return ESP_OK;
}

void rs485_bridge_print_stats(void)
{
    ESP_LOGI(TAG, "=== Bridge Statistics ===");
    ESP_LOGI(TAG, "Wi-Fi/Ethernet → RS485: %d", stats_wifi_to_rs485);
    ESP_LOGI(TAG, "RS485 → Wi-Fi/Ethernet: %d", stats_rs485_to_wifi);
    ESP_LOGI(TAG, "Filtered (protocol): %d", stats_filtered_protocol);
    ESP_LOGI(TAG, "Filtered (rate limit): %d", stats_filtered_rate);
    ESP_LOGI(TAG, "=========================");
}
