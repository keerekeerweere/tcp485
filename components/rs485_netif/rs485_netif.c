#include "rs485_netif.h"
#include "rs485_frame.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "RS485_NETIF";

static esp_netif_t *s_rs485_netif = NULL;
static rs485_phy_t s_phy;
static csma_context_t s_csma_ctx;
static uint8_t s_mac_addr[MAC_ADDR_LEN];
static uint32_t s_rx_frames = 0;
static uint32_t s_rx_errors = 0;
static TaskHandle_t s_rx_task_handle = NULL;

static esp_err_t rs485_netif_transmit(void *h, void *buffer, size_t len);

static void rs485_rx_task(void *param)
{
    uint8_t rx_buffer[RS485_MAX_FRAME];
    
    ESP_LOGI(TAG, "RX task started");
    
    while (1) {
        int len = rs485_phy_receive(&s_phy, rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(100));
        
        if (len > 0) {
            s_rx_frames++;
            
            rs485_frame_t parsed;
            uint8_t *ip_packet;
            size_t ip_len;
            
            esp_err_t ret = rs485_frame_parse(rx_buffer, len, &parsed, &ip_packet, &ip_len);
            
            if (ret == ESP_OK) {
                if (memcmp(parsed.hdr.dest_mac, s_mac_addr, MAC_ADDR_LEN) == 0 ||
                    rs485_is_broadcast_mac(parsed.hdr.dest_mac)) {
                    
                    esp_netif_receive(s_rs485_netif, ip_packet, ip_len, NULL);
                }
            } else {
                s_rx_errors++;
                ESP_LOGD(TAG, "Frame parse error: %d", ret);
            }
        }
    }
}

static esp_err_t rs485_netif_transmit(void *h, void *buffer, size_t len)
{
    if (buffer == NULL || len < 14) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *eth_buffer = (uint8_t *)buffer;
    uint8_t dest_mac[MAC_ADDR_LEN];
    uint16_t ethertype;
    uint8_t *payload;
    size_t payload_len;
    
    memcpy(dest_mac, eth_buffer, MAC_ADDR_LEN);
    ethertype = (eth_buffer[12] << 8) | eth_buffer[13];
    payload = &eth_buffer[14];
    payload_len = len - 14;
    
    uint8_t rs485_frame[RS485_MAX_FRAME];
    size_t frame_len = sizeof(rs485_frame);
    
    esp_err_t ret = rs485_frame_build(dest_mac, s_mac_addr, ethertype, 
                                       payload, payload_len, 
                                       rs485_frame, &frame_len);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Frame build failed: %d", ret);
        return ret;
    }
    
    ret = csma_cd_transmit(&s_phy, rs485_frame, frame_len, &s_csma_ctx);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "CSMA/CD transmit failed: %d", ret);
    }
    
    return ret;
}

esp_err_t rs485_netif_init(rs485_netif_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing RS485 network interface (Node %d)", config->node_id);

    memset(&s_phy, 0, sizeof(s_phy));
    s_phy.uart_num = RS485_UART_NUM;
    s_phy.tx_pin = config->uart_tx_pin;
    s_phy.rx_pin = config->uart_rx_pin;
    s_phy.rts_pin = config->uart_rts_pin;
    s_phy.baud_rate = config->baud_rate;
    
    ESP_ERROR_CHECK(rs485_phy_init(&s_phy));
    
    rs485_generate_mac(s_mac_addr, config->node_id);
    csma_cd_init(&s_csma_ctx);
    
    ESP_LOGI(TAG, "MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
             s_mac_addr[0], s_mac_addr[1], s_mac_addr[2], 
             s_mac_addr[3], s_mac_addr[4], s_mac_addr[5]);

    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    s_rs485_netif = esp_netif_new(&netif_cfg);
    
    if (s_rs485_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create netif");
        return ESP_FAIL;
    }

    esp_netif_driver_ifconfig_t driver_cfg = {
        .handle = NULL,
        .transmit = rs485_netif_transmit,
        .driver_free_rx_buffer = NULL
    };
    
    esp_err_t ret = esp_netif_set_driver_config(s_rs485_netif, &driver_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set driver config: %d", ret);
        return ret;
    }
    
    esp_netif_set_mac(s_rs485_netif, s_mac_addr);
    
    esp_netif_ip_info_t ip_info;
    ip4addr_aton(config->ip_addr, &ip_info.ip);
    ip4addr_aton(config->netmask, &ip_info.netmask);
    ip4addr_aton(config->gateway, &ip_info.gw);
    
    ret = esp_netif_set_ip_info(s_rs485_netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set IP info: %d", ret);
        return ret;
    }

    ret = esp_netif_action_start(s_rs485_netif, NULL, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start netif: %d", ret);
        return ret;
    }
    
    xTaskCreate(rs485_rx_task, "rs485_rx", 4096, NULL, 10, &s_rx_task_handle);
    
    ESP_LOGI(TAG, "RS485 network interface initialized successfully");
    return ESP_OK;
}

esp_err_t rs485_netif_deinit(void)
{
    if (s_rx_task_handle != NULL) {
        vTaskDelete(s_rx_task_handle);
        s_rx_task_handle = NULL;
    }
    
    if (s_rs485_netif != NULL) {
        esp_netif_action_stop(s_rs485_netif, NULL, 0, NULL);
        esp_netif_destroy(s_rs485_netif);
        s_rs485_netif = NULL;
    }
    
    rs485_phy_deinit(&s_phy);
    
    return ESP_OK;
}

void rs485_netif_print_stats(void)
{
    uint32_t collisions, tx_frames, tx_errors;
    csma_cd_get_stats(&s_csma_ctx, &collisions, &tx_frames, &tx_errors);
    
    ESP_LOGI(TAG, "NETIF Stats: RX=%d, RX_ERR=%d, TX=%d, TX_ERR=%d, Collisions=%d",
             s_rx_frames, s_rx_errors, tx_frames, tx_errors, collisions);
}

esp_netif_t *rs485_netif_get_handle(void)
{
    return s_rs485_netif;
}
