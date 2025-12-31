#ifndef RS485_NETIF_H
#define RS485_NETIF_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "rs485_phy.h"
#include "rs485_mac.h"

typedef struct {
    uint8_t node_id;
    uint32_t baud_rate;
    int uart_tx_pin;
    int uart_rx_pin;
    int uart_rts_pin;
    const char *ip_addr;
    const char *netmask;
    const char *gateway;
    const char *dns_server1;
    const char *dns_server2;
    const char *hostname;
    const char *search_domain;
} rs485_netif_config_t;

esp_err_t rs485_netif_init(rs485_netif_config_t *config);
esp_err_t rs485_netif_deinit(void);
void rs485_netif_print_stats(void);
esp_netif_t *rs485_netif_get_handle(void);

#endif
