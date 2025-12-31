# TCP/IP over RS485 with Custom MAC Layer

> **DISCLAIMER**: This code is AI-generated and has NOT been tested on hardware. It is provided as a thought experiment and reference implementation only. Use at your own risk. This code may contain errors, bugs, or incomplete functionality.

**Feedback & Improvements Are Welcome!**
This is an experimental implementation intended as a reference. If you encounter issues, find bugs, or have suggestions for improvements, please provide feedback. Bug reports, security fixes, and enhancements are encouraged. You are free to modify, patch, or improve any part of this code.

**Security Considerations**
- This implementation has NOT undergone security review or penetration testing
- RS485 is a shared medium - consider network segmentation if used in sensitive environments
- Custom MAC protocol has NOT been audited for vulnerabilities
- Review and harden the implementation before production use
- Use appropriate authentication and encryption for sensitive data

**Testing Recommendations**
- Start with small-scale testing (2-3 nodes) before deploying full 32-node network
- Verify proper RS485 termination (120Ω resistors at both ends)
- Monitor collision rates and adjust CSMA/CD parameters as needed
- Test DHCP client behavior with various server configurations
- Verify DNS resolution works correctly with your DHCP server

**Resources for Improvement**
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/lwip.html)
- [lwIP Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/lwip.html)
- [ESP-NETIF Programming Manual](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_netif_programming.html)
- [ESP-IDF Examples](https://github.com/espressif/esp-idf/tree/master/examples)

**Known Limitations**
- DHCP server not implemented (client-side only)
- No DHCP relay/switch support (server-side would need this)
- No dynamic lease table management (client relies on server)
- Single network interface support (no multi-interface DNS per interface)
- Limited to RS485 physical layer constraints (19200 baud)

**License**
Apache 2.0

---

Complete implementation of a TCP/IP stack over RS485 using a custom MAC layer with CSMA/CD collision detection. Compatible with ESP32/ESP32-S3 and ESP-IDF.

Complete implementation of a TCP/IP stack over RS485 using a custom MAC layer with CSMA/CD collision detection. Compatible with ESP32/ESP32-S3 and ESP-IDF.

## Architecture

```
Layer 4 (Transport): TCP Sockets | UDP Datagrams
                           ↓
Layer 3 (Internet):   IP Routing | ARP | ICMP (ping)
                           ↓
Layer 2 (MAC):        RS485 CSMA/CD + MAC Addressing (Custom)
                           ↓
Layer 1 (Physical):   RS485 Half-Duplex UART @ 19200 baud
```

## Features

- **Full TCP/IP Stack**: lwIP with ARP, ICMP (ping), TCP, UDP
- **Custom MAC Protocol**: IEEE 802.3-inspired frame format
- **CSMA/CD**: Carrier Sense Multiple Access with Collision Detection and exponential backoff
- **Up to 32 Nodes**: RS485 bus support for 32 devices
- **Standard Sockets API**: BSD socket interface for applications
- **CRC-32**: Frame integrity checking

## Hardware Requirements

- ESP32 or ESP32-S3
- RS485 transceiver (e.g., MAX485, SP3485)
- RS485 bus wiring:
  - Node 1-32: TX/RX connected to A/B lines
  - 120Ω termination resistors at both ends
  - Optional bias resistors for idle state

## Pin Configuration (Default)

```
UART2: RS485
  - TX: GPIO 17
  - RX: GPIO 18
  - RTS: GPIO 19 (DE/RE control)
```

## Software Requirements

- ESP-IDF v5.0 or later
- CMake 3.5 or later

## Building and Flashing

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Configure project
cd tcp485
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configuration (menuconfig)

```
Serial Flasher Config → Default serial port
Component config → LWIP → Enable ICMP, TCP, UDP
Component config → ESP NETIF → TCP/IP stack
```

## Usage

### Node Configuration

Edit `main/main.c` to configure your node:

```c
#define NODE_ID 5                    // Node ID (1-32)
#define RS485_BAUD_RATE 19200       // UART baud rate

rs485_netif_config_t netif_config = {
    .node_id = NODE_ID,
    .baud_rate = RS485_BAUD_RATE,
    .uart_tx_pin = 17,
    .uart_rx_pin = 18,
    .uart_rts_pin = 19,
    .ip_addr = "192.168.100.5",    // Unique IP for this node
    .netmask = "255.255.255.0",
    .gateway = "192.168.100.1"
};
```

### IP Addressing Scheme

Each node should have a unique IP in the 192.168.100.0/24 subnet:
- Node 1: 192.168.100.1
- Node 2: 192.168.100.2
- ...
- Node 32: 192.168.100.32

### MAC Addressing

MAC addresses are auto-generated as: `12:34:56:78:00:XX` where XX is the node ID.

### DNS Configuration

Configure DNS server(s) and hostname for name resolution:

```c
rs485_netif_config_t netif_config = {
    .node_id = NODE_ID,
    .baud_rate = RS485_BAUD_RATE,
    .uart_tx_pin = 17,
    .uart_rx_pin = 18,
    .uart_rts_pin = 19,
    .ip_addr = "192.168.100.5",
    .netmask = "255.255.255.0",
    .gateway = "192.168.100.1",
    .dns_server1 = "192.168.100.1",   // Primary DNS server
    .dns_server2 = NULL,               // Optional secondary DNS
    .hostname = "node5",              // This node's hostname
    .search_domain = "rs485.local"    // DNS search domain
};
```

**DNS Configuration Options:**

1. **Single DNS server**: Use `dns_server1` only
2. **Primary + Secondary DNS**: Set both `dns_server1` and `dns_server2` for failover
3. **Search domain**: Set `search_domain` to enable short hostname resolution (e.g., `node10` → `node10.rs485.local`)
4. **Hostname**: Set this node's hostname (used for DHCP/DNS identification)

**DNS Resolution Examples:**

```c
// Full hostname (always works)
rs485_ping_test("node10.rs485.local", 4);

// Short hostname (requires search_domain)
rs485_ping_test("node10", 4);  // Resolves as "node10.rs485.local"

// IP address (bypasses DNS)
rs485_ping_test("192.168.100.10", 4);

// Programmatic DNS lookup
#include "lwip/netdb.h"

struct addrinfo hints = {0};
hints.ai_family = AF_INET;
hints.ai_socktype = SOCK_STREAM;

struct addrinfo *res;
int err = getaddrinfo("node10.rs485.local", "80", &hints, &res);
if (err == 0) {
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    printf("Resolved to: %s\n", inet_ntoa(addr->sin_addr));
    freeaddrinfo(res);
}
```

**Note**: DNS server configuration is global (not per-interface). If using multiple network interfaces, configure DNS carefully to avoid conflicts.

## Example Applications

### DHCP Client Example

See `examples/dhcp_client/` for complete DHCP client implementation and configuration.

### DNS Resolution

```c
esp_err_t rs485_dns_lookup_example(const char *hostname);
esp_err_t rs485_dns_get_server_info(void);
```

### Ping Test

```c
rs485_ping_test("192.168.100.10", 4);  // Ping 4 times
```

### TCP Server (Echo)

```c
rs485_tcp_server_start(5000);  // Listen on port 5000
```

### UDP Broadcast

```c
rs485_udp_broadcast_start(5000);  // Broadcast every 1 second
```

## Frame Format

```
┌─────────┬────┬──────────────┬──────────────┬────────┬─────────────┬─────────┐
│Preamble │SFD │ Dest MAC (6) │ Src MAC (6)  │Type(2) │  Payload    │ CRC-32  │
│ (0x55)  │0xD5│              │              │0x0800  │ (IP packet) │ (4)     │
│ 1 byte  │ 1  │              │              │  (2)   │ 1-1500      │         │
└─────────┴────┴──────────────┴──────────────┴────────┴─────────────┴─────────┘
```

## CSMA/CD Parameters

- **Inter-frame gap**: 5ms (96 bit-times @ 19200 baud)
- **Slot time**: 51ms
- **Max retries**: 16
- **Backoff**: Exponential, k = min(10, attempt)

## Performance

| Metric | Value |
|--------|-------|
| Max throughput | 1.7 KB/s (19200 baud) |
| Ping latency | 13-28ms (no collisions) |
| TCP connect | 30-50ms |
| Collision probability | <5% (32 nodes) |

## Troubleshooting

### No communication between nodes
- Check RS485 wiring (A/B polarity)
- Verify termination resistors (120Ω at both ends)
- Confirm node IDs are unique
- Check IP addresses don't conflict

### High collision rate
- Reduce network load
- Increase inter-frame gap
- Check for electrical noise
- Verify RS485 bias resistors

### Frames not received
- Check UART pin configuration
- Verify baud rate matches on all nodes
- Check RS485 transceiver DE/RE control

## Project Structure

```
tcp485/
├── main/                    # Main application
├── components/
│   ├── rs485_phy/         # UART physical layer
│   ├── rs485_frame/       # Frame building/parsing + CRC32
│   ├── rs485_mac/         # CSMA/CD implementation
│   └── rs485_netif/       # ESP-NETIF integration + examples
└── examples/
    ├── ping_test/
    ├── tcp_server/
    ├── tcp_client/
    └── udp_broadcast/
```

## API Reference

### Network Interface

```c
esp_err_t rs485_netif_init(rs485_netif_config_t *config);
esp_err_t rs485_netif_deinit(void);
void rs485_netif_print_stats(void);
```

### Physical Layer

```c
esp_err_t rs485_phy_init(rs485_phy_t *phy);
esp_err_t rs485_phy_transmit(rs485_phy_t *phy, const uint8_t *data, size_t len);
int rs485_phy_receive(rs485_phy_t *phy, uint8_t *buf, size_t max_len, TickType_t timeout);
bool rs485_phy_is_busy(rs485_phy_t *phy);
```

### Frame Layer

```c
uint32_t rs485_crc32(const uint8_t *data, size_t len);
esp_err_t rs485_frame_build(const uint8_t *dest_mac, const uint8_t *src_mac,
                            uint16_t ethertype, const uint8_t *payload,
                            size_t payload_len, uint8_t *out_frame, size_t *out_len);
esp_err_t rs485_frame_parse(const uint8_t *frame, size_t len,
                            rs485_frame_t *parsed, uint8_t **payload_ptr, size_t *payload_len);
```

### CSMA/CD

```c
void csma_cd_init(csma_context_t *ctx);
esp_err_t csma_cd_transmit(rs485_phy_t *phy, const uint8_t *frame, size_t len, csma_context_t *ctx);
void csma_cd_print_stats(const csma_context_t *ctx);
```

### DNS Resolution

```c
esp_err_t rs485_dns_lookup_example(const char *hostname);
esp_err_t rs485_dns_get_server_info(void);
```

## License

Apache 2.0

## References

- [ESP-NETIF Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_netif.html)
- [lwIP Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/lwip.html)
- [IEEE 802.3 Ethernet Standard](https://standards.ieee.org/standard/802_3-2022.html)
