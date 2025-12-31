#include "rs485_tcp_server.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "RS485_TCP_SERVER";

static void tcp_server_task(void *arg)
{
    uint16_t port = *(uint16_t *)arg;
    int listen_sock = -1;
    char rx_buffer[128];

    while (1) {
        listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            close(listen_sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        ESP_LOGI(TAG, "Socket bound, port %d", port);

        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            close(listen_sock);
            continue;
        }

        while (1) {
            struct sockaddr_in source_addr;
            socklen_t addr_len = sizeof(source_addr);
            int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                break;
            }
            ESP_LOGI(TAG, "Socket accepted from %s:%d", inet_ntoa(source_addr.sin_addr), ntohs(source_addr.sin_port));

            int len;
            do {
                len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
                if (len < 0) {
                    ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
                } else if (len == 0) {
                    ESP_LOGI(TAG, "Connection closed");
                } else {
                    rx_buffer[len] = 0;
                    ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
                    
                    int to_write = len;
                    while (to_write > 0) {
                        int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                        if (written < 0) {
                            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                            break;
                        }
                        to_write -= written;
                    }
                }
            } while (len > 0);

            shutdown(sock, 0);
            close(sock);
        }

        close(listen_sock);
    }
    
    vTaskDelete(NULL);
}

esp_err_t rs485_tcp_server_start(uint16_t port)
{
    uint16_t *port_arg = malloc(sizeof(uint16_t));
    if (port_arg == NULL) {
        return ESP_ERR_NO_MEM;
    }
    *port_arg = port;

    BaseType_t ret = xTaskCreate(tcp_server_task, "tcp_server", 4096, port_arg, 5, NULL);
    if (ret != pdPASS) {
        free(port_arg);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "TCP server started on port %d", port);
    return ESP_OK;
}
