idf_component_register(SRCS "rs485_netif_minimal.c"
                     INCLUDE_DIRS "."
                     REQUIRES "esp_netif" "lwip" "esp_netif" "rs485_phy" "rs485_frame" "rs485_mac")
