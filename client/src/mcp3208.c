#include "mcp3208.h"

#include <stdint.h>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/time.h>

#define SPI_ID (spi0)
#define PIN_SPI_CS_MCP3208 (6)

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_SPI_CS_MCP3208, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_SPI_CS_MCP3208, 1);
    asm volatile("nop \n nop \n nop");
}

uint16_t mcp3208_get_raw(uint8_t channel) {
    uint8_t tx_buf[3] = {
        0x02 | (channel >> 3),
        (channel & 0x07) << 5,
        0x00,
    };
    uint8_t rx_buf[3];

    cs_select();
    spi_write_read_blocking(SPI_ID, tx_buf, rx_buf, 3);
    cs_deselect();
    sleep_us(10);

    return (rx_buf[1] & 0x0F) << 8 | rx_buf[2];
}
