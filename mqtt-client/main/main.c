#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

#define PIN_LED 21

#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_TX_PIN 43
#define UART_RX_PIN 44
#define BUF_SIZE 1024

#define WIFI_SSID "kanazawa-formula-g"
#define WIFI_PASS "kanazawa-password"
#define MQTT_BROKER_URI "mqtt://192.168.0.10:1883"

static const char* TAG = "MQTT_CLIENT";

static QueueHandle_t uart_queue;
static QueueHandle_t mqtt_queue;

typedef struct {
    char topic[16];
    char payload[256];
} mqtt_msg_t;

static char rx_buffer[BUF_SIZE];
static size_t rx_len = 0;

static void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
            },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void task_mqtt(void* pvParameters) {
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvParameters;
    mqtt_msg_t msg;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);

    for (;;) {
        while (xQueueReceive(mqtt_queue, &msg, 0) == pdTRUE) {
            gpio_set_level(PIN_LED, 0);
            esp_mqtt_client_publish(client, msg.topic, msg.payload, 0, 0, 0);
            ESP_LOGI(TAG, "Publishing: topic=%s, payload=%s", msg.topic,
                     msg.payload);
            gpio_set_level(PIN_LED, 1);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

static void process_line(const char* line) {
    ESP_LOGI(TAG, "UART Line: %s", line);

    cJSON* root = cJSON_Parse(line);
    if (!root) {
        ESP_LOGE(TAG, "Invalid JSON");
        return;
    }

    cJSON* topic = cJSON_GetObjectItem(root, "topic");
    cJSON* payload = cJSON_GetObjectItem(root, "payload");

    if (cJSON_IsString(topic) && cJSON_IsString(payload)) {
        mqtt_msg_t msg;
        strncpy(msg.topic, topic->valuestring, sizeof(msg.topic) - 1);
        strncpy(msg.payload, payload->valuestring, sizeof(msg.payload) - 1);

        if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE) {
            ESP_LOGW(TAG, "MQTT queue full, dropping message");
        } else {
            ESP_LOGI(TAG, "Queued topic=%s, payload=%s", msg.topic,
                     msg.payload);
        }
    } else {
        ESP_LOGE(TAG, "JSON missing topic/payload");
    }

    cJSON_Delete(root);
}

static void task_uart(void* pvParameters) {
    uart_event_t event;
    uint8_t data[128];

    for (;;) {
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                int len = uart_read_bytes(UART_PORT_NUM, data, event.size,
                                          portMAX_DELAY);
                if (len > 0) {
                    for (int i = 0; i < len; i++) {
                        char c = data[i];
                        if (rx_len < BUF_SIZE - 1) {
                            rx_buffer[rx_len++] = c;
                        }

                        if (c == '\n') {
                            rx_buffer[rx_len - 1] = '\0';
                            process_line(rx_buffer);
                            rx_len = 0;
                        }
                    }
                }
            }
        }
    }
}

static void task_ping(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(60 * 1000);
    for (;;) {
        mqtt_msg_t msg;

        TickType_t now = xTaskGetTickCount();
        uint32_t ms = now * portTICK_PERIOD_MS;

        snprintf(msg.topic, sizeof(msg.topic), "ping");
        snprintf(msg.payload, sizeof(msg.payload), "{\"ms\":%lu}",
                 (unsigned long)ms);

        if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE) {
            ESP_LOGW(TAG, "MQTT queue full, dropping message");
        } else {
            ESP_LOGI(TAG, "Queued topic=%s, payload=%s", msg.topic,
                     msg.payload);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void app_main(void) {
    gpio_reset_pin(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED, 1);

    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20,
                        &uart_queue, 0);

    mqtt_queue = xQueueCreate(200, sizeof(mqtt_msg_t));

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);

    xTaskCreate(task_mqtt, "mqtt", 16 * 1024, (void*)client, 5, NULL);
    xTaskCreate(task_uart, "uart", 4 * 1024, NULL, 4, NULL);
    xTaskCreate(task_ping, "ping", 4 * 1024, NULL, 3, NULL);
}
