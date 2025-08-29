#include <pico/multicore.h>
#include <stdio.h>

#include <format>
#include <string>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/uart.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include "mcp3204.h"
#include "mcp3208.h"

#define SPI_ID (spi0)
#define SPI_BAUD (1'000'000)

#define UART_ID (uart1)
#define UART_BAUD (115'200)

#define PIN_SPI_SCK (2)
#define PIN_SPI_TX (3)
#define PIN_SPI_RX (4)
#define PIN_SPI_CS_MCP3208 (5)
#define PIN_SPI_CS_MCP3204 (6)

#define PIN_RPM (10)

#define PIN_UART_TX (20)
#define PIN_UART_RX (21)

#define PIN_LED (25)

volatile double measured_freq = 0;  // core0から読めるように共有変数に

void core1_main() {
    gpio_init(PIN_RPM);
    gpio_set_dir(PIN_RPM, GPIO_IN);

    uint32_t pulse_count = 0;
    absolute_time_t last_time = get_absolute_time();

    while (true) {
        // エッジ検出（立ち上がりを待つ）
        while (!gpio_get(PIN_RPM));
        while ( gpio_get(PIN_RPM));

        pulse_count++;

        // 一定時間ごとに周波数計算
        absolute_time_t now = get_absolute_time();
        uint64_t elapsed_us = absolute_time_diff_us(last_time, now);

        if (elapsed_us >= 1000000) {   // 1秒ゲート時間
            measured_freq = (double)pulse_count * 1e6 / elapsed_us;
            pulse_count = 0;
            last_time = now;
        }
    }
}

void msg_publish(const std::string& topic, const std::string& payload) {
    std::string escaped_payload;
    for (char c : payload) {
        if (c == '"') {
            escaped_payload += "\\\"";  // convert ["] to [\"]
        } else {
            escaped_payload += c;
        }
    }
    auto msg = std::format(R"({{"topic":"{}","payload":"{}"}})", topic,
                           escaped_payload) +
               "\n";
    printf("published %s\n", msg.c_str());
    uart_puts(UART_ID, msg.c_str());
}

int main() {
    stdio_init_all();
    printf("start\n");

    spi_init(SPI_ID, SPI_BAUD);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_RX, GPIO_FUNC_SPI);

    gpio_init(PIN_SPI_CS_MCP3208);
    gpio_set_dir(PIN_SPI_CS_MCP3208, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3208, 1);

    gpio_init(PIN_SPI_CS_MCP3204);
    gpio_set_dir(PIN_SPI_CS_MCP3204, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3204, 1);

    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(PIN_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RX, GPIO_FUNC_UART);

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 0);

    mcp3208_dev_t mcp3208 = {
        .spi_id = SPI_ID,
        .pin_cs = PIN_SPI_CS_MCP3208,
    };

    mcp3204_dev_t mcp3204 = {
        .spi_id = SPI_ID,
        .pin_cs = PIN_SPI_CS_MCP3204,
    };

    multicore_launch_core1(core1_main);

    for (;;) {
        gpio_put(PIN_LED, 1);

        auto time_start = get_absolute_time();
        auto time_usec = to_us_since_boot(time_start);

        uint16_t raw_in = mcp3204_get_raw(&mcp3204, mcp3204_channel_single_ch0);
        uint16_t raw_out =
            mcp3204_get_raw(&mcp3204, mcp3204_channel_single_ch1);

        double in = calc_103jt_k(raw_in);
        double out = calc_103jt_k(raw_out);

        uint16_t raw_ect =
            mcp3208_get_raw(&mcp3208, mcp3208_channel_single_ch0);
        uint16_t raw_tps =
            mcp3208_get_raw(&mcp3208, mcp3208_channel_single_ch1);
        uint16_t raw_iap =
            mcp3208_get_raw(&mcp3208, mcp3208_channel_single_ch2);
        uint16_t raw_gp =
            mcp3208_get_raw(&mcp3208, mcp3208_channel_diff_ch4_ch5);

        double ect = raw_ect * 3.3 / 4096;
        double tps = raw_tps * 3.3 / 4096;
        double iap = raw_iap * 3.3 / 4096;
        double gp = raw_gp * 3.3 / 4096;

        auto json_water = std::format(
            R"({{"usec":{},"inlet_temp":{:.2f},"outlet_temp":{:.2f}}})",
            time_usec, in, out);

        msg_publish("water", json_water);

        auto json_ecu = std::format(
            R"({{"usec":{},"ect":{:.2f},"tps":{:.2f},"iap":{:.2f},"gp":{:.2f},"rpm":{:.2f}}})",
            time_usec, ect, tps, iap, gp, measured_freq * 120);
        msg_publish("ecu", json_ecu);

        gpio_put(PIN_LED, 0);

        auto time_next = delayed_by_ms(time_start, 100);
        sleep_until(time_next);
    }
}
