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
#define SPI_BAUD 100000

#define SPI_SCK 2
#define SPI_TX 3
#define SPI_RX 4
#define SPI_CS 5

#define WIFI_SSID "Buffalo-G-4810"
#define WIFI_PASSWORD "password"

#define MQTT_BROKER_IP "192.168.11.10"
#define MQTT_BROKER_PORT 1883

enum class Channel : uint8_t {
    differential_0 = 0x00,
    differential_1 = 0x01,
    single_0 = 0x02,
    single_1 = 0x03,
};

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(SPI_CS, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(SPI_CS, 1);
    asm volatile("nop \n nop \n nop");
}

uint16_t read_volt(Channel ch) {
    const uint8_t tx_buf[2] = {
        static_cast<uint8_t>(0x90 | static_cast<std::uint8_t>(ch) << 5),
        0x00,
    };
    uint8_t rx_buf[2];

    cs_select();
    spi_write_read_blocking(SPI_ID, tx_buf, rx_buf, 2);
    cs_deselect();

    return (rx_buf[0] & 0x07) << 7 | rx_buf[1] >> 1;
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

void mqtt_publish(mqtt_client_t* client, const std::string& str) {
    const char* topic = "temp";
    const char* payload = str.c_str();

    err_t err = mqtt_publish(client, topic, payload, strlen(payload), 0, 0,
                             nullptr, nullptr);
    if (err == ERR_OK) {
        printf("published %s\n", payload);
    } else {
        printf("publish error: %d\n", err);
    }
}

int main() {
    stdio_init_all();
    printf("start\n");

    spi_init(SPI_ID, SPI_BAUD);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(SPI_RX, GPIO_FUNC_SPI);
    bi_decl(bi_3pins_with_func(SPI_SCK, SPI_TX, SPI_RX, GPIO_FUNC_SPI));

    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1);
    bi_decl(bi_1pin_with_name(SPI_CS, "SPI CS"));

    if (cyw43_arch_init()) {
        printf("Failed to initialize.\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                           CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect.\n");
        return 1;
    }

    printf("wifi connected\n");

    mqtt_client_t* mqtt = mqtt_client_new();
    mqtt_connect(mqtt);

    for (;;) {
        if (!mqtt_connected) {
            printf("reconnecting\n");
            mqtt_connect(mqtt);
        }

        auto temp = read_volt(Channel::single_0);

        auto time_current = get_absolute_time();
        auto time_usec = to_us_since_boot(time_current);

        auto json_str =
            std::format(R"({{"usec":{},"temp":{}}})", time_usec, temp);

        mqtt_publish(mqtt, json_str);

        sleep_ms(100);
    }
}
