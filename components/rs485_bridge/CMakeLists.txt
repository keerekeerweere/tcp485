idf_component_register(SRCS "rs485_bridge.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp_netif" "esp_wifi" "esp_eth" "rs485_netif")
