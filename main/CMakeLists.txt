idf_component_register(SRCS "simple_main.c"
                     INCLUDE_DIRS "."
                     REQUIRES "rs485_phy" "rs485_frame" "rs485_mac" "nvs_flash")
