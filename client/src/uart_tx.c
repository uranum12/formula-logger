/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/pio.h"
#include "uart_tx.pio.h"

#include "uart_tx.h"

// We're going to use PIO to print "Hello, world!" on the same GPIO which we
// normally attach UART0 to.

void uart_tx_init(uart_tx_dev_t* dev, uint8_t pin_tx, int baud) {
    dev->pin = pin_tx;
    // This is the same as the default UART baud rate on Pico

    // This will find a free pio and state machine for our program and load it
    // for us We use pio_claim_free_sm_and_add_program_for_gpio_range
    // (for_gpio_range variant) so we will get a PIO instance suitable for
    // addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
        &uart_tx_program, &dev->pio, &dev->sm, &dev->offset, pin_tx, 1, true);
    hard_assert(success);

    uart_tx_program_init(dev->pio, dev->sm, dev->offset, pin_tx, baud);
}

void uart_tx_line(uart_tx_dev_t* dev, const char* s) {
    uart_tx_program_puts(dev->pio, dev->sm, s);
}
