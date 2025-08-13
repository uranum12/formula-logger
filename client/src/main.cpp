#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <format>
#include <string>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <lwip/apps/mqtt.h>
#include <pico/binary_info.h>
#include <pico/cyw43_arch.h>
#include <pico/stdio.h>
#include <pico/time.h>

#define SPI_ID spi0
#define SPI_BAUD 1'000'000

#define PIN_SPI_SCK 2
#define PIN_SPI_TX 3
#define PIN_SPI_RX 4
#define PIN_SPI_CS_BME280 5
#define PIN_SPI_CS_MCP3204 6

#define WIFI_SSID "Buffalo-G-4810"
#define WIFI_PASSWORD "password"

#define MQTT_BROKER_IP "192.168.11.10"
#define MQTT_BROKER_PORT 1883

typedef enum {
    osr_skip = 0x00,
    osr_x1 = 0x01,
    osr_x2 = 0x02,
    osr_x4 = 0x03,
    osr_x8 = 0x04,
    osr_x16 = 0x05,
} osr_t;

typedef enum {
    filter_off = 0x00,
    filter_x2 = 0x01,
    filter_x4 = 0x02,
    filter_x8 = 0x03,
    filter_x16 = 0x04,
} filter_t;

typedef enum {
    standby_time_0p5ms = 0x00,
    standby_time_62p5ms = 0x01,
    standby_time_125ms = 0x02,
    standby_time_250ms = 0x03,
    standby_time_500ms = 0x04,
    standby_time_1000ms = 0x05,
    standby_time_10ms = 0x06,
    standby_time_20ms = 0x07,
} standby_time_t;

typedef enum bme280_mode_e {
    mode_sleep = 0x00,
    mode_forced = 0x01,
    mode_normal = 0x03,
} bme280_mode_t;

typedef struct {
    uint8_t osr_t;         // temperature oversampling
    uint8_t osr_p;         // pressure oversampling
    uint8_t osr_h;         // humidity oversampling
    uint8_t filter;        // filter coefficient
    uint8_t standby_time;  // standby time
} settings_t;

typedef struct {
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
    uint8_t dig_h1;
    int16_t dig_h2;
    uint8_t dig_h3;
    int16_t dig_h4;
    int16_t dig_h5;
    int8_t dig_h6;
} calib_data_t;

typedef struct {
    uint32_t temperature;
    uint32_t pressure;
    uint16_t humidity;
} raw_data_t;

static inline void cs_select(uint8_t pin_cs) {
    asm volatile("nop \n nop \n nop");
    gpio_put(pin_cs, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(uint8_t pin_cs) {
    asm volatile("nop \n nop \n nop");
    gpio_put(pin_cs, 1);
    asm volatile("nop \n nop \n nop");
}

static void write_register(uint8_t addr, uint8_t data) {
    uint8_t buf[2];
    buf[0] = addr & 0x7F;
    buf[1] = data;
    cs_select(PIN_SPI_CS_BME280);
    spi_write_blocking(SPI_ID, buf, 2);
    cs_deselect(PIN_SPI_CS_BME280);
    sleep_us(10);
}

static void read_registers(uint8_t addr, uint8_t* buf, uint16_t len) {
    addr |= 0x80;
    cs_select(PIN_SPI_CS_BME280);
    spi_write_blocking(SPI_ID, &addr, 1);
    sleep_us(10);
    spi_read_blocking(SPI_ID, 0, buf, len);
    cs_deselect(PIN_SPI_CS_BME280);
    sleep_us(10);
}

void reset() {
    write_register(0xE0, 0xB6);
}

uint8_t get_chip_id() {
    uint8_t chip_id = 0;
    read_registers(0xD0, &chip_id, 1);
    return chip_id;
}

bool is_chip_id_valid(uint8_t chip_id) {
    return chip_id == 0x60;
}

uint8_t get_status() {
    uint8_t status = 0;
    read_registers(0xF3, &status, 1);
    return status;
}

bool is_status_measuring(uint8_t status) {
    return status & 0x08;
}

bool is_status_im_update(uint8_t status) {
    return status & 0x01;
}

void get_calib_data(calib_data_t* calib_data) {
    uint8_t buf[26];
    read_registers(0x88, buf, 26);
    calib_data->dig_t1 = (uint16_t)buf[1] << 8 | buf[0];
    calib_data->dig_t2 = (int16_t)(buf[3] << 8 | buf[2]);
    calib_data->dig_t3 = (int16_t)(buf[5] << 8 | buf[4]);
    calib_data->dig_p1 = (uint16_t)buf[7] << 8 | buf[6];
    calib_data->dig_p2 = (int16_t)(buf[9] << 8 | buf[8]);
    calib_data->dig_p3 = (int16_t)(buf[11] << 8 | buf[10]);
    calib_data->dig_p4 = (int16_t)(buf[13] << 8 | buf[12]);
    calib_data->dig_p5 = (int16_t)(buf[15] << 8 | buf[14]);
    calib_data->dig_p6 = (int16_t)(buf[17] << 8 | buf[16]);
    calib_data->dig_p7 = (int16_t)(buf[19] << 8 | buf[18]);
    calib_data->dig_p8 = (int16_t)(buf[21] << 8 | buf[20]);
    calib_data->dig_p9 = (int16_t)(buf[23] << 8 | buf[22]);
    calib_data->dig_h1 = buf[25];
    read_registers(0xE1, buf, 7);
    calib_data->dig_h2 = (int16_t)(buf[1] << 8 | buf[0]);
    calib_data->dig_h3 = buf[2];
    calib_data->dig_h4 = (int16_t)(buf[3] << 4 | (buf[4] & 0x0F));
    calib_data->dig_h5 = (int16_t)(buf[5] << 4 | buf[4] >> 4);
    calib_data->dig_h6 = (int8_t)buf[6];
}

uint8_t get_mode() {
    uint8_t buf = 0;
    read_registers(0xF4, &buf, 1);
    return buf & 0x03;
}

void set_mode(uint8_t mode) {
    uint8_t buf = 0;
    read_registers(0xF4, &buf, 1);
    buf = (buf & 0xFC) | mode;
    write_register(0xF4, buf);
}

void get_settings(settings_t* settings) {
    uint8_t buf[3];
    read_registers(0xF2, buf, 1);
    read_registers(0xF4, buf + 1, 2);
    settings->osr_h = buf[0] & 0x07;
    settings->osr_p = (buf[1] & 0x1C) >> 2;
    settings->osr_t = (buf[1] & 0xE0) >> 5;
    settings->filter = (buf[2] & 0x1C) >> 2;
    settings->standby_time = (buf[2] & 0xE0) >> 5;
}

void set_settings(settings_t* settings) {
    uint8_t mode = get_mode();
    write_register(0xF2, settings->osr_h);
    write_register(0xF4, settings->osr_t << 5 | settings->osr_p << 2 | mode);
    write_register(0xF5, settings->standby_time << 5 | settings->filter << 2);
}

void get_raw_data(raw_data_t* raw_data) {
    uint8_t buf[8];
    read_registers(0xF7, buf, 8);
    raw_data->pressure = buf[0] << 12 | buf[1] << 4 | buf[2] >> 4;
    raw_data->temperature = buf[3] << 12 | buf[4] << 4 | buf[5] >> 4;
    raw_data->humidity = buf[6] << 8 | buf[7];
}

double compensate_temperature(uint32_t raw_data, calib_data_t* calib_data,
                              int32_t* t_fine) {
    double var1 =
        ((double)raw_data) / 16384.0 - ((double)calib_data->dig_t1) / 1024.0;
    var1 = var1 * ((double)calib_data->dig_t2);
    double var2 =
        (((double)raw_data) / 131072.0 - ((double)calib_data->dig_t1) / 8192.0);
    var2 = (var2 * var2) * ((double)calib_data->dig_t3);
    *t_fine = (int32_t)(var1 + var2);
    double temperature = (var1 + var2) / 5120.0;
    return temperature < -40.0  ? -40.0
           : 85.0 < temperature ? 85.0
                                : temperature;
}

double compensate_pressure(uint32_t raw_data, calib_data_t* calib_data,
                           int32_t t_fine) {
    double var1 = ((double)t_fine / 2.0) - 64000.0;
    double var2 = var1 * var1 * ((double)calib_data->dig_p6) / 32768.0;
    var2 = var2 + var1 * ((double)calib_data->dig_p5) * 2.0;
    var2 = (var2 / 4.0) + (((double)calib_data->dig_p4) * 65536.0);
    var1 = ((((double)calib_data->dig_p3) * var1 * var1 / 524288.0) +
            ((double)calib_data->dig_p2) * var1) /
           524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)calib_data->dig_p1);
    if (var1 > 0.0) {
        double var3 = 1048576.0 - (double)raw_data;
        var3 = (var3 - (var2 / 4096.0)) * 6250.0 / var1;
        var1 = ((double)calib_data->dig_p9) * var3 * var3 / 2147483648.0;
        var2 = var3 * ((double)calib_data->dig_p8) / 32768.0;
        double pressure =
            (var3 + (var1 + var2 + ((double)calib_data->dig_p7)) / 16.0) / 100;
        return pressure < 300.0 ? 300.0 : 1100.0 < pressure ? 1100.0 : pressure;
    }
    return 300.0;
}

double compensate_humidity(uint16_t raw_data, calib_data_t* calib_data,
                           int32_t t_fine) {
    double var1 = ((double)t_fine) - 76800.0;
    double var2 = (((double)calib_data->dig_h4) * 64.0 +
                   (((double)calib_data->dig_h5) / 16384.0) * var1);
    double var3 = raw_data - var2;
    double var4 = ((double)calib_data->dig_h2) / 65536.0;
    double var5 = (1.0 + (((double)calib_data->dig_h3) / 67108864.0) * var1);
    double var6 =
        1.0 + (((double)calib_data->dig_h6) / 67108864.0) * var1 * var5;
    var6 = var3 * var4 * (var5 * var6);
    double humidity =
        var6 * (1.0 - ((double)calib_data->dig_h1) * var6 / 524288.0);
    humidity = humidity < 0.0 ? 0.0 : 100.0 < humidity ? 100.0 : humidity;
    return humidity;
}

bool mqtt_connected = false;

void mqtt_connect_cb(mqtt_client_t* client, void* arg,
                     mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("connect success\n");
        mqtt_connected = true;
    } else {
        printf("connect failed\n");
        mqtt_connected = false;
    }
}

void mqtt_connect(mqtt_client_t* client) {
    ip_addr_t broker_ip;
    ipaddr_aton(MQTT_BROKER_IP, &broker_ip);

    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = "raspberry_pi_pico_w";
    ci.keep_alive = 60;

    mqtt_client_connect(client, &broker_ip, MQTT_BROKER_PORT, mqtt_connect_cb,
                        nullptr, &ci);
}

void mqtt_publish(mqtt_client_t* client, const std::string& topic_s,
                  const std::string& str) {
    const char* topic = topic_s.c_str();
    const char* payload = str.c_str();

    err_t err = mqtt_publish(client, topic, payload, strlen(payload), 0, 0,
                             nullptr, nullptr);
    if (err == ERR_OK) {
        printf("published %s\n", payload);
    } else {
        printf("publish error: %d\n", err);
    }
}

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

uint16_t mcp3204_get_raw(mcp3204_channel_t channel) {
    uint8_t tx_buf[3] = {
        0x02 | (channel >> 3),
        (channel & 0x07) << 5,
        0x00,
    };
    uint8_t rx_buf[3];

    cs_select(PIN_SPI_CS_MCP3204);
    spi_write_read_blocking(SPI_ID, tx_buf, rx_buf, 3);
    cs_deselect(PIN_SPI_CS_MCP3204);

    return (rx_buf[1] & 0x0F) << 8 | rx_buf[2];
}

// clang-format off
bi_decl(bi_3pins_with_func(PIN_SPI_SCK, PIN_SPI_TX, PIN_SPI_RX, GPIO_FUNC_SPI));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_BME280, "SPI CS for bme280"));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3204, "SPI CS for mcp3204"));
// clang-format on

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

    reset();

    do {
        sleep_ms(1);
    } while (is_status_im_update(get_status()));

    uint8_t chip_id = get_chip_id();
    if (is_chip_id_valid(chip_id)) {
        printf("chip_id: %#x is valid\n", chip_id);
    }

    calib_data_t calib_data;
    get_calib_data(&calib_data);

    settings_t settings = {
        .osr_t = osr_x4,
        .osr_p = osr_x4,
        .osr_h = osr_x4,
        .filter = filter_x2,
        .standby_time = standby_time_1000ms,
    };
    set_settings(&settings);

    if (cyw43_arch_init()) {
        printf("Failed to initialize.\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    while (cyw43_arch_wifi_connect_timeout_ms(
               WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        printf("Failed to connect.\n");
        sleep_ms(1000);
    }

    printf("wifi connected\n");

    for (int i = 0; i < 3; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(200);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(200);
    }

    mqtt_client_t* mqtt = mqtt_client_new();
    mqtt_connect(mqtt);

    for (;;) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

        if (!mqtt_connected) {
            printf("reconnecting\n");
            mqtt_connect(mqtt);
        }

        set_mode(mode_forced);

        uint16_t raw_x = mcp3204_get_raw(mcp3204_channel_single_ch0);
        uint16_t raw_y = mcp3204_get_raw(mcp3204_channel_single_ch1);
        uint16_t raw_z = mcp3204_get_raw(mcp3204_channel_single_ch2);

        double x = raw_x * 3.3 / 4096;
        double y = raw_y * 3.3 / 4096;
        double z = raw_z * 3.3 / 4096;

        do {
            sleep_ms(10);
        } while (is_status_measuring(get_status()));

        raw_data_t raw_data;
        get_raw_data(&raw_data);

        int32_t t_fine = 0;
        double temp =
            compensate_temperature(raw_data.temperature, &calib_data, &t_fine);
        double pres =
            compensate_pressure(raw_data.pressure, &calib_data, t_fine);
        double hum =
            compensate_humidity(raw_data.humidity, &calib_data, t_fine);

        auto time_current = get_absolute_time();
        auto time_usec = to_us_since_boot(time_current);

        auto json_temp =
            std::format(R"({{"usec":{},"temp":{},"pres":{},"hum":{}}})",
                        time_usec, temp, pres, hum);
        auto json_acc = std::format(R"({{"usec":{},"x":{},"y":{},"z":{}}})",
                                    time_usec, x, y, z);

        mqtt_publish(mqtt, "temp", json_temp);
        mqtt_publish(mqtt, "acc", json_acc);

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        sleep_ms(100);
    }
}
