#ifndef RS485_MAC_H
#define RS485_MAC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "rs485_phy.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_RETRIES            16
#define SLOT_TIME_MS           51
#define INTER_FRAME_GAP_MS     5
#define JAM_BITS              32

typedef enum {
    CSMA_IDLE,
    CSMA_WAITING,
    CSMA_TRANSMITTING,
    CSMA_BACKOFF,
    CSMA_JAMMING
} csma_state_t;

typedef struct {
    csma_state_t state;
    uint8_t retry_count;
    uint32_t backoff_time;
    bool collision_detected;
    uint32_t collisions;
    uint32_t transmissions;
    uint32_t tx_frames;
    uint32_t tx_errors;
} csma_context_t;

void csma_cd_init(csma_context_t *ctx);
esp_err_t csma_cd_transmit(rs485_phy_t *phy, const uint8_t *frame, size_t len, csma_context_t *ctx);
bool csma_cd_detect_collision(rs485_phy_t *phy);
void csma_cd_send_jam(rs485_phy_t *phy);
uint32_t csma_cd_backoff_delay(uint8_t attempt);
void csma_cd_print_stats(const csma_context_t *ctx);
void csma_cd_get_stats(const csma_context_t *ctx, uint32_t *collisions, uint32_t *tx_frames, uint32_t *tx_errors);

#endif
