#include "rs485_udp_broadcast.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "RS485_UDP_BCAST";

static void udp_broadcast_task(void *arg)
{
    uint16_t port = *(uint16_t *)arg;
    int sock = -1;
    uint32_t counter = 0;

    while (1) {
        if (sock < 0) {
            sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            int opt = 1;
            setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in bind_addr;
            bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            bind_addr.sin_family = AF_INET;
            bind_addr.sin_port = htons(port);

            if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) != 0) {
                ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
                close(sock);
                sock = -1;
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            ESP_LOGI(TAG, "UDP broadcast socket bound to port %d", port);
        }

        char message[64];
        int len = snprintf(message, sizeof(message), "Node broadcast #%lu", counter++);

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr("192.168.100.255");
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);

        int err = sendto(sock, message, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            close(sock);
            sock = -1;
        } else {
            ESP_LOGD(TAG, "Broadcast: %s", message);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (sock >= 0) {
        close(sock);
    }
    
    vTaskDelete(NULL);
}

esp_err_t rs485_udp_broadcast_start(uint16_t port)
{
    uint16_t *port_arg = malloc(sizeof(uint16_t));
    if (port_arg == NULL) {
        return ESP_ERR_NO_MEM;
    }
    *port_arg = port;

    BaseType_t ret = xTaskCreate(udp_broadcast_task, "udp_bcast", 4096, port_arg, 5, NULL);
    if (ret != pdPASS) {
        free(port_arg);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "UDP broadcast started on port %d", port);
    return ESP_OK;
}
