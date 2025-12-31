# RS485-WiFi/Ethernet L2 Bridge Component

Transparent L2 bridge between WiFi/Ethernet and RS485 networks with compile-time traffic filtering and rate limiting to protect RS485 from overload.

> **DISCLAIMER**: This code is AI-generated and has NOT been tested on hardware. It is provided as a thought experiment and reference implementation only. Use at your own risk. This code may contain errors, bugs, or incomplete functionality. You are free to improve, modify, or reuse any part of this code without restriction.

**Feedback & Improvements Are Welcome!**
This is an experimental implementation intended as a reference. If you encounter issues, find bugs, or have suggestions for improvements, please provide feedback. Bug reports, security fixes, and enhancements are encouraged. You are free to modify, patch, or improve any part of this code.

**Security Considerations**
- Bridge node becomes single point of compromise - segment RS485 network
- Implement authentication for bridge management interface
- RS485 is a shared medium - consider network segmentation for sensitive environments
- Filter rules are compile-time - no runtime security controls
- Review and harden implementation before production use

**Testing Recommendations**
- Start with 2-node test (WiFi ↔ RS485)
- Monitor filtered packet counts to verify filtering rules
- Verify rate limiting prevents RS485 overflow
- Test with actual hardware traffic loads

---

## Architecture Overview

```
                    ┌─────────────────────────────────────────┐
                    │  Internet/WAN (WiFi/Ethernet)      │
                    │                                   │
        ┌─────────────────────────────────────┴──────────┐
        │                                      │          │
    ┌──────────────────────────────────────────────┐    │
    │           Bridge Node (ESP32-S3)           │    │
    │  ┌──────────────────────────────────────┐    │    │
    │  │ WiFi/Ethernet Netif (Fast)         │    │    │
    │  │ 192.168.1.0/24                  │    │    │
    │  │ MAC: AA:BB:CC:DD:EE:FF          │    │    │
    │  │ ↓ L2 Forward (Transparent)            │    │    │
    │  └──────────────────────────────────────────┘    │    │
    │                                      │          │
    │                                      │          │
    │  RS485 Netif (Slow)                  │          │
    │  192.168.100.1/24                    │          │
    │  MAC: 12:34:56:78:00:01            │          │
    │  ↓                                  │          │
    └──────────────────────────────────────────────┘    │    │
            │                                             │
            ▼                                             │
              ┌─────────────────────────────────────┐
              │ RS485 Network (32 Nodes)       │
              │ 192.168.100.0/24              │
              │ Node 1: 192.168.100.1 (Bridge)   │
              │ Node 2-32: 192.168.100.2-32 (DHCP clients) │
              └─────────────────────────────────────┘
```

## Key Features

- **Transparent L2 Bridging**: MAC-layer forwarding between WiFi/Ethernet and RS485
- **Compile-Time Filtering**: Simple #define-based filtering (no runtime overhead)
- **Rate Limiting**: WiFi→RS485 bandwidth ratio protection (default: 10%)
- **Independent RS485 IP**: Bridge has its own IP (192.168.100.1) on RS485 network
- **Standard ESP-IDF APIs**: Uses `esp_netif_transmit()` for compatibility
- **Statistics**: Optional compile-time statistics for debugging

## Traffic Filtering (Compile-Time)

| Filter Type | Default | #define | Description |
|-------------|---------|----------|-------------|
| ARP | Enabled | `BRIDGE_FILTER_ARP` | Allow/disallow ARP packets |
| ICMP (ping) | Enabled | `BRIDGE_FILTER_ICMP` | Allow/disallow ICMP echo requests |
| TCP | Enabled | `BRIDGE_FILTER_TCP` | Allow TCP traffic |
| UDP | Disabled | `BRIDGE_FILTER_UDP` | Block UDP (prevent floods) |
| Port Filtering | Enabled | `BRIDGE_FILTER_PORTS` | Only forward specific ports |

## Rate Limiting (Compile-Time)

```c
// Default: 10% of WiFi bandwidth to RS485
#define BRIDGE_WIFI_TO_RS485_RATIO_DEFAULT 10  // 1.9 KB/s at 100 Mbps WiFi

// Compile-time rate limiting
#define BRIDGE_RATE_LIMIT_ENABLED        // Enable rate limiting
#define BRIDGE_WIFI_TO_RS485_RATIO     10  // Configurable ratio (1-100%)
```

## Configuration

```c
#include "rs485_bridge.h"

void app_main(void)
{
    // Bridge configuration
    rs485_bridge_config_t config = {
        // Network interfaces
        .use_wifi = true,           // Use WiFi
        .use_ethernet = false,      // Not using Ethernet
        
        // RS485 configuration (independent IP)
        .rs485_node_id = 1,          // Bridge is node 1 on RS485
        .rs485_baud_rate = 19200,
        .rs485_tx_pin = 17,
        .rs485_rx_pin = 18,
        .rs485_rts_pin = 19,
        
        // Compile-time filtering (simple, no runtime config)
        .filter_enabled = true,       // Enable filtering
        .allow_arp = true,           // Allow ARP
        .allow_icmp = true,          // Allow ICMP (ping)
        .allow_tcp = true,           // Allow TCP
        .allow_udp = false,          // Block UDP (prevent floods)
        .allowed_tcp_ports = {80, 443, 5000},  // HTTP, HTTPS, app port
        .allowed_udp_ports = {},     // No UDP ports allowed
        
        // Rate limiting
        .rate_limit_enabled = true,
        .wifi_to_rs485_ratio = 10, // 10% of WiFi bandwidth
    };
    
    ESP_ERROR_CHECK(rs485_bridge_init(&config));
    
    // Bridge now transparently forwards WiFi ↔ RS485
    // with compile-time traffic filtering
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));  // 10 seconds
        rs485_bridge_print_stats();
    }
}
```

## Filtering Rules

### Compile-Time Defines

```c
// In main.c or sdkconfig

#define BRIDGE_FILTER_ARP         // Define to enable ARP filtering
#define BRIDGE_FILTER_ICMP        // Define to enable ICMP filtering
#define BRIDGE_FILTER_TCP         // Define to enable TCP filtering
#define BRIDGE_FILTER_UDP         // Define to enable UDP filtering
#define BRIDGE_FILTER_PORTS        // Define to enable port-based filtering
#define BRIDGE_RATE_LIMIT         // Define to enable rate limiting
```

### Default Allowed Protocols

```c
// Default compile-time configuration
- ARP:      ALLOWED  // Required for IP resolution
- ICMP:     ALLOWED  // Allows ping
- TCP:      ALLOWED  // Web traffic, app protocols
- UDP:      BLOCKED  // Prevents broadcast floods
```

### Allowed Ports

```c
// Default compile-time configuration
.allowed_tcp_ports = {80, 443, 5000}  // HTTP, HTTPS, application
.allowed_udp_ports = {}                       // No UDP ports by default
```

## Rate Limiting Algorithm

**Token Bucket (1-second window):**

```
┌─────────────────────────────────────────┐
│ WiFi RX → Token Bucket Check   │
│              ↓                     │
│  Allow if within 10% quota   │
│              ↓                     │
│  Forward to RS485               │
└─────────────────────────────────────────┘
```

**How it works:**

1. Track bytes forwarded in last 1-second window
2. Calculate maximum: `(WiFi_BW_BPS * wifi_to_rs485_ratio) / 8`
3. If within quota → forward packet
4. If over quota → drop packet silently

**Example (100 Mbps WiFi):**
- Max to RS485: `(100,000,000 * 10%) / 8` = 1,250,000 bytes/s
- RS485 capacity: 1.7 KB/s
- 10% quota protects RS485 from WiFi overwhelming it

## Statistics

```c
rs485_bridge_print_stats();
```

Output:

```
=== Bridge Statistics ===
Wi-Fi/Ethernet → RS485: 12345
RS485 → Wi-Fi/Ethernet: 678
Filtered (protocol): 23
Filtered (rate limit): 12
=========================
```

## RS485 Network Protection

### Why Protection Needed

**Bandwidth Mismatch:**
- WiFi/Ethernet: 100-300 Mbps
- RS485: 19.2 kbps = 1.7 KB/s
- Ratio: ~6000:1

**Without protection:**
- WiFi network traffic can completely flood RS485
- RS485 CSMA/CD breaks down
- 32 RS485 nodes experience massive collisions
- Network becomes unusable

**With protection:**
- Rate limiting restricts WiFi→RS485 to 10% (configurable)
- Only essential protocols allowed (ARP, ICMP, TCP)
- UDP blocked by default (prevents broadcast storms)
- Specific ports only (HTTP, HTTPS, 5000)

### Protection Strategies

| Strategy | Implementation | Effectiveness |
|-----------|----------------|---------------|
| **Rate Limiting** | Token bucket (1-sec window) | Very effective |
| **Protocol Filtering** | Compile-time #define blocks | Very effective |
| **Port Filtering** | Compile-time port whitelist | Effective |
| **UDP Blocking** | Prevents broadcast floods | Effective |
| **Broadcast Suppression** | Count-based (not yet implemented) | N/A |

## API Reference

### Bridge API

```c
#include "rs485_bridge.h"

// Initialize bridge
esp_err_t rs485_bridge_init(rs485_bridge_config_t *config);

// Stop bridge
esp_err_t rs485_bridge_stop(void);

// Print statistics (compile-time optional)
void rs485_bridge_print_stats(void);
```

### Configuration Structure

```c
typedef struct {
    // Network interfaces
    bool use_wifi;             // Enable WiFi
    bool use_ethernet;          // Enable Ethernet
    
    // RS485 configuration (independent IP)
    uint8_t rs485_node_id;
    uint32_t rs485_baud_rate;
    int rs485_tx_pin;
    int rs485_rx_pin;
    int rs485_rts_pin;
    
    // Compile-time filtering (simple)
    bool filter_enabled;
    bool allow_arp;
    bool allow_icmp;
    bool allow_tcp;
    bool allow_udp;
    uint16_t allowed_tcp_ports[8];
    uint16_t allowed_udp_ports[8];
    
    // Rate limiting
    bool rate_limit_enabled;
    uint8_t wifi_to_rs485_ratio;  // 1-100% (10% = default)
} rs485_bridge_config_t;
```

## Hardware Requirements

- ESP32-S3 or ESP32
- WiFi module (built-in or external)
- RS485 transceiver (MAX485, SP3485)
- RS485 bus wiring:
  - 120Ω termination resistors at both ends
  - Bridge node: connects to RS485 bus as node 1
  - Other nodes: connect as nodes 2-32
- Optional: Ethernet module for Ethernet bridge

## Pin Configuration

### WiFi (Built-in)
- Uses default ESP32-S3 WiFi pins

### RS485
```
UART2: RS485
  - TX: GPIO 17
  - RX: GPIO 18
  - RTS: GPIO 19 (DE/RE control)
```

### Ethernet (Optional)
- Uses default ESP32-S3 Ethernet pins if enabled

## Known Limitations

- **No Runtime Filter Configuration**: Filtering is compile-time only (#define)
- **No Alerts**: No alerts when packets are filtered
- **Static Filter Rules**: Cannot change rules without recompiling
- **No UDP Broadcast Suppression**: Not yet implemented
- **Simple Rate Limiting**: 1-second window, no sophisticated algorithms
- **No Authentication**: Bridge management interface not secured

## Troubleshooting

### Bridge Not Forwarding

- Check if `rs485_bridge_init()` was called successfully
- Verify RS485 netif is initialized (should be automatic)
- Check statistics output
- Verify compile-time filter #defines are set correctly

### All Packets Being Dropped

- Check filter configuration (ensure desired protocols/ports are enabled)
- Reduce `wifi_to_rs485_ratio` if RS485 is being overwhelmed
- Check if `rate_limit_enabled` is true

### RS485 Network Issues

- Check RS485 termination (120Ω at both ends)
- Verify all nodes have unique IPs
- Check baud rate matches (19200)
- Check RS485 wiring (A/B polarity)

### WiFi Network Issues

- Verify WiFi connection is stable
- Check if WiFi IP is in 192.168.1.0/24 subnet
- Verify DHCP client is working (WiFi side)

## Usage Examples

### Example 1: Basic WiFi-to-RS485 Bridge

```c
rs485_bridge_config_t config = {
    .use_wifi = true,
    .use_ethernet = false,
    
    // RS485: Bridge is node 1
    .rs485_node_id = 1,
    .rs485_baud_rate = 19200,
    .rs485_tx_pin = 17,
    .rs485_rx_pin = 18,
    .rs485_rts_pin = 19,
    
    // Filtering: Allow essential traffic only
    .filter_enabled = true,
    .allow_arp = true,
    .allow_icmp = true,
    .allow_tcp = true,
    .allow_udp = false,
    .allowed_tcp_ports = {80, 443, 5000},
    .allowed_udp_ports = {},
    
    // Rate limiting: 10%
    .rate_limit_enabled = true,
    .wifi_to_rs485_ratio = 10,
};

ESP_ERROR_CHECK(rs485_bridge_init(&config));
```

### Example 2: Ethernet-to-RS485 Bridge

```c
rs485_bridge_config_t config = {
    .use_wifi = false,
    .use_ethernet = true,
    
    // RS485: Bridge is node 1
    .rs485_node_id = 1,
    .rs485_baud_rate = 19200,
    .rs485_tx_pin = 17,
    .rs485_rx_pin = 18,
    .rs485_rts_pin = 19,
    
    // Filtering
    .filter_enabled = true,
    .allow_arp = true,
    .allow_icmp = true,
    .allow_tcp = true,
    .allow_udp = false,
    .allowed_tcp_ports = {80, 443, 5000},
    .allowed_udp_ports = {},
    
    // Rate limiting: 20% (faster Ethernet)
    .rate_limit_enabled = true,
    .wifi_to_rs485_ratio = 20,
};

ESP_ERROR_CHECK(rs485_bridge_init(&config));
```

### Example 3: Aggressive Filtering (UDP Disabled)

```c
rs485_bridge_config_t config = {
    .use_wifi = true,
    .use_ethernet = false,
    
    .rs485_node_id = 1,
    .rs485_baud_rate = 19200,
    .rs485_tx_pin = 17,
    .rs485_rx_pin = 18,
    .rs485_rts_pin = 19,
    
    // Block all UDP to prevent floods
    .filter_enabled = true,
    .allow_udp = false,
    .allowed_udp_ports = {},
    
    // Allow only web traffic
    .allowed_tcp_ports = {80},
    
    // No rate limiting (aggressive mode)
    .rate_limit_enabled = false,
};

ESP_ERROR_CHECK(rs485_bridge_init(&config));
```

## Building and Flashing

```bash
cd rs485_bridge_example
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Resources for Improvement

- [ESP-IDF Programming Guide - ESP-NETIF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_netif_programming.html)
- [ESP-IDF WiFi Component](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/wifi.html)
- [ESP-IDF Ethernet Component](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/eth.html)
- [RS485 Protection TI App Note](https://www.ti.com/lit/SLLA639)
- [RS485 Surge Protection](https://www.analog.com/en/resources/analog-dialogue/articles/safeguard-your-rs485-communication-networks.html)
- [Protecting RS485 Networks](https://www.electronicspecifier.com/products/power/protect-industrial-rs485-networks/)

## License

Apache 2.0
