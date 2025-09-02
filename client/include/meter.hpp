#ifndef METER_HPP
#define METER_HPP

#include <stdint.h>

#include <optional>
#include <tuple>

constexpr uint8_t number_table[] = {
    0b11111100,  // 0
    0b01100000,  // 1
    0b11011010,  // 2
    0b11110010,  // 3
    0b01100110,  // 4
    0b10110110,  // 5
    0b10111110,  // 6
    0b11100000,  // 7
    0b11111110,  // 8
    0b11110110,  // 9
};

constexpr uint8_t meter_table[] = {
    0b00000000,  // 0
    0b10000000,  // 1
    0b11000000,  // 2
    0b11100000,  // 3
    0b11110000,  // 4
    0b11111000,  // 5
    0b11111100,  // 6
    0b11111110,  // 7
    0b11111111,  // 8
};

constexpr int level_thresholds[] = {
    // 0
    0,
    // 1
    3000,
    // 2
    4000,
    // 3
    5000,
    // 4
    6000,
    // 5
    7000,
    // 6
    8000,
    // 7
    9000,
    // 8
};

constexpr int number_table_len = sizeof(number_table) / sizeof(uint8_t);
constexpr int meter_table_len = sizeof(meter_table) / sizeof(uint8_t);
constexpr int level_thresholds_len = sizeof(level_thresholds) / sizeof(int);
static_assert(meter_table_len == level_thresholds_len + 1,
              "meter table and level thresholds length invalid");

uint8_t convertNumber(const int num);
uint8_t convertMeter(const int num);
int calcLevel(const int rpm);

std::optional<std::tuple<int, int>> parseUARTMessage(const char* str);
void fillBuf(int gear, int rpm, uint8_t* buf);

#endif /* end of include guard: METER_HPP */
