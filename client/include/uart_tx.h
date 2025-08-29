#ifndef UART_TX_H
#define UART_TX_H

#include <stdint.h>

#include <hardware/pio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t pin;
    PIO pio;
    uint sm;
    uint offset;
} uart_tx_dev_t;

void uart_tx_init(uart_tx_dev_t* dev, uint8_t pin_tx, int baud);
void uart_tx_line(uart_tx_dev_t* dev, const char* s);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: UART_TX_H */
