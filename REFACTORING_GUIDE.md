# ESP-IDF v5.5.2 Refactoring Guide

This document provides a systematic approach to refactoring the TCP/IP over RS485 project for ESP-IDF v5.5.2 compatibility.

## Current Status

The project was originally developed for ESP-IDF v5.0 and requires significant updates to work with ESP-IDF v5.5.2. 

### Major API Changes Required:

1. **UART Driver API** - Function signatures and parameters have changed
2. **Network Stack API** - DNS and IP address functions have been updated
3. **Component Dependencies** - Several missing component dependencies added
4. **Include Paths** - Header file organization has changed

## Completed Fixes ✅

### 1. Component Dependencies
- Added `rs485_phy` to `rs485_mac` component requirements
- Fixed missing component dependencies in CMakeLists.txt files

### 2. Header Files
- Fixed malformed include statement in `rs485_phy.c`
- Added proper include guards and forward declarations

### 3. Type Definitions
- Added missing `rs485_phy_t` struct definition
- Fixed duplicate function declarations
- Removed duplicate CSMA type definitions

### 4. Format Specifiers
- Fixed `%X` to `%08lX` for uint32_t in `rs485_frame.c`
- Added proper format specifiers for different data types

## Required Systematic Fixes 📋

### 1. UART API Updates

**File:** `components/rs485_phy/rs485_phy.c`

#### Issues:
- `uart_param_config()` function doesn't exist in ESP-IDF v5.5.2
- `uart_set_pin()` parameter order has changed
- Missing proper UART configuration sequence

#### Required Changes:
```c
// OLD (v5.0):
uart_param_config(uart_num, &config);
uart_driver_install(uart_num, ...);
uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, rts_pin);

// NEW (v5.5.2):
uart_config_t uart_config = {
    .baud_rate = s_phy.baud_rate,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB
};

ESP_ERROR_CHECK(uart_param_config(phy->uart_num, &uart_config));
ESP_ERROR_CHECK(uart_driver_install(phy->uart_num, RS485_UART_BUF_SIZE, 
                                   RS485_UART_BUF_SIZE, 0, NULL, 0));
ESP_ERROR_CHECK(uart_set_pin(phy->uart_num, s_phy.tx_pin, s_phy.rx_pin, 
                                 s_phy.rts_pin, UART_PIN_NO_CHANGE));
```

### 2. DNS API Updates

**File:** `components/rs485_netif/rs485_dns_example.c`

#### Issues:
- `dns_getserver()` function signature has changed
- `dns_setserver()` parameters have changed

#### Required Changes:
```c
// OLD (v5.0):
dns_setserver(index, server_ip);
dns_getserver(index);

// NEW (v5.5.2):
esp_netif_dns_info_t dns_info;
esp_netif_get_dns_info(netif, &dns_info);
// Use dns_info.ip.u_addr.ip4.addr for server address
```

### 3. IP Address API Updates

**File:** `components/rs485_netif/rs485_netif.c`

#### Issues:
- `ip4addr_aton()` function has been deprecated
- `ipaddr_ntoa()` function parameters have changed

#### Required Changes:
```c
// OLD (v5.0):
ip4addr_aton(ip_str, &addr);
ipaddr_ntoa(&addr);

// NEW (v5.5.2):
ip4_addr_t addr;
ip4addr_aton(ip_str, &addr);
ipaddr_ntoa(&addr, buffer, sizeof(buffer));
```

### 4. Missing Headers

**Files:** Multiple files across components

#### Required Headers:
```c
// Add to rs485_phy.c:
#include "esp_random.h"
#include "esp_system.h"

// Add to rs485_mac.c:
#include "esp_random.h"

// Add to rs485_netif files:
#include "esp_netif.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
```

### 5. Function Signature Updates

#### rs485_phy_init function:
```c
// Fix the order of operations:
esp_err_t rs485_phy_init(rs485_phy_t *phy)
{
    if (phy == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // First configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = phy->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB
    };
    
    // Then install driver with proper parameters
    ESP_ERROR_CHECK(uart_param_config(phy->uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(phy->uart_num, 
                                   RS485_UART_BUF_SIZE, 
                                   RS485_UART_BUF_SIZE, 
                                   0, 
                                   NULL, 
                                   0));
    
    // Finally set pins
    ESP_ERROR_CHECK(uart_set_pin(phy->uart_num, 
                                 phy->tx_pin, 
                                 phy->rx_pin, 
                                 phy->rts_pin, 
                                 UART_PIN_NO_CHANGE));
    
    // Store configuration
    s_phy.uart_num = phy->uart_num;
    s_phy.tx_pin = phy->tx_pin;
    s_phy.rx_pin = phy->rx_pin;
    s_phy.rts_pin = phy->rts_pin;
    s_phy.baud_rate = phy->baud_rate;
    s_phy.initialized = true;
    
    ESP_LOGI(TAG, "RS485 PHY initialized: UART%d, TX=%d, RX=%d, RTS=%d, %d baud", 
              s_phy.uart_num, s_phy.tx_pin, s_phy.rx_pin, s_phy.rts_pin, s_phy.baud_rate);
    
    return ESP_OK;
}
```

## Testing Strategy

### Phase 1: Component Level
1. Fix `rs485_phy` component first (lowest level)
2. Fix `rs485_frame` component (depends only on std libs)
3. Fix `rs485_mac` component (depends on phy)
4. Fix `rs485_netif` component (depends on mac, frame, phy)

### Phase 2: Integration Level
1. Fix main application integration
2. Test basic functionality
3. Add comprehensive error handling

### Phase 3: Example Level
1. Fix DHCP client example
2. Fix DNS example
3. Fix ping, TCP server, UDP broadcast examples

## Build System Updates

### sdkconfig Requirements
Ensure these settings are enabled in sdkconfig:
```
CONFIG_LWIP_IPV4=y
CONFIG_LWIP_ICMP=y
CONFIG_LWIP_TCP=y
CONFIG_LWIP_UDP=y
CONFIG_ESP_NETIF_IP_LOST_TIMER_INTERVAL=1000
```

## Common Issues and Solutions

### Issue: "unknown type name 'uint32_t'"
**Solution:** Add `#include <stdint.h>` or `#include <sys/types.h>`

### Issue: "implicit declaration of function"
**Solution:** Add proper header includes at the top of source files

### Issue: "passing argument X of function Y from incompatible pointer type"
**Solution:** Check API documentation for function signatures and update accordingly

## Migration Checklist

- [ ] Fix all UART API calls
- [ ] Update DNS API functions
- [ ] Update IP address functions  
- [ ] Add missing headers across all files
- [ ] Test compilation of each component individually
- [ ] Test full project build
- [ ] Verify runtime functionality
- [ ] Test on actual hardware

## References

- [ESP-IDF v5.5.2 Documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/)
- [UART Driver Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/api-reference/peripherals/uart.html)
- [Network Interface Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/api-guides/esp-netif.html)
- [lwIP Integration](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/api-guides/lwip.html)

## Next Steps

1. Systematically apply fixes component by component
2. Test build after each component fix
3. Address remaining API incompatibilities
4. Validate functionality on target hardware
5. Update documentation for ESP-IDF v5.5.2

## Notes

- This refactoring requires understanding of both ESP-IDF v5.0 and v5.5.2 APIs
- Some deprecated functions may require rewriting logic rather than simple API updates
- Testing on actual hardware is essential after code compilation succeeds
- Consider backward compatibility if older ESP-IDF versions need support