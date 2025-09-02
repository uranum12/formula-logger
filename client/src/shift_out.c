#include "shift_out.h"

#include <stdint.h>

#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/time.h>

#include "shift_out.pio.h"

void shift_out_init(shift_out_dev_t* dev) {
    dev->offset = pio_add_program(dev->pio, &shift_out_program);

    dev->sm = pio_claim_unused_sm(dev->pio, true);

    pio_gpio_init(dev->pio, dev->pin_data);
    pio_gpio_init(dev->pio, dev->pin_clock);
    pio_sm_set_consecutive_pindirs(dev->pio, dev->sm, dev->pin_data, 1, true);
    pio_sm_set_consecutive_pindirs(dev->pio, dev->sm, dev->pin_clock, 1, true);

    pio_sm_config c = shift_out_program_get_default_config(dev->offset);
    sm_config_set_out_pins(&c, dev->pin_data, 1);
    sm_config_set_sideset_pins(&c, dev->pin_clock);
    sm_config_set_out_shift(&c, true, false, 8);
    sm_config_set_clkdiv(&c, 10.0f);

    pio_sm_init(dev->pio, dev->sm, dev->offset, &c);
    pio_sm_set_enabled(dev->pio, dev->sm, true);

    gpio_init(dev->pin_latch);
    gpio_set_dir(dev->pin_latch, GPIO_OUT);
    gpio_put(dev->pin_latch, 0);
}

void shift_out_send(shift_out_dev_t* dev, const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        pio_sm_put_blocking(dev->pio, dev->sm, buf[i]);
    }
    gpio_put(dev->pin_latch, 1);
    sleep_ms(1);
    gpio_put(dev->pin_latch, 0);
}
