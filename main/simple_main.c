#include <stdio.h>
#include "esp_log.h"
#include "rs485_phy.h"
#include "rs485_mac.h"
#include "rs485_frame.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    ESP_LOGI("MAIN", "RS485 TCP/IP stack application starting...");
    
    ESP_LOGI("MAIN", "This is a minimal main for testing core components");
    ESP_LOGI("MAIN", "Core RS485 components (PHY, MAC, FRAME) are compiled");
    ESP_LOGI("MAIN", "For full functionality, rs485_netif component needs API migration");
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}