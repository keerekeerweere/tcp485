idf_component_register(SRCS "rs485_netif.c" "rs485_ping.c" "rs485_tcp_server.c" "rs485_udp_broadcast.c" "rs485_dns_example.c"
                     INCLUDE_DIRS "."
                     REQUIRES "esp_netif" "lwip" "esp_netif" "rs485_phy" "rs485_frame" "rs485_mac" "rs485_bridge")
