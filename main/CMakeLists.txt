idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp_netif" "lwip" "driver" "rs485_phy" "rs485_frame" "rs485_mac" "rs485_netif")
