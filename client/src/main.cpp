#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <format>
#include <string>

#include <cyw43_country.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <lwip/apps/mqtt.h>
#include <lwip/memp.h>
#include <lwip/stats.h>
#include <pico/binary_info.h>
#include <pico/cyw43_arch.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include "bme280.h"
#include "mcp3208.h"

#define SPI_ID spi0
#define SPI_BAUD 1'000'000

#define PIN_SPI_SCK 2
#define PIN_SPI_TX 3
#define PIN_SPI_RX 4
#define PIN_SPI_CS_BME280 5
#define PIN_SPI_CS_MCP3208 6

#define WIFI_SSID "Buffalo-G-4810"
#define WIFI_PASSWORD "password"

#define MQTT_BROKER_IP "192.168.11.10"
#define MQTT_BROKER_PORT 1883

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
        if (err == ERR_MEM) {
            for (int i = 0; i < MEMP_MAX; i++) {
                MEMP_STATS_DISPLAY(i);
            }
        }
    }
}

// clang-format off
bi_decl(bi_3pins_with_func(PIN_SPI_SCK, PIN_SPI_TX, PIN_SPI_RX, GPIO_FUNC_SPI));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_BME280, "SPI CS for bme280"));
bi_decl(bi_1pin_with_name(PIN_SPI_CS_MCP3208, "SPI CS for mcp3208"));
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

    gpio_init(PIN_SPI_CS_MCP3208);
    gpio_set_dir(PIN_SPI_CS_MCP3208, GPIO_OUT);
    gpio_put(PIN_SPI_CS_MCP3208, 1);

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

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_JAPAN)) {
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
        auto time_start = get_absolute_time();

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

        if (!mqtt_connected) {
            printf("reconnecting\n");
            mqtt_connect(mqtt);
        }

        bme280_set_mode(bme280_mode_forced);

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

        do {
            sleep_ms(10);
        } while (bme280_is_status_measuring(bme280_get_status()));

        bme280_raw_data_t raw_data;
        bme280_get_raw_data(&raw_data);

        int32_t t_fine = 0;
        double temp = bme280_compensate_temperature(raw_data.temperature,
                                                    &calib_data, &t_fine);
        double pres =
            bme280_compensate_pressure(raw_data.pressure, &calib_data, t_fine);
        double hum =
            bme280_compensate_humidity(raw_data.humidity, &calib_data, t_fine);

        auto time_current = get_absolute_time();
        auto time_usec = to_us_since_boot(time_current);

        auto json_temp = std::format(
            R"({{"usec":{},"temp":{:.2f},"pres":{:.2f},"hum":{:.2f}}})",
            time_usec, temp, pres, hum);
        auto json_water = std::format(
            R"({{"usec":{},"in":{:.2f},"out":{:.2f}}})", time_usec, in, out);
        auto json_acc =
            std::format(R"({{"usec":{},"x":{:.2f},"y":{:.2f},"z":{:.2f}}})",
                        time_usec, x, y, z);

        mqtt_publish(mqtt, "temp", json_temp);
        mqtt_publish(mqtt, "water", json_water);
        mqtt_publish(mqtt, "acc", json_acc);

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        cyw43_arch_poll();

        auto time_next = delayed_by_ms(time_start, 100);
        sleep_until(time_next);
    }
}
