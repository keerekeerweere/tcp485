#include "rs485_dns_example.h"
#include "esp_log.h"
#include "lwip/netdb.h"
#include <string.h>

static const char *TAG = "RS485_DNS_EXAMPLE";

esp_err_t rs485_dns_lookup_example(const char *hostname)
{
    if (hostname == NULL) {
        ESP_LOGE(TAG, "Invalid hostname");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Looking up: %s", hostname);

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    int err = getaddrinfo(hostname, NULL, &hints, &res);
    
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed: %d", err);
        return ESP_FAIL;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntoa_r(addr->sin_addr, ip_str, sizeof(ip_str));
    
    ESP_LOGI(TAG, "Resolved %s -> %s", hostname, ip_str);
    
    freeaddrinfo(res);
    return ESP_OK;
}

esp_err_t rs485_dns_get_server_info(void)
{
    ip_addr_t dns_server;
    
    dns_server = *dns_getserver(0);
    if (!ip_addr_isany(&dns_server)) {
        char dns_str[INET_ADDRSTRLEN];
        ipaddr_ntoa(&dns_server, dns_str, sizeof(dns_str));
        ESP_LOGI(TAG, "DNS server 1: %s", dns_str);
    } else {
        ESP_LOGI(TAG, "DNS server 1: Not configured");
    }
    
    dns_server = *dns_getserver(1);
    if (!ip_addr_isany(&dns_server)) {
        char dns_str[INET_ADDRSTRLEN];
        ipaddr_ntoa(&dns_server, dns_str, sizeof(dns_str));
        ESP_LOGI(TAG, "DNS server 2: %s", dns_str);
    } else {
        ESP_LOGI(TAG, "DNS server 2: Not configured");
    }
    
    return ESP_OK;
}
