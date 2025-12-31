#include "rs485_frame.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "RS485_FRAME";

static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void crc32_init_table(void)
{
    uint32_t poly = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

uint32_t rs485_crc32(const uint8_t *data, size_t len)
{
    if (!crc32_table_initialized) {
        crc32_init_table();
    }

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

esp_err_t rs485_frame_build(const uint8_t *dest_mac, const uint8_t *src_mac,
                            uint16_t ethertype, const uint8_t *payload,
                            size_t payload_len, uint8_t *out_frame, size_t *out_len)
{
    if (dest_mac == NULL || src_mac == NULL || out_frame == NULL || out_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t total_len = RS485_HDR_LEN + payload_len + RS485_CRC_LEN;
    if (total_len > RS485_MAX_FRAME) {
        ESP_LOGE(TAG, "Frame too large: %d bytes (max %d)", total_len, RS485_MAX_FRAME);
        return ESP_ERR_INVALID_SIZE;
    }

    if (*out_len < total_len) {
        ESP_LOGE(TAG, "Output buffer too small: %d bytes (need %d)", *out_len, total_len);
        return ESP_ERR_INVALID_SIZE;
    }

    size_t pos = 0;
    out_frame[pos++] = RS485_PREAMBLE;
    out_frame[pos++] = RS485_SFD;
    memcpy(&out_frame[pos], dest_mac, MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;
    memcpy(&out_frame[pos], src_mac, MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;
    out_frame[pos++] = (ethertype >> 8) & 0xFF;
    out_frame[pos++] = ethertype & 0xFF;
    
    if (payload != NULL && payload_len > 0) {
        memcpy(&out_frame[pos], payload, payload_len);
        pos += payload_len;
    }

    uint32_t crc = rs485_crc32(&out_frame[2], pos - 2);
    memcpy(&out_frame[pos], &crc, RS485_CRC_LEN);
    pos += RS485_CRC_LEN;

    *out_len = pos;
    return ESP_OK;
}

esp_err_t rs485_frame_parse(const uint8_t *frame, size_t len,
                            rs485_frame_t *parsed, uint8_t **payload_ptr, size_t *payload_len)
{
    if (frame == NULL || len < RS485_MIN_FRAME) {
        return ESP_ERR_INVALID_ARG;
    }

    if (frame[0] != RS485_PREAMBLE || frame[1] != RS485_SFD) {
        ESP_LOGW(TAG, "Invalid preamble/SFD: %02X %02X", frame[0], frame[1]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (parsed != NULL) {
        parsed->hdr.preamble = frame[0];
        parsed->hdr.sfd = frame[1];
        memcpy(parsed->hdr.dest_mac, &frame[2], MAC_ADDR_LEN);
        memcpy(parsed->hdr.src_mac, &frame[8], MAC_ADDR_LEN);
        parsed->hdr.ethertype = (frame[14] << 8) | frame[15];
    }

    size_t payload_start = RS485_HDR_LEN;
    size_t payload_end = len - RS485_CRC_LEN;
    
    if (payload_end < payload_start) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint32_t received_crc;
    memcpy(&received_crc, &frame[len - RS485_CRC_LEN], RS485_CRC_LEN);
    
    uint32_t calculated_crc = rs485_crc32(&frame[2], payload_end - 2);
    if (received_crc != calculated_crc) {
        ESP_LOGW(TAG, "CRC mismatch: received=0x%08X, calculated=0x%08X", received_crc, calculated_crc);
        return ESP_ERR_INVALID_CRC;
    }

    if (parsed != NULL) {
        parsed->crc = received_crc;
    }

    if (payload_ptr != NULL) {
        *payload_ptr = (uint8_t *)&frame[payload_start];
    }

    if (payload_len != NULL) {
        *payload_len = payload_end - payload_start;
    }

    return ESP_OK;
}

bool rs485_is_broadcast_mac(const uint8_t *mac)
{
    if (mac == NULL) {
        return false;
    }
    
    const uint8_t broadcast[] = BROADCAST_MAC;
    return memcmp(mac, broadcast, MAC_ADDR_LEN) == 0;
}

void rs485_generate_mac(uint8_t *mac, uint8_t node_id)
{
    if (mac == NULL) {
        return;
    }
    
    mac[0] = 0x12;
    mac[1] = 0x34;
    mac[2] = 0x56;
    mac[3] = 0x78;
    mac[4] = 0x00;
    mac[5] = node_id;
}
