#ifndef MCP3208_H
#define MCP3208_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    mcp3208_channel_single_ch0 = 0b1000,
    mcp3208_channel_single_ch1 = 0b1001,
    mcp3208_channel_single_ch2 = 0b1010,
    mcp3208_channel_single_ch3 = 0b1011,
    mcp3208_channel_single_ch4 = 0b1100,
    mcp3208_channel_single_ch5 = 0b1101,
    mcp3208_channel_single_ch6 = 0b1110,
    mcp3208_channel_single_ch7 = 0b1111,
    mcp3208_channel_diff_ch0_ch1 = 0b0000,
    mcp3208_channel_diff_ch1_ch0 = 0b0001,
    mcp3208_channel_diff_ch2_ch3 = 0b0010,
    mcp3208_channel_diff_ch3_ch2 = 0b0011,
    mcp3208_channel_diff_ch4_ch5 = 0b0100,
    mcp3208_channel_diff_ch5_ch4 = 0b0101,
    mcp3208_channel_diff_ch6_ch7 = 0b0110,
    mcp3208_channel_diff_ch7_ch6 = 0b0111,
} mcp3208_channel_t;

uint16_t mcp3208_get_raw(uint8_t channel);

double calc_kxr94_2050_g(uint16_t raw);
double calc_103jt_k(uint16_t raw);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: MCP3208_H */
