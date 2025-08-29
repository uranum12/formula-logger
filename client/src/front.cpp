#include <stdio.h>

#include <format>
#include <queue>
#include <string>

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <hardware/uart.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/mutex.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include "bme280.h"
#include "bno055.h"
#include "mcp3204.h"
#include "uart_tx.h"

#define SPI_ID (spi0)
#define SPI_BAUD (1'000'000)

#define I2C_ID (i2c1)
#define I2C_BAUD (400'000)
#define I2C_ADDR_BNO055 (0x28)

#define UART_ID (uart1)
#define UART_BAUD (115'200)

#define PIN_SPI_SCK (2)
#define PIN_SPI_TX (3)
#define PIN_SPI_RX (4)
#define PIN_SPI_CS_BME280 (5)
#define PIN_SPI_CS_MCP3204 (6)

#define PIN_I2C_SDA (14)
#define PIN_I2C_SCL (15)

#define PIN_UART_TX (20)

#define PIN_UART_RX (21)

#define PIN_LED (25)

// bi_decl(bi_3pins_with_func(PIN_SPI_SCK, PIN_SPI_TX, PIN_SPI_RX,
// GPIO_FUNC_SPI)); bi_decl(bi_1pin_with_name(PIN_SPI_CS_BME280, "SPI CS for
// bme280")); bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3204, "SPI CS for
// mcp3204")); bi_decl(bi_2pins_with_func(PIN_I2C_SDA, PIN_I2C_SCL,
// GPIO_FUNC_I2C)); bi_decl(bi_2pins_with_func(PIN_UART_TX, PIN_UART_RX,
// GPIO_FUNC_UART)); bi_decl(bi_1pin_with_name(PIN_LED, "LED"));

mutex_t queue_mutex;
std::queue<std::string> msg_queue;

void add_msg(const std::string& msg) {
    mutex_enter_blocking(&queue_mutex);
    if (msg_queue.size() > 1000) {
        msg_queue.pop();
    }
    msg_queue.push(msg);
    mutex_exit(&queue_mutex);
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
    printf("published %s", msg.c_str());

    add_msg(msg);
}

uart_tx_dev_t dev;

void core1_main() {
    for (;;) {
        std::string msg;
        if (uart_is_readable(UART_ID)) {
            uint8_t buf;
            do {
                buf = uart_getc(UART_ID);
                msg += buf;
            } while (buf != '\n');
            add_msg(msg);
        }

        std::vector<std::string> batch;
        mutex_enter_blocking(&queue_mutex);
        while (!msg_queue.empty()) {
            batch.push_back(std::move(msg_queue.front()));
            msg_queue.pop();
        }
        mutex_exit(&queue_mutex);

        for (auto& m : batch) {
            uart_tx_line(&dev, m.c_str());
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

    gpio_init(PIN_SPI_CS_BME280);
    gpio_set_dir(PIN_SPI_CS_BME280, GPIO_OUT);
    gpio_put(PIN_SPI_CS_BME280, 1);

    gpio_init(PIN_SPI_CS_MCP3204);
    gpio_set_dir(PIN_SPI_CS_MCP3204, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3204, 1);

    i2c_init(I2C_ID, I2C_BAUD);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);

    uart_init(UART_ID, UART_BAUD);
    // gpio_set_function(PIN_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RX, GPIO_FUNC_UART);

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 0);

    uart_tx_init(&dev, PIN_UART_TX, UART_BAUD);

    bme280_dev_t bme280 = {
        .spi_id = SPI_ID,
        .pin_cs = PIN_SPI_CS_BME280,
    };

    bme280_reset(&bme280);

    do {
        sleep_ms(1);
    } while (bme280_is_status_im_update(bme280_get_status(&bme280)));

    if (uint8_t chip_id = bme280_get_chip_id(&bme280);
        bme280_is_chip_id_valid(chip_id)) {
        printf("chip_id: %#x is valid\n", chip_id);
    }

    bme280_calib_data_t calib_data;
    bme280_get_calib_data(&bme280, &calib_data);

    bme280_settings_t settings = {
        .osr_t = bme280_osr_x4,
        .osr_p = bme280_osr_x4,
        .osr_h = bme280_osr_x4,
        .filter = bme280_filter_x2,
        .standby_time = bme280_standby_time_1000ms,
    };
    bme280_set_settings(&bme280, &settings);

    mcp3204_dev_t mcp3204 = {
        .spi_id = SPI_ID,
        .pin_cs = PIN_SPI_CS_MCP3204,
    };

    bno055_dev_t bno055 = {
        .i2c_id = I2C_ID,
        .addr = I2C_ADDR_BNO055,
    };

    sleep_ms(1000);

    if (uint8_t chip_id = bno055_get_chip_id(&bno055);
        bno055_is_chip_id_valid(chip_id)) {
        printf("chip_id: %#x is valid\n", chip_id);
    }

    bno055_set_mode(&bno055, bno055_mode_ndof);
    sleep_ms(50);

    bool is_bme280_measure = false;

    mutex_init(&queue_mutex);
    multicore_launch_core1(core1_main);

    for (;;) {
        for (int i = 0; i < 10; i++) {
            gpio_put(PIN_LED, 1);
            bool is_bme280_raw_set = false;

            auto time_start = get_absolute_time();
            auto time_usec = to_us_since_boot(time_start);

            bme280_raw_data_t raw_data;
            if (i % 10 == 0) {
                bme280_set_mode(&bme280, bme280_mode_forced);
                is_bme280_measure = true;
            } else if (is_bme280_measure && !bme280_is_status_measuring(
                                                bme280_get_status(&bme280))) {
                bme280_get_raw_data(&bme280, &raw_data);
                is_bme280_raw_set = true;
                is_bme280_measure = false;
            }

            uint16_t left_raw =
                mcp3204_get_raw(&mcp3204, mcp3204_channel_single_ch0);
            uint16_t right_raw =
                mcp3204_get_raw(&mcp3204, mcp3204_channel_single_ch1);

            double left = calc_stroke(left_raw);
            double right = calc_stroke(right_raw);

            bno055_accel_t acc;
            bno055_gyro_t gyro;
            bno055_mag_t mag;
            bno055_euler_t euler;
            bno055_quaternion_t quaternion;
            bno055_linear_accel_t linear_accel;
            bno055_gravity_t gravity;
            bno055_calib_status_t status;
            bno055_read_accel(&bno055, &acc);
            bno055_read_gyro(&bno055, &gyro);
            bno055_read_mag(&bno055, &mag);
            bno055_read_euler(&bno055, &euler);
            bno055_read_quaternion(&bno055, &quaternion);
            bno055_read_linear_accel(&bno055, &linear_accel);
            bno055_read_gravity(&bno055, &gravity);
            bno055_read_calib_status(&bno055, &status);

            if (is_bme280_raw_set) {
                int32_t t_fine = 0;
                double temp = bme280_compensate_temperature(
                    raw_data.temperature, &calib_data, &t_fine);
                double pres = bme280_compensate_pressure(raw_data.pressure,
                                                         &calib_data, t_fine);
                double hum = bme280_compensate_humidity(raw_data.humidity,
                                                        &calib_data, t_fine);

                auto json_env = std::format(
                    R"({{"usec":{},"temp":{:.2f},"pres":{:.2f},"hum":{:.2f}}})",
                    time_usec, temp, pres, hum);
                msg_publish("env", json_env);
            }

            auto json_stroke_front =
                std::format(R"({{"usec":{},"left":{:.2f},"right":{:.2f}}})",
                            time_usec, left, right);
            msg_publish("stroke/front", json_stroke_front);

            auto json_acc = std::format(R"({{"usec":{},"x":{},"y":{},"z":{}}})",
                                        time_usec, acc.x, acc.y, acc.z);
            msg_publish("acc", json_acc);

            gpio_put(PIN_LED, 0);

            auto time_next = delayed_by_ms(time_start, 100);
            sleep_until(time_next);
        }
    }
}
