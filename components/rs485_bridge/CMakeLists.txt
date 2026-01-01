idf_component_register(SRCS "rs485_bridge.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp_netif" "esp_wifi" "rs485_netif" "rs485_mac" "rs485_phy")