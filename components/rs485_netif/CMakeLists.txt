idf_component_register(SRCS "rs485_netif.c" "rs485_ping.c" "rs485_tcp_server.c" "rs485_udp_broadcast.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp_netif" "lwip" "rs485_phy" "rs485_frame" "rs485_mac")
