#include <stdint.h>
#include <stdio.h>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/uart.h>
#include <pico/multicore.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <pico/util/queue.h>

#include <cJSON.h>

#include "mcp3204.h"
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
#define PIN_SPI_CS_MCP3208 (5)
#define PIN_SPI_CS_MCP3204 (6)

#define PIN_RPM (10)

#define PIN_UART_TX (20)
#define PIN_UART_RX (21)

#define PIN_LED (25)

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

void msg_publish(const char* topic, const char* payload) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "topic", topic);
    cJSON_AddStringToObject(root, "payload", payload);

    char buf[STR_SIZE];
    cJSON_PrintPreallocated(root, buf, STR_SIZE, false);

    queue_try_add(&msg_queue, &buf);

    cJSON_Delete(root);
}

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

    queue_init(&msg_queue, STR_SIZE, QUEUE_SIZE);
    multicore_launch_core1(core1_main);

    char buf[STR_SIZE];
    for (;;) {
        gpio_put(PIN_LED, 1);

        auto time_start = get_absolute_time();

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

        if (last_time_us != 0 &&
            (to_us_since_boot(time_start) - last_time_us) > 200'000) {
            frequency = 0.0f;
        }

        auto json_water = Json();
        json_water.addTime(time_start);
        json_water.addNumber("inlet_temp", in);
        json_water.addNumber("outlet_temp", out);
        json_water.toBuffer(buf, STR_SIZE);
        msg_publish("water", buf);

        auto json_ecu = Json();
        json_ecu.addTime(time_start);
        json_ecu.addNumber("ect", ect);
        json_ecu.addNumber("tps", tps);
        json_ecu.addNumber("iap", iap);
        json_ecu.addNumber("gp", gp);
        json_ecu.addNumber("rpm", frequency * 120);
        json_ecu.toBuffer(buf, STR_SIZE);
        msg_publish("ecu", buf);

        gpio_put(PIN_LED, 0);

        auto time_next = delayed_by_ms(time_start, 100);
        sleep_until(time_next);
    }
}
