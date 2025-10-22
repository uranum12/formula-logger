#include <stdint.h>
#include <stdio.h>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/uart.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <pico/util/queue.h>

#include <cJSON.h>

#include "mcp3208.h"

#include "json.hpp"

#define STR_SIZE (512)
#define QUEUE_SIZE (32)

#define ALPHA (0.2)

#define SPI_ID (spi0)
#define SPI_BAUD (1'000'000)

#define UART_ID (uart1)
#define UART_BAUD (115'200)

#define PIN_SPI_SCK (2)
#define PIN_SPI_TX (3)
#define PIN_SPI_RX (4)
#define PIN_SPI_CS_MCP3208_ECU (5)
#define PIN_SPI_CS_MCP3208_1 (6)
#define PIN_SPI_CS_MCP3208_2 (7)

#define PIN_RPM (9)

#define PIN_UART_TX (20)
#define PIN_UART_RX (21)
#define PIN_RS485_ENABLE (22)

#define PIN_LED (25)

bi_decl(bi_3pins_with_func(PIN_SPI_SCK, PIN_SPI_TX, PIN_SPI_RX, GPIO_FUNC_SPI));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3208_ECU,
                          "SPI CS for mcp3208 for ECU"));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3208_1, "SPI CS for mcp3208 1"));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3208_2, "SPI CS for mcp3208 2"));
bi_decl(bi_1pin_with_name(PIN_RPM, "RPM"));
bi_decl(bi_2pins_with_func(PIN_UART_TX, PIN_UART_RX, GPIO_FUNC_UART));
bi_decl(bi_1pin_with_name(PIN_LED, "LED"));

volatile uint64_t last_time_us = 0;
volatile double frequency = 0.0;

void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now_us = to_us_since_boot(get_absolute_time());

    if (last_time_us != 0) {
        uint32_t dt_us = now_us - last_time_us;
        double freq_new = 1000000.0f / dt_us;

        frequency = ALPHA * freq_new + (1.0f - ALPHA) * frequency;
    }

    last_time_us = now_us;
}

queue_t msg_queue;

void core1_main() {
    gpio_init(PIN_RPM);
    gpio_set_dir(PIN_RPM, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_RPM, GPIO_IRQ_EDGE_RISE, true,
                                       gpio_callback);

    uart_init(UART_ID, UART_BAUD);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, true);
    uart_set_hw_flow(UART_ID, false, false);
    gpio_set_function(PIN_UART_TX, UART_FUNCSEL_NUM(UART_ID, PIN_UART_TX));
    gpio_set_function(PIN_UART_RX, UART_FUNCSEL_NUM(UART_ID, PIN_UART_RX));

    gpio_init(PIN_RS485_ENABLE);
    gpio_set_dir(PIN_RS485_ENABLE, GPIO_OUT);
    gpio_put(PIN_RS485_ENABLE, 1);

    char str[STR_SIZE];

    for (;;) {
        if (queue_try_remove(&msg_queue, &str)) {
            uart_puts(UART_ID, str);
            uart_putc(UART_ID, '\n');
        }
    }
}

int main() {
    stdio_init_all();
    printf("start\n");

    spi_init(SPI_ID, SPI_BAUD);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_RX, GPIO_FUNC_SPI);

    gpio_init(PIN_SPI_CS_MCP3208_ECU);
    gpio_set_dir(PIN_SPI_CS_MCP3208_ECU, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3208_ECU, 1);

    gpio_init(PIN_SPI_CS_MCP3208_1);
    gpio_set_dir(PIN_SPI_CS_MCP3208_1, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3208_1, 1);

    gpio_init(PIN_SPI_CS_MCP3208_2);
    gpio_set_dir(PIN_SPI_CS_MCP3208_2, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3208_2, 1);

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 0);

    mcp3208_dev_t mcp3208_ecu = {
        .spi_id = SPI_ID,
        .pin_cs = PIN_SPI_CS_MCP3208_ECU,
    };

    mcp3208_dev_t mcp3208_1 = {
        .spi_id = SPI_ID,
        .pin_cs = PIN_SPI_CS_MCP3208_1,
    };

    queue_init(&msg_queue, STR_SIZE, QUEUE_SIZE);
    multicore_launch_core1(core1_main);

    char buf[STR_SIZE];
    for (;;) {
        for (int i = 0; i < 4; i++) {
            auto time_start = get_absolute_time();

            gpio_put(PIN_LED, 1);

            if (i % 10 == 0) {
                // water (10hz)
                uint16_t raw_in =
                    mcp3208_get_raw(&mcp3208_1, mcp3208_channel_single_ch0);
                uint16_t raw_out =
                    mcp3208_get_raw(&mcp3208_1, mcp3208_channel_single_ch1);

                double in = calc_103jt_k(raw_in);
                double out = calc_103jt_k(raw_out);

                auto json_water = Json("water");
                json_water.addTime(get_absolute_time());
                json_water.add("inlet_temp", in);
                json_water.add("outlet_temp", out);
                json_water.toBuffer(buf, STR_SIZE);
                queue_try_add(&msg_queue, &buf);

                // stroke/rear (10hz)
                uint16_t raw_right =
                    mcp3208_get_raw(&mcp3208_1, mcp3208_channel_single_ch2);
                uint16_t raw_left =
                    mcp3208_get_raw(&mcp3208_1, mcp3208_channel_single_ch3);

                double right = raw_right * 3.3 / 4096;
                double left = raw_left * 3.3 / 4096;

                auto json_stroke_rear = Json("stroke/rear");
                json_stroke_rear.addTime(get_absolute_time());
                json_stroke_rear.add("right", right);
                json_stroke_rear.add("left", left);
                json_stroke_rear.toBuffer(buf, STR_SIZE);
                queue_try_add(&msg_queue, &buf);
            }

            // ECU (100hz)
            // uint16_t raw_ect =
            //     mcp3208_get_raw(&mcp3208_ecu, mcp3208_channel_single_ch0);
            // uint16_t raw_tps =
            //     mcp3208_get_raw(&mcp3208_ecu, mcp3208_channel_single_ch1);
            // uint16_t raw_iap =
            //     mcp3208_get_raw(&mcp3208_ecu, mcp3208_channel_single_ch2);
            // uint16_t raw_gp =
            //     mcp3208_get_raw(&mcp3208_ecu, mcp3208_channel_diff_ch4_ch5);
            //
            // double ect = raw_ect * 5.0 / 4096;
            // double tps = raw_tps * 5.0 / 4096;
            // double iap = raw_iap * 5.0 / 4096;
            // double gp = raw_gp * 5.0 / 4096;
            //
            // auto json_ecu = Json("ecu");
            // json_ecu.addTime(get_absolute_time());
            // json_ecu.add("ect", ect);
            // json_ecu.add("tps", tps);
            // json_ecu.add("iap", iap);
            // json_ecu.add("gp", gp);
            // json_ecu.toBuffer(buf, STR_SIZE);
            // queue_try_add(&msg_queue, &buf);
            //
            // // RPM (20hz)
            // if (last_time_us != 0 &&
            //     (to_us_since_boot(time_start) - last_time_us) > 200'000) {
            //     frequency = 0.0f;
            // }
            //
            // if (i % 5 == 0) {
            //     auto json_rpm = Json("rpm");
            //     json_rpm.addTime(get_absolute_time());
            //     json_rpm.add("rpm", frequency * 120);
            //     json_rpm.toBuffer(buf, STR_SIZE);
            //     queue_try_add(&msg_queue, &buf);
            // }

            gpio_put(PIN_LED, 0);

            auto time_next = delayed_by_ms(time_start, 1000);
            sleep_until(time_next);
        }
    }
}
