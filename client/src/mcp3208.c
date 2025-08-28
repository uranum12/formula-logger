#include "mcp3208.h"

#include <math.h>
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

uint16_t mcp3208_get_raw(mcp3208_dev_t* dev, uint8_t channel) {
    uint8_t tx_buf[3] = {
        0x04 | (channel >> 2),
        (channel & 0x03) << 6,
        0x00,
    };
    uint8_t rx_buf[3];

    cs_select(dev->pin_cs);
    spi_write_read_blocking(dev->spi_id, tx_buf, rx_buf, 3);
    cs_deselect(dev->pin_cs);
    sleep_us(10);

    return (rx_buf[1] & 0x0F) << 8 | rx_buf[2];
}

double calc_kxr94_2050_g(uint16_t raw) {
    const double v_ref = 3.3;
    const double v_0g = v_ref / 2;
    const double v_sensitivity = 0.66;

    double v = raw * v_ref / 4096;
    return (v - v_0g) / v_sensitivity;
}

double calc_103jt_k(uint16_t raw) {
    const double r_ref = 1.0;
    const double r0 = 10.0;
    const double b_value = 3435.0;
    const double t0 = 25.0;
    const double t_abs = 273.15;

    double r = r_ref * raw / (4095 - raw);
    return 1.0 / (1.0 / b_value * log(r / r0) + 1.0 / (t0 + t_abs));
}
