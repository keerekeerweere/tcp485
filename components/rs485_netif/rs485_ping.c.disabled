#include "rs485_ping.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/icmp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "RS485_PING";

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
    uint8_t data[32];
} icmp_echo_t;

static uint16_t icmp_checksum(void *data, size_t len)
{
    uint16_t *buf = (uint16_t *)data;
    uint32_t sum = 0;
    
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    
    if (len == 1) {
        sum += *(uint8_t *)buf;
    }
    
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    
    return ~sum;
}

static void on_ping_success(esp_ping_handle_t hdl, void *args)
{
    esp_ping_resp_t *resp = (esp_ping_resp_t *)args;
    ESP_LOGI(TAG, "%d bytes from %s: icmp_seq=%d ttl=%d time=%d ms",
             resp->total_len, ipaddr_ntoa(&resp->ip_addr),
             resp->seqno, resp->ttl, resp->resp_time);
}

static void on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    ESP_LOGW(TAG, "Ping timeout");
}

static void on_ping_end(esp_ping_handle_t hdl, void *args)
{
    ESP_LOGI(TAG, "Ping finished");
}

esp_err_t rs485_ping_test(const char *target_ip, uint32_t count)
{
    if (target_ip == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Pinging %s (%d times)...", target_ip, count);

    esp_ping_config_t config = {
        .target_addr = {},
        .count = count,
        .interval_ms = 1000,
        .timeout_ms = 2000,
        .data_size = 32,
        .tos = 0,
        .count_mode = ESP_PING_COUNT_SINGLE,
        .task_stack = 2048,
        .task_prio = 10
    };
    
    ip4addr_aton(target_ip, &config.target_addr.u_addr.ip4);
    config.target_addr.type = IPADDR_TYPE_V4;

    esp_ping_callbacks_t cbs = {
        .on_ping_success = on_ping_success,
        .on_ping_timeout = on_ping_timeout,
        .on_ping_end = on_ping_end,
        .cb_args = NULL,
        .esp_ping_install = NULL
    };

    esp_ping_handle_t ping;
    esp_err_t ret = esp_ping_new_session(&config, &cbs, &ping);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ping session: %d", ret);
        return ret;
    }

    ret = esp_ping_start(ping);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ping: %d", ret);
        esp_ping_delete_session(ping);
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(count * 1000 + 2000));

    esp_ping_stop(ping);
    esp_ping_delete_session(ping);

    return ESP_OK;
}
