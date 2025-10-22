#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pio.h>
#include <hardware/spi.h>
#include <hardware/uart.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/mutex.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <pico/util/queue.h>

#include <cJSON.h>
#include <cmp.h>

// #include "bme280.h"
// #include "bno055.h"
#include "crc16.h"
#include "mcp3208.h"
#include "shift_out.h"
#include "spi_slave.h"

#include "json.hpp"
#include "meter.hpp"

#define STR_SIZE (512)
#define QUEUE_SIZE (32)

#define SPI_ID (spi0)
#define SPI_BAUD (1'000'000)

#define I2C_ID (i2c0)
#define I2C_BAUD (400'000)
#define I2C_ADDR_BNO055 (0x28)

#define UART_ID (uart1)
#define UART_BAUD (115'200)

#define PIO_ID (pio0)

#define PIN_SPI_SCK (2)
#define PIN_SPI_TX (3)
#define PIN_SPI_RX (4)
#define PIN_SPI_CS_MCP3208_1 (5)
#define PIN_SPI_CS_MCP3208_2 (6)
#define PIN_SPI_CS_BME280 (7)

#define PIN_I2C_SDA (8)
#define PIN_I2C_SCL (9)

#define PIN_74HC595_DATA (18)
#define PIN_74HC595_CLOCK (19)
#define PIN_74HC595_LATCH (26)

#define PIN_UART_TX (20)
#define PIN_UART_RX (21)
#define PIN_RS485_ENABLE (22)

#define PIN_LED (25)

bi_decl(bi_3pins_with_func(PIN_SPI_SCK, PIN_SPI_TX, PIN_SPI_RX, GPIO_FUNC_SPI));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3208_1, "SPI CS for mcp3208 1"));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3208_2, "SPI CS for mcp3208 2"));
// bi_decl(bi_2pins_with_func(PIN_I2C_SDA, PIN_I2C_SCL, GPIO_FUNC_I2C));
bi_decl(bi_1pin_with_name(PIN_74HC595_DATA, "74HC595 data"));
bi_decl(bi_1pin_with_name(PIN_74HC595_CLOCK, "74HC595 clock"));
bi_decl(bi_1pin_with_name(PIN_74HC595_LATCH, "74HC595 latch"));
bi_decl(bi_2pins_with_func(PIN_UART_TX, PIN_UART_RX, GPIO_FUNC_UART));
bi_decl(bi_1pin_with_name(PIN_LED, "LED"));

typedef struct {
    uint8_t* buf;
    size_t size;
    size_t pos;
} mem_t;

static bool mem_read(cmp_ctx_t* ctx, void* data, size_t limit) {
    mem_t* m = (mem_t*)ctx->buf;
    if (m->pos + limit > m->size) {
        return false;
    }
    memcpy(data, &m->buf[m->pos], limit);
    m->pos += limit;
    return true;
}

static bool mem_skip(cmp_ctx_t* ctx, size_t count) {
    mem_t* m = (mem_t*)ctx->buf;
    if (m->pos + count > m->size) {
        return false;
    }
    m->pos += count;
    return true;
}

static size_t mem_write(cmp_ctx_t* ctx, const void* data, size_t count) {
    mem_t* m = (mem_t*)ctx->buf;
    if (m->pos + count > m->size) {
        return 0;
    }
    memcpy(&m->buf[m->pos], data, count);
    m->pos += count;
    return count;
}

queue_t uart_queue;

// shift_out_dev_t shift_out = {
//     .pio = PIO_ID,
//     .pin_data = PIN_74HC595_DATA,
//     .pin_clock = PIN_74HC595_CLOCK,
//     .pin_latch = PIN_74HC595_LATCH,
// };

void on_uart_rx() {
    static char buf[STR_SIZE];
    static int index = 0;

    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        buf[index] = ch;
        if (ch == '\n' || index >= STR_SIZE - 1) {
            buf[index] = '\0';
            queue_try_add(&uart_queue, &buf);
            index = 0;
        } else {
            ++index;
        }
    }
}

// void msg_publish(const char* topic, const char* payload) {
//     cJSON* root = cJSON_CreateObject();
//     cJSON_AddStringToObject(root, "topic", topic);
//     cJSON_AddStringToObject(root, "payload", payload);
//
//     char buf[STR_SIZE];
//     cJSON_PrintPreallocated(root, buf, STR_SIZE, false);
//
//     queue_try_add(&msg_queue, &buf);
//
//     cJSON_Delete(root);
// }

void core1_main() {
    uart_init(UART_ID, UART_BAUD);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, true);
    uart_set_hw_flow(UART_ID, false, false);
    gpio_set_function(PIN_UART_TX, UART_FUNCSEL_NUM(UART_ID, PIN_UART_TX));
    gpio_set_function(PIN_UART_RX, UART_FUNCSEL_NUM(UART_ID, PIN_UART_RX));

    int uart_irq = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, on_uart_rx);
    irq_set_enabled(uart_irq, true);
    uart_set_irq_enables(UART_ID, true, false);

    gpio_init(PIN_RS485_ENABLE);
    gpio_set_dir(PIN_RS485_ENABLE, GPIO_OUT);
    gpio_put(PIN_RS485_ENABLE, 0);

    spi_slave_init();

    char str[STR_SIZE];
    // int gear, rpm;
    // bool meter_update = false;

    for (;;) {
        if (queue_try_remove(&uart_queue, &str)) {
            printf("%s\n", str);
            cJSON* root = cJSON_Parse(str);
            if (!root) {
                printf("JSON parse error\n");
                continue;
            }

            uint8_t buf[SPI_SLAVE_BUF_SIZE];
            memset(buf, 0x00, SPI_SLAVE_BUF_SIZE);
            mem_t mem = {buf, SPI_SLAVE_BUF_SIZE, 4};
            cmp_ctx_t cmp;

            cmp_init(&cmp, &mem, mem_read, mem_skip, mem_write);
            bool ok = true;

            const char* topic =
                cJSON_GetStringValue(cJSON_GetObjectItem(root, "topic"));
            cJSON* payload = cJSON_GetObjectItem(root, "payload");

            int size = cJSON_GetArraySize(payload);

            ok &= cmp_write_map(&cmp, 2);
            ok &= cmp_write_str(&cmp, "topic", 5);
            ok &= cmp_write_str(&cmp, topic, strlen(topic));
            ok &= cmp_write_str(&cmp, "payload", 7);
            ok &= cmp_write_map(&cmp, size);

            cJSON* item = payload->child;
            while (item) {
                ok &= cmp_write_str(&cmp, item->string, strlen(item->string));

                if (cJSON_IsNumber(item)) {
                    double d = item->valuedouble;
                    double i;
                    if (modf(d, &i) == 0.0) {
                        ok &= cmp_write_integer(&cmp, (int64_t)i);
                    } else {
                        ok &= cmp_write_double(&cmp, d);
                    }
                } else if (cJSON_IsBool(item)) {
                    ok &= cmp_write_bool(&cmp, cJSON_IsTrue(item));
                } else if (cJSON_IsString(item)) {
                    ok &= cmp_write_str(&cmp, item->valuestring,
                                        strlen(item->valuestring));
                } else {
                    ok &= cmp_write_nil(&cmp);
                }

                item = item->next;
            }

            cJSON_Delete(root);

            if (!ok) {
                printf("msgpack serialize error\n");
                printf("%s\n", cmp_strerror(&cmp));
                continue;
            }

            int length = mem.pos - 4;
            int crc = crc16(buf + 4, length);

            buf[0] = length >> 8;
            buf[1] = length & 0xFF;
            buf[2] = crc >> 8;
            buf[3] = crc & 0xFF;

            spi_slave_push_bytes(buf);

            // auto gear_opt = parseGear(str);
            // if (gear_opt) {
            //     gear = *gear_opt;
            //     meter_update = true;
            // }
            //
            // auto rpm_opt = parseRPM(str);
            // if (rpm_opt) {
            //     rpm = *rpm_opt;
            //     meter_update = true;
            // }
            //
            // if (meter_update) {
            //     uint8_t buf[6];
            //     fillBuf(gear, rpm, buf);
            //     shift_out_send(&shift_out, buf, 6);
            //     meter_update = false;
            // }
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

    gpio_init(PIN_SPI_CS_MCP3208_1);
    gpio_set_dir(PIN_SPI_CS_MCP3208_1, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3208_1, 1);

    gpio_init(PIN_SPI_CS_MCP3208_2);
    gpio_set_dir(PIN_SPI_CS_MCP3208_2, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3208_2, 1);

    gpio_init(PIN_SPI_CS_BME280);
    gpio_set_dir(PIN_SPI_CS_BME280, GPIO_OUT);
    gpio_put(PIN_SPI_CS_BME280, 1);

    i2c_init(I2C_ID, I2C_BAUD);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 0);

    mcp3208_dev_t mcp3208_1 = {
        .spi_id = SPI_ID,
        .pin_cs = PIN_SPI_CS_MCP3208_1,
    };

    mcp3208_dev_t mcp3208_2 = {
        .spi_id = SPI_ID,
        .pin_cs = PIN_SPI_CS_MCP3208_2,
    };

    // bme280_dev_t bme280 = {
    //     .spi_id = SPI_ID,
    //     .pin_cs = PIN_SPI_CS_BME280,
    // };
    //
    // bme280_reset(&bme280);
    //
    // do {
    //     sleep_ms(1);
    // } while (bme280_is_status_im_update(bme280_get_status(&bme280)));
    //
    // if (uint8_t chip_id = bme280_get_chip_id(&bme280);
    //     bme280_is_chip_id_valid(chip_id)) {
    //     printf("chip_id: %#x is valid\n", chip_id);
    // }
    //
    // bme280_calib_data_t calib_data;
    // bme280_get_calib_data(&bme280, &calib_data);
    //
    // bme280_settings_t settings = {
    //     .osr_t = bme280_osr_x4,
    //     .osr_p = bme280_osr_x4,
    //     .osr_h = bme280_osr_x4,
    //     .filter = bme280_filter_x2,
    //     .standby_time = bme280_standby_time_1000ms,
    // };
    // bme280_set_settings(&bme280, &settings);

    // bno055_dev_t bno055 = {
    //     .i2c_id = I2C_ID,
    //     .addr = I2C_ADDR_BNO055,
    // };

    // sleep_ms(1000);

    // if (uint8_t chip_id = bno055_get_chip_id(&bno055);
    //     bno055_is_chip_id_valid(chip_id)) {
    //     printf("chip_id: %#x is valid\n", chip_id);
    // }
    //
    // bno055_set_mode(&bno055, bno055_mode_ndof);
    // sleep_ms(50);

    // shift_out_init(&shift_out);

    // bool is_bme280_measure = false;

    queue_init(&uart_queue, STR_SIZE, QUEUE_SIZE);
    multicore_launch_core1(core1_main);

    char buf[STR_SIZE];
    for (;;) {
        for (int i = 0; i < 10; i++) {
            gpio_put(PIN_LED, 1);

            auto time_start = get_absolute_time();

            // if (i % 10 == 0) {
            //     bme280_set_mode(&bme280, bme280_mode_forced);
            //     is_bme280_measure = true;
            // } else if (is_bme280_measure && !bme280_is_status_measuring(
            //                                     bme280_get_status(&bme280)))
            //                                     {
            //     bme280_raw_data_t raw_data;
            //     bme280_get_raw_data(&bme280, &raw_data);
            //     is_bme280_measure = false;
            //
            //     int32_t t_fine = 0;
            //     double temp = bme280_compensate_temperature(
            //         raw_data.temperature, &calib_data, &t_fine);
            //     double pres = bme280_compensate_pressure(raw_data.pressure,
            //                                              &calib_data,
            //                                              t_fine);
            //     double hum = bme280_compensate_humidity(raw_data.humidity,
            //                                             &calib_data, t_fine);
            //
            //     auto json_env = Json();
            //     json_env.addTime(get_absolute_time());
            //     json_env.addNumber("temp", temp);
            //     json_env.addNumber("pres", pres);
            //     json_env.addNumber("hum", hum);
            //     json_env.toBuffer(buf, STR_SIZE);
            //     msg_publish("env", buf);
            // }

            uint16_t left_raw =
                mcp3208_get_raw(&mcp3208_1, mcp3208_channel_single_ch0);
            uint16_t right_raw =
                mcp3208_get_raw(&mcp3208_1, mcp3208_channel_single_ch1);

            double left = left_raw * 3.3 / 4096;
            double right = right_raw * 3.3 / 4096;

            // auto json_stroke_front = Json();
            // json_stroke_front.addTime(get_absolute_time());
            // json_stroke_front.addNumber("left", left);
            // json_stroke_front.addNumber("right", right);
            // json_stroke_front.toBuffer(buf, STR_SIZE);
            // msg_publish("stroke/front", buf);
            {
                uint8_t buf[SPI_SLAVE_BUF_SIZE];
                memset(buf, 0x00, SPI_SLAVE_BUF_SIZE);
                mem_t mem = {buf, SPI_SLAVE_BUF_SIZE, 4};
                cmp_ctx_t cmp;

                cmp_init(&cmp, &mem, mem_read, mem_skip, mem_write);

                bool ok = true;
                ok &= cmp_write_map(&cmp, 2);
                ok &= cmp_write_str(&cmp, "topic", 5);
                ok &= cmp_write_str(&cmp, "stroke/front", 12);
                ok &= cmp_write_str(&cmp, "payload", 7);
                ok &= cmp_write_map(&cmp, 2);
                ok &= cmp_write_str(&cmp, "left", 4);
                ok &= cmp_write_double(&cmp, left);
                ok &= cmp_write_str(&cmp, "right", 5);
                ok &= cmp_write_double(&cmp, right);

                if (!ok) {
                    printf("msgpack serialize error\n");
                    printf("%s\n", cmp_strerror(&cmp));
                } else {
                    int length = mem.pos - 4;
                    int crc = crc16(buf + 4, length);

                    buf[0] = length >> 8;
                    buf[1] = length & 0xFF;
                    buf[2] = crc >> 8;
                    buf[3] = crc & 0xFF;

                    spi_slave_push_bytes(buf);
                }
            }

            // uint16_t af_raw =
            //     mcp3204_get_raw(&mcp3204, mcp3204_channel_diff_ch2_ch3);
            //
            // double af = af_raw * 3.3 / 4096;
            //
            // auto json_af = Json();
            // json_af.addTime(get_absolute_time());
            // json_af.addNumber("af", af);
            // json_af.toBuffer(buf, STR_SIZE);
            // msg_publish("af", buf);

            // bno055_accel_t acc;
            // bno055_gyro_t gyro;
            // bno055_mag_t mag;
            // bno055_euler_t euler;
            // bno055_quaternion_t quaternion;
            // bno055_linear_accel_t linear_accel;
            // bno055_gravity_t gravity;
            // bno055_calib_status_t status;
            // bno055_read_accel(&bno055, &acc);
            // bno055_read_gyro(&bno055, &gyro);
            // bno055_read_mag(&bno055, &mag);
            // bno055_read_euler(&bno055, &euler);
            // bno055_read_quaternion(&bno055, &quaternion);
            // bno055_read_linear_accel(&bno055, &linear_accel);
            // bno055_read_gravity(&bno055, &gravity);
            // bno055_read_calib_status(&bno055, &status);

            // auto json_acc = Json();
            // json_acc.addTime(get_absolute_time());
            // json_acc.addNumber("ax", acc.x);
            // json_acc.addNumber("ay", acc.y);
            // json_acc.addNumber("az", acc.z);
            // json_acc.addNumber("gx", gyro.x);
            // json_acc.addNumber("gy", gyro.y);
            // json_acc.addNumber("gz", gyro.z);
            // json_acc.addNumber("mx", mag.x);
            // json_acc.addNumber("my", mag.y);
            // json_acc.addNumber("mz", mag.z);
            // json_acc.addNumber("h", euler.heading);
            // json_acc.addNumber("r", euler.roll);
            // json_acc.addNumber("p", euler.pitch);
            // json_acc.addNumber("qw", quaternion.w);
            // json_acc.addNumber("qx", quaternion.x);
            // json_acc.addNumber("qy", quaternion.y);
            // json_acc.addNumber("qz", quaternion.z);
            // json_acc.addNumber("lx", linear_accel.x);
            // json_acc.addNumber("ly", linear_accel.y);
            // json_acc.addNumber("lz", linear_accel.z);
            // json_acc.addNumber("x", gravity.x);
            // json_acc.addNumber("y", gravity.y);
            // json_acc.addNumber("z", gravity.z);
            // json_acc.addNumber("ss", status.sys);
            // json_acc.addNumber("sg", status.gyro);
            // json_acc.addNumber("sa", status.accel);
            // json_acc.addNumber("sm", status.mag);
            // json_acc.toBuffer(buf, STR_SIZE);
            // msg_publish("acc", buf);

            gpio_put(PIN_LED, 0);

            auto time_next = delayed_by_ms(time_start, 10000);
            sleep_until(time_next);
        }
    }
}
