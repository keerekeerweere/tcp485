#ifndef RS485_BRIDGE_H
#define RS485_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define RS485_BRIDGE_NODE_ID 1
#define RS485_BRIDGE_IP "192.168.100.1"
#define RS485_BRIDGE_NETMASK "255.255.255.0"

typedef enum {
    RS485_BRIDGE_MODE_L2_TRANSPARENT,
    RS485_BRIDGE_MODE_L3_ROUTED,
} rs485_bridge_mode_t;

typedef struct {
    bool use_wifi;
    bool use_ethernet;
    
    bool filter_enabled;
    bool allow_arp;
    bool allow_icmp;
    bool allow_tcp;
    bool allow_udp;
    uint16_t allowed_tcp_ports[8];
    uint16_t allowed_udp_ports[8];
    
    bool rate_limit_enabled;
    uint8_t wifi_to_rs485_ratio;
    
    uint8_t rs485_node_id;
    uint32_t rs485_baud_rate;
    int rs485_tx_pin;
    int rs485_rx_pin;
    int rs485_rts_pin;
} rs485_bridge_config_t;

esp_err_t rs485_bridge_init(rs485_bridge_config_t *config);
esp_err_t rs485_bridge_deinit(void);

#endif
