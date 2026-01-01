#include "rs485_mac.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char *TAG = "RS485_MAC";

void csma_cd_init(csma_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    memset(ctx, 0, sizeof(csma_context_t));
    ctx->state = CSMA_IDLE;
}

bool csma_cd_detect_collision(rs485_phy_t *phy)
{
    if (phy == NULL) {
        return false;
    }

    return rs485_phy_is_busy(phy);
}

void csma_cd_send_jam(rs485_phy_t *phy)
{
    if (phy == NULL) {
        return;
    }

    uint8_t jam_pattern[JAM_BITS / 8];
    memset(jam_pattern, 0xFF, sizeof(jam_pattern));
    rs485_phy_transmit(phy, jam_pattern, sizeof(jam_pattern));
    uart_wait_tx_done(phy->uart_num, pdMS_TO_TICKS(100));
}

uint32_t csma_cd_backoff_delay(uint8_t attempt)
{
    uint8_t k = (attempt < 10) ? attempt : 10;
    uint32_t max_slots = (1 << k) - 1;
    uint32_t random_slots = esp_random() % (max_slots + 1);
    return random_slots * MAC_SLOT_TIME_MS;
}

esp_err_t csma_cd_transmit(rs485_phy_t *phy, const uint8_t *frame, size_t len, csma_context_t *ctx)
{
    if (phy == NULL || frame == NULL || ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ctx->retry_count = 0;
    ctx->collision_detected = false;
    ctx->state = CSMA_WAITING;

    while (ctx->retry_count < MAX_RETRIES) {
        while (rs485_phy_is_busy(phy)) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        vTaskDelay(pdMS_TO_TICKS(INTER_FRAME_GAP_MS));

        ctx->state = CSMA_TRANSMITTING;
        esp_err_t ret = rs485_phy_transmit(phy, frame, len);

        if (ret == ESP_OK) {
            ctx->transmissions++;
            ctx->tx_frames++;
            ctx->state = CSMA_IDLE;
            return ESP_OK;
        } else {
            ctx->collision_detected = true;
            ctx->collisions++;
            
            ESP_LOGW(TAG, "Collision detected, attempt %d/%d", ctx->retry_count + 1, MAX_RETRIES);

            csma_cd_send_jam(phy);
            ctx->state = CSMA_JAMMING;
            
            uint32_t backoff = csma_cd_backoff_delay(ctx->retry_count);
            ESP_LOGD(TAG, "Backoff: %d ms", backoff);
            vTaskDelay(pdMS_TO_TICKS(backoff));

            ctx->state = CSMA_BACKOFF;
            ctx->retry_count++;
        }
    }

    ctx->tx_errors++;
    ctx->state = CSMA_IDLE;
    ESP_LOGE(TAG, "Max retries reached, frame dropped");
    return ESP_ERR_TIMEOUT;
}

void csma_cd_print_stats(const csma_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ESP_LOGI(TAG, "CSMA/CD Stats: TX=%d, Collisions=%d, Errors=%d, Success=%.1f%%",
             ctx->tx_frames, ctx->collisions, ctx->tx_errors,
             ctx->transmissions > 0 ? (100.0 * ctx->tx_frames / ctx->transmissions) : 0.0);
}

void csma_cd_get_stats(const csma_context_t *ctx, uint32_t *collisions, uint32_t *tx_frames, uint32_t *tx_errors)
{
    if (ctx == NULL) {
        return;
    }

    if (collisions != NULL) {
        *collisions = ctx->collisions;
    }

    if (tx_frames != NULL) {
        *tx_frames = ctx->tx_frames;
    }

    if (tx_errors != NULL) {
        *tx_errors = ctx->tx_errors;
    }
}
