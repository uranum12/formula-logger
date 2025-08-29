#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <format>
#include <string>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/uart.h>
#include <pico.h>
#include <pico/binary_info.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include "bme280.h"
#include "mcp3208.h"

#define SPI_ID spi0
#define SPI_BAUD 1'000'000

#define UART_ID uart1
#define UART_BAUD 119'200

#define PIN_SPI_SCK 2
#define PIN_SPI_TX 3
#define PIN_SPI_RX 4
#define PIN_SPI_CS_BME280 5
#define PIN_SPI_CS_MCP3208 6

#define PIN_UART_TX 20
#define PIN_UART_RX 21
#define PIN_DE_RE 19

// clang-format off
bi_decl(bi_3pins_with_func(PIN_SPI_SCK, PIN_SPI_TX, PIN_SPI_RX, GPIO_FUNC_SPI));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_BME280, "SPI CS for bme280"));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3208, "SPI CS for mcp3208"));
bi_decl(bi_2pins_with_func(PIN_UART_TX, PIN_UART_RX, GPIO_FUNC_UART));
bi_decl(bi_1pin_with_name(PIN_DE_RE, "MAX485 Enable"));
// clang-format on

std::string escapeDoubleQuotes(const std::string& input) {
    std::string output;
    for (char c : input) {
        if (c == '"') {
            output += "\\\"";  // " を \" に置換
        } else {
            output += c;
        }
    }
    return output;
}

void uart_putc_blocking(uart_inst_t* uart, char c) {
    while (!uart_is_writable(uart)) {
        tight_loop_contents();
    }
    uart_putc_raw(uart, c);
}

void uart_puts_blocking(uart_inst_t* uart, const char* str) {
    while (*str) {
        uart_putc_blocking(uart, *str++);
    }
}

void msg_publish(const std::string& topic, const std::string& payload) {
    auto payload_escaped = escapeDoubleQuotes(payload);
    auto msg = std::format(R"({{"topic":"{}","payload":"{}"}})", topic,
                           payload_escaped) +
               "\n";

    gpio_put(PIN_DE_RE, 1);
    sleep_ms(1);
    uart_puts_blocking(UART_ID, msg.c_str());
    sleep_ms(1);
    gpio_put(PIN_DE_RE, 0);
    sleep_ms(1);
}

int main() {
    stdio_init_all();
    printf("start\n");

    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(PIN_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RX, GPIO_FUNC_UART);

    for (;;) {
        uart_puts(UART_ID, "{\"topic\":\"hello\",\"payload\":\"world\"}\n");
        printf("puts\n");
        sleep_ms(10 * 1000);
    }
}

int main_() {
    stdio_init_all();
    printf("start\n");

    spi_init(SPI_ID, SPI_BAUD);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_RX, GPIO_FUNC_SPI);

    gpio_init(PIN_SPI_CS_BME280);
    gpio_set_dir(PIN_SPI_CS_BME280, GPIO_OUT);
    gpio_put(PIN_SPI_CS_BME280, 1);

    gpio_init(PIN_SPI_CS_MCP3208);
    gpio_set_dir(PIN_SPI_CS_MCP3208, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3208, 1);

    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(PIN_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RX, GPIO_FUNC_UART);

    gpio_init(PIN_DE_RE);
    gpio_set_dir(PIN_DE_RE, GPIO_OUT);
    gpio_put(PIN_DE_RE, 0);

    bme280_reset();

    do {
        sleep_ms(1);
    } while (bme280_is_status_im_update(bme280_get_status()));

    if (uint8_t chip_id = bme280_get_chip_id();
        bme280_is_chip_id_valid(chip_id)) {
        printf("chip_id: %#x is valid\n", chip_id);
    }

    bme280_calib_data_t calib_data;
    bme280_get_calib_data(&calib_data);

    bme280_settings_t settings = {
        .osr_t = bme280_osr_x4,
        .osr_p = bme280_osr_x4,
        .osr_h = bme280_osr_x4,
        .filter = bme280_filter_x2,
        .standby_time = bme280_standby_time_1000ms,
    };
    bme280_set_settings(&settings);

    // if (cyw43_arch_init()) {
    //     printf("Failed to initialize.\n");
    //     return 1;
    // }

    // for (int i = 0; i < 3; i++) {
    //     cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    //     sleep_ms(200);
    //     cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    //     sleep_ms(200);
    // }

    bool is_bme280_measure = false;

    for (;;) {
        for (int i = 0; i < 10; i++) {
            bool is_bme280_raw_set = false;

            auto time_start = get_absolute_time();
            auto time_usec = to_us_since_boot(time_start);

            // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

            bme280_raw_data_t raw_data;
            if (i % 10 == 0) {
                bme280_set_mode(bme280_mode_forced);
                is_bme280_measure = true;
            } else if (is_bme280_measure &&
                       !bme280_is_status_measuring(bme280_get_status())) {
                bme280_get_raw_data(&raw_data);

                is_bme280_raw_set = true;
                is_bme280_measure = false;
            }

            uint16_t raw_x = mcp3208_get_raw(mcp3208_channel_single_ch0);
            uint16_t raw_y = mcp3208_get_raw(mcp3208_channel_single_ch1);
            uint16_t raw_z = mcp3208_get_raw(mcp3208_channel_single_ch2);

            double x = calc_kxr94_2050_g(raw_x);
            double y = calc_kxr94_2050_g(raw_y);
            double z = calc_kxr94_2050_g(raw_z);

            uint16_t raw_in = mcp3208_get_raw(mcp3208_channel_single_ch3);
            uint16_t raw_out = mcp3208_get_raw(mcp3208_channel_single_ch4);

            double in = calc_103jt_k(raw_in);
            double out = calc_103jt_k(raw_out);

            if (is_bme280_raw_set) {
                int32_t t_fine = 0;
                double temp = bme280_compensate_temperature(
                    raw_data.temperature, &calib_data, &t_fine);
                double pres = bme280_compensate_pressure(raw_data.pressure,
                                                         &calib_data, t_fine);
                double hum = bme280_compensate_humidity(raw_data.humidity,
                                                        &calib_data, t_fine);

                auto json_temp = std::format(
                    R"({{"usec":{},"temp":{:.2f},"pres":{:.2f},"hum":{:.2f}}})",
                    time_usec, temp, pres, hum);

                msg_publish("temp", json_temp);
            }

            auto json_water =
                std::format(R"({{"usec":{},"in":{:.2f},"out":{:.2f}}})",
                            time_usec, in, out);
            auto json_acc =
                std::format(R"({{"usec":{},"x":{:.2f},"y":{:.2f},"z":{:.2f}}})",
                            time_usec, x, y, z);

            msg_publish("water", json_water);
            msg_publish("acc", json_acc);

            // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

            // cyw43_arch_poll();
            tight_loop_contents();

            auto time_next = delayed_by_ms(time_start, 100);
            sleep_until(time_next);
        }
    }
}
