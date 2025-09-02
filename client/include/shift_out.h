#ifndef SHIFT_OUT_H
#define SHIFT_OUT_H

#include <stdint.h>

#include <hardware/pio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PIO pio;
    uint sm;
    uint offset;
    uint8_t pin_data;
    uint8_t pin_clock;
    uint8_t pin_latch;
} shift_out_dev_t;

void shift_out_init(shift_out_dev_t* dev);
void shift_out_send(shift_out_dev_t* dev, const uint8_t* buf, size_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: SHIFT_OUT_H */
