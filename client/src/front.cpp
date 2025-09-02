#include <stdio.h>

#include <memory>

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <hardware/uart.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/mutex.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <pico/util/queue.h>

#include <cJSON.h>

#include "bme280.h"
#include "bno055.h"
#include "mcp3204.h"

#define STR_SIZE (512)
#define QUEUE_SIZE (64)

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

class Json {
public:
    Json() : root_(cJSON_CreateObject(), cJSON_Delete) {}

    void addNumber(const char* key, double value) {
        cJSON_AddNumberToObject(root_.get(), key, value);
    }
    void addString(const char* key, const char* value) {
        cJSON_AddStringToObject(root_.get(), key, value);
    }

    bool toBuffer(char* buf, int size) const {
        if (!buf || size == 0) {
            return false;
        }
        return cJSON_PrintPreallocated(root_.get(), buf, size, false);
    }

    cJSON* get() {
        return root_.get();
    }
    const cJSON* get() const {
        return root_.get();
    }

private:
    using CJSONPtr = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;
    CJSONPtr root_;
};

queue_t msg_queue;

void add_msg(const char* msg) {
    char buf[STR_SIZE];
    snprintf(buf, STR_SIZE, "%s", msg);
    if (!queue_try_add(&msg_queue, &buf)) {
        printf("full queue\n");
    }
}

void on_uart_rx() {
    static char buf[STR_SIZE];
    static int index = 0;

    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        buf[index] = ch;
        if (ch == '\n' || index >= STR_SIZE - 1) {
            buf[index] = '\0';
            add_msg(buf);
            index = 0;
        } else {
            ++index;
        }
    }
}

void msg_publish(const char* topic, const char* payload) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "topic", topic);
    cJSON_AddStringToObject(root, "payload", payload);

    char buf[STR_SIZE];
    cJSON_PrintPreallocated(root, buf, STR_SIZE, false);

    add_msg(buf);

    cJSON_Delete(root);
}

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

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 0);

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

    queue_init(&msg_queue, STR_SIZE, QUEUE_SIZE);
    multicore_launch_core1(core1_main);

    char buf[STR_SIZE];
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

            uint16_t af_raw =
                mcp3204_get_raw(&mcp3204, mcp3204_channel_diff_ch2_ch3);

            double left = left_raw * 3.3 / 4096;
            double right = right_raw * 3.3 / 4096;

            double af = af_raw * 3.3 / 4096;

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

                auto json_env = Json();
                json_env.addNumber("usec", time_usec);
                json_env.addNumber("temp", temp);
                json_env.addNumber("pres", pres);
                json_env.addNumber("hum", hum);
                json_env.toBuffer(buf, STR_SIZE);
                msg_publish("env", buf);
            }

            auto json_stroke_front = Json();
            json_stroke_front.addNumber("usec", time_usec);
            json_stroke_front.addNumber("left", left);
            json_stroke_front.addNumber("right", right);
            json_stroke_front.toBuffer(buf, STR_SIZE);
            msg_publish("stroke/front", buf);

            auto json_af = Json();
            json_af.addNumber("usec", time_usec);
            json_af.addNumber("af", af);
            json_af.toBuffer(buf, STR_SIZE);
            msg_publish("af", buf);

            auto json_acc = Json();
            json_acc.addNumber("usec", time_usec);
            json_acc.addNumber("ax", acc.x);
            json_acc.addNumber("ay", acc.y);
            json_acc.addNumber("az", acc.z);
            json_acc.addNumber("gx", gyro.x);
            json_acc.addNumber("gy", gyro.y);
            json_acc.addNumber("gz", gyro.z);
            json_acc.addNumber("mx", mag.x);
            json_acc.addNumber("my", mag.y);
            json_acc.addNumber("mz", mag.z);
            json_acc.addNumber("h", euler.heading);
            json_acc.addNumber("r", euler.roll);
            json_acc.addNumber("p", euler.pitch);
            json_acc.addNumber("qw", quaternion.w);
            json_acc.addNumber("qx", quaternion.x);
            json_acc.addNumber("qy", quaternion.y);
            json_acc.addNumber("qz", quaternion.z);
            json_acc.addNumber("lx", linear_accel.x);
            json_acc.addNumber("ly", linear_accel.y);
            json_acc.addNumber("lz", linear_accel.z);
            json_acc.addNumber("x", gravity.x);
            json_acc.addNumber("y", gravity.y);
            json_acc.addNumber("z", gravity.z);
            json_acc.addNumber("ss", status.sys);
            json_acc.addNumber("sg", status.gyro);
            json_acc.addNumber("sa", status.accel);
            json_acc.addNumber("sm", status.mag);
            json_acc.toBuffer(buf, STR_SIZE);
            msg_publish("acc", buf);

            gpio_put(PIN_LED, 0);

            auto time_next = delayed_by_ms(time_start, 100);
            sleep_until(time_next);
        }
    }
}
