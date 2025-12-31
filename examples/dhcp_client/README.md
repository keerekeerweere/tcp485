# DHCP Client Example

Demonstrates RS485 network interface with DHCP client support using ESP-IDF's built-in DHCP component.

## Overview

This example shows how to configure an RS485 node (2-32) as a DHCP client that obtains its IP address, netmask, gateway, and DNS settings from a DHCP server running on the RS485 network.

## Network Topology

```
┌─────────────────────────────────────────────────┐
│              RS485 Network (192.168.100.0/24)        │
├─────────────────────────────────────────────────┤
│                                                         │
│  Node 1 (DHCP Server)                                │
│  IP: 192.168.100.1                                       │
│  Pool: 192.168.100.2 - 192.168.100.32                │
│  └───► Nodes 2-32 send DHCP DISCOVER ──►         │
│                                                         │
│  Node 2-32 (DHCP Clients)                              │
│  ┌─────────────────────┐                                  │
│  │ Node 5 (Example) │ Broadcast DHCP DISCOVER                 │
│  │                   │ Receive DHCPOFFER from server        │
│  │                   │ Configure IP from lease                 │
│  │                   │ Send hostname (DHCP Option 12)           │
│  └─────────────────────┘                                  │
└─────────────────────────────────────────────────┘
```

## DHCP Client Behavior

1. **Initialization**: RS485_NETIF_INIT with `use_dhcp = true`
2. **DHCP DISCOVER**: Broadcasts to FF:FF:FF:FF:FF:FF:FF (RS485 broadcast MAC)
3. **DHCPOFFER**: Receives IP assignment from 192.168.100.1
4. **IP Configuration**: Applies IP, netmask, gateway from lease
5. **Event Handler**: `IP_EVENT_STA_GOT_IP` triggers on IP assignment
6. **Hostname**: Sent in DHCP Option 12 for server identification

## Configuration

### DHCP Client Mode (Node 2-32)

```c
rs485_netif_config_t netif_config = {
    .node_id = 5,
    .baud_rate = 19200,
    .uart_tx_pin = 17,
    .uart_rx_pin = 18,
    .uart_rts_pin = 19,
    .ip_addr = NULL,           // DHCP will assign
    .netmask = NULL,           // DHCP will assign
    .gateway = NULL,           // DHCP will assign
    .use_dhcp = true,         // Enable DHCP client
    .hostname = "node5",       // Sent in DHCP Option 12
    .dns_server1 = "192.168.100.1",  // From DHCP server
    .dns_server2 = NULL,
    .search_domain = "rs485.local"
};
```

### Static IP Mode (Node 1 / Future DHCP Server)

```c
rs485_netif_config_t netif_config = {
    .node_id = 1,
    .use_dhcp = false,        // Disable DHCP client
    .ip_addr = "192.168.100.1",   // Static IP
    .netmask = "255.255.255.0",
    .gateway = "192.168.100.1",
    .hostname = "gateway"
};
```

## DHCP Process

### DHCP DISCOVER Broadcast

```c
// DHCP client broadcasts to FF:FF:FF:FF:FF:FF:FF (all nodes)
// DHCP server on Node 1 receives and responds with DHCPOFFER
```

### DHCP DHCPOFFER (IP Assignment)

```
┌─────────────────────────────────────────┐
│ DHCPOFFER (From DHCP Server)       │
├─────────────────────────────────────────┤
│  Message Type: OFFER (2)          │
│  Your IP Address: 192.168.100.5      │
│  Subnet Mask: 255.255.255.0            │
│  Router (Gateway): 192.168.100.1    │
│  Lease Time: 86400 seconds (24h)       │
│  DNS Server: 192.168.100.1            │
└─────────────────────────────────────────┘
```

### DHCP REQUEST (Confirmation)

```c
// Client sends DHCP REQUEST to confirm IP assignment
// Server responds with DHCP ACK
```

## Building and Flashing

```bash
cd /home/dbram/work/tcp485
idf.py build

# Flash to RS485 network node 5 (DHCP client)
idf.py -p /dev/ttyUSB0 flash monitor
```

## Output Example

```
I (1234) DHCP_CLIENT: DHCP client started, waiting for IP assignment from 192.168.100.1...
I (2345) DHCP_CLIENT: Got IP: 192.168.100.5, mask: 255.255.255.0, gw: 192.168.100.1
I (3456) DHCP_CLIENT: DHCP client ready, can now use TCP/UDP sockets
```

## DHCP Options Sent by Client

| Option Code | Option Name | Value (Example) |
|-------------|--------------|----------------|
| 12 | Hostname | "node5" |
| 50 | Request IP | Client's requested IP |
| 53 | Message Type | DHCP Request |
| 55 | Parameter List | Client parameter list |

## DHCP Options Received from Server

| Option Code | Option Name | Value (Example) |
|-------------|--------------|----------------|
| 1 | Subnet Mask | 255.255.255.0 |
| 3 | Router | 192.168.100.1 |
| 6 | DNS Server | 192.168.100.1 |
| 51 | Lease Time | 86400 (24h) |
| 54 | Server ID | DHCP server identifier |
| 255 | End | End of options |

## Key Features

✅ **DHCP Client**: Uses ESP-IDF's built-in `esp_dhcp` component  
✅ **Hostname**: Sends hostname in DHCP Option 12  
✅ **IP Assignment**: Receives IP, netmask, gateway from server  
✅ **DNS Configuration**: Receives DNS server from server  
✅ **Event Handling**: `IP_EVENT_STA_GOT_IP` event on IP assignment  
✅ **Static IP Fallback**: Can configure static IP when `use_dhcp = false`  

## API Reference

```c
// Configuration
typedef struct {
    uint8_t node_id;
    uint32_t baud_rate;
    int uart_tx_pin;
    int uart_rx_pin;
    int uart_rts_pin;
    const char *ip_addr;      // NULL for DHCP, static IP otherwise
    const char *netmask;      // NULL for DHCP, static IP otherwise
    const char *gateway;      // NULL for DHCP, static IP otherwise
    bool use_dhcp;            // true = DHCP client, false = static IP
    const char *hostname;      // DHCP Option 12
    const char *dns_server1;
    const char *dns_server2;
    const char *search_domain;
} rs485_netif_config_t;

// Initialize RS485 network with DHCP client
esp_err_t rs485_netif_init(rs485_netif_config_t *config);

// Register IP event handler
esp_err_t esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, handler, NULL);
```

## Hardware Requirements

- ESP32 or ESP32-S3
- RS485 transceiver (MAX485, SP3485)
- RS485 bus with proper termination

## Notes

- **DHCP Server**: Not implemented in this version (user specified server-side management)
- **DHCP Relay**: Any node can forward DHCP traffic (not yet implemented)
- **Broadcast Address**: Uses FF:FF:FF:FF:FF:FF for DHCP DISCOVER
- **Hostname Option**: Required for DNS/DHCP server identification
- **Lease Time**: Configured by DHCP server (24h default)

## Troubleshooting

### No IP Assignment

- Verify RS485 wiring (A/B polarity)
- Verify baud rate (19200) on all nodes
- Check if DHCP server (192.168.100.1) is running
- Check UART TX/RX pins

### DHCP Server Not Responding

- Verify Node 1 has DHCP server running
- Check if server is configured for RS485 network
- Verify DHCP server IP pool includes client IP range

### IP Conflicts

- Check if multiple clients have same IP (DHCP server should prevent this)
- Verify server IP pool is correct (192.168.100.2-192.168.100.32 for clients)

## Comparison: Static IP vs DHCP

| Feature | Static IP | DHCP Client |
|----------|-----------|---------------|
| Configuration | Manual IP entry | Automatic from server |
| IP Conflicts | Possible if misconfigured | Server manages allocation |
| Network Changes | Manual reconfiguration | Automatic reassignment |
| Use Case | Small fixed networks | Dynamic, growing networks |
| Simplicity | Simple | Requires DHCP server |

## License

Apache 2.0
