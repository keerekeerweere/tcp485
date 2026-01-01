# TCP/IP over RS485 Setup Guide

This document provides detailed instructions for setting up and running the TCP/IP over RS485 project.

## Overview

This project implements a TCP/IP stack over RS485 using a custom MAC layer with CSMA/CD collision detection. It's compatible with ESP32/ESP32-S3 and uses ESP-IDF framework.

## Prerequisites

### Hardware Requirements
- ESP32 or ESP32-S3 microcontroller
- RS485 transceiver (e.g., MAX485, SP3485)
- RS485 bus wiring:
  - Node 1-32: TX/RX connected to A/B lines
  - 120Ω termination resistors at both ends
  - Optional bias resistors for idle state

### Software Requirements
- ESP-IDF v5.0 or later (preferably v5.5.2 for latest features and bug fixes)
- CMake 3.5 or later
- Python virtual environment (already created)

## Installation Steps

### 1. Setup ESP-IDF Environment

First, you need to source the ESP-IDF environment:
```bash
. $HOME/esp/esp-idf/export.sh
```

If you don't have ESP-IDF installed:
```bash
mkdir -p $HOME/esp
cd $HOME/esp
git clone -b v5.5.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
source export.sh
```

### 2. Clone and Setup Project

```bash
cd /path/to/tcp485
```

### 3. Build Project

Compile the project with:
```bash
idf.py build
```

### 4. Flash and Monitor

Flash to your device and monitor output:
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Known Compatibility Issues

### ESP-IDF Version Mismatch
The project was originally developed for ESP-IDF v5.0. Newer versions (v5.5.2) have removed certain components and changed APIs, which may cause build errors.

If you encounter errors like:
```
Failed to resolve component 'esp_dhcp' required by component 'rs485_netif':
unknown name.
```

This is because:
1. `esp_dhcp` component no longer exists in newer ESP-IDF versions
2. DHCP client functionality has been restructured in ESP-IDF v5.x

**Workaround:** You may need to modify component dependencies and DHCP code to use modern ESP-IDF APIs. In newer versions, DHCP functionality is typically integrated with `esp_netif`.

## Configuration Details

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

## Network Topology

### Wiring Requirements
- TX line connects to A (data+) on transceiver
- RX line connects to B (data-) on transceiver  
- RTS line controls DE/RE (Driver Enable/Receiver Enable) on transceiver
- 120Ω termination resistors at both ends of the bus
- Proper grounding and power supply

### Default Pin Configuration
```
UART2: RS485
  - TX: GPIO 17
  - RX: GPIO 18
  - RTS: GPIO 19 (DE/RE control)
```

## Running Examples

The project includes several example applications:

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

## Performance Metrics

| Metric | Value |
|--------|-------|
| Max throughput | 1.7 KB/s (19200 baud) |
| Ping latency | 13-28ms (no collisions) |
| TCP connect | 30-50ms |
| Collision probability | <5% (32 nodes) |

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
    ├── dhcp_client/
    ├── ping_test/
    ├── tcp_server/
    ├── tcp_client/
    └── udp_broadcast/
```

## License

Apache 2.0

## References

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/lwip.html)
- [lwIP Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/lwip.html)
- [ESP-NETIF Programming Manual](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_netif_programming.html)