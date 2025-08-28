#include "mcp3204.h"

#include <stdint.h>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/time.h>

static inline void cs_select(uint8_t pin_cs) {
    asm volatile("nop \n nop \n nop");
    gpio_put(pin_cs, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(uint8_t pin_cs) {
    asm volatile("nop \n nop \n nop");
    gpio_put(pin_cs, 1);
    asm volatile("nop \n nop \n nop");
}

uint16_t mcp3204_get_raw(mcp3204_dev_t* dev, mcp3204_channel_t channel) {
    uint8_t tx_buf[3] = {
        0x02 | (channel >> 3),
        (channel & 0x07) << 5,
        0x00,
    };
    uint8_t rx_buf[3];

    cs_select(dev->pin_cs);
    spi_write_read_blocking(dev->spi_id, tx_buf, rx_buf, 3);
    cs_deselect(dev->pin_cs);

    return (rx_buf[1] & 0x0F) << 8 | rx_buf[2];
}

double calc_stroke(uint16_t raw) {
    // TODO: GPT出力そのままなので直す
    const float A = 0.02683150867279716f;
    const float B = -14.289689253066399f;
    const float R_fixed = 1000.0;

    if (raw >= 4095)
        raw = 4094;  // safety
    if (raw == 0)
        return 0.0f;  // 0除算防止: adc==0ならVout=0 -> R=0 => mm = A*0 + B
                      // (clampで0)
    float denom = (4095.0f - (float)raw);
    if (denom < 1e-6f)
        denom = 1e-6f;
    // R = R_fixed * adc / (4095 - adc)
    float R = R_fixed * ((float)raw / denom);
    float mm = A * R + B;
    if (mm < 0.0f)
        mm = 0.0f;
    if (mm > 54.0f)
        mm = 54.0f;
    return mm;
}
