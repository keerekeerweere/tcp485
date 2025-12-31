#ifndef RS485_FRAME_H
#define RS485_FRAME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define RS485_PREAMBLE      0x55
#define RS485_SFD          0xD5
#define RS485_MIN_FRAME     22
#define RS485_MAX_FRAME    1536
#define RS485_HDR_LEN       16
#define RS485_CRC_LEN       4
#define RS485_MAX_PAYLOAD   (RS485_MAX_FRAME - RS485_HDR_LEN - RS485_CRC_LEN)

#define MAC_ADDR_LEN       6
#define BROADCAST_MAC      {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

#define ETHERTYPE_IP       0x0800
#define ETHERTYPE_ARP      0x0806

typedef struct __attribute__((packed)) {
    uint8_t preamble;
    uint8_t sfd;
    uint8_t dest_mac[MAC_ADDR_LEN];
    uint8_t src_mac[MAC_ADDR_LEN];
    uint16_t ethertype;
    uint8_t payload[];
} rs485_frame_hdr_t;

typedef struct {
    rs485_frame_hdr_t hdr;
    uint32_t crc;
} rs485_frame_t;

uint32_t rs485_crc32(const uint8_t *data, size_t len);
esp_err_t rs485_frame_build(const uint8_t *dest_mac, const uint8_t *src_mac,
                            uint16_t ethertype, const uint8_t *payload,
                            size_t payload_len, uint8_t *out_frame, size_t *out_len);
esp_err_t rs485_frame_parse(const uint8_t *frame, size_t len,
                            rs485_frame_t *parsed, uint8_t **payload_ptr, size_t *payload_len);
bool rs485_is_broadcast_mac(const uint8_t *mac);
void rs485_generate_mac(uint8_t *mac, uint8_t node_id);

#endif
