#ifndef MCP3204_H
#define MCP3204_H

#include <stdint.h>

#include <hardware/spi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    mcp3204_channel_single_ch0 = 0b1000,
    mcp3204_channel_single_ch1 = 0b1001,
    mcp3204_channel_single_ch2 = 0b1010,
    mcp3204_channel_single_ch3 = 0b1011,
    mcp3204_channel_diff_ch0_ch1 = 0b0000,
    mcp3204_channel_diff_ch1_ch0 = 0b0001,
    mcp3204_channel_diff_ch2_ch3 = 0b0010,
    mcp3204_channel_diff_ch3_ch2 = 0b0011,
} mcp3204_channel_t;

typedef struct {
    spi_inst_t* spi_id;
    uint8_t pin_cs;
} mcp3204_dev_t;

uint16_t mcp3204_get_raw(mcp3204_dev_t* dev, mcp3204_channel_t channel);

double calc_stroke(uint16_t raw);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: MCP3204_H */
