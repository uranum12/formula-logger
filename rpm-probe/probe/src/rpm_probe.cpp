#include <stdio.h>
#include <string.h>

#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <pico/stdio.h>
#include <pico/util/queue.h>

#include <cJSON.h>

#define STR_SIZE (512)
#define QUEUE_SIZE (32)

#define UART_ID (uart0)
#define UART_BAUD (115'200)

#define PIN_UART_TX (0)
#define PIN_UART_RX (1)

queue_t uart_queue;

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

int main() {
    stdio_init_all();

    queue_init(&uart_queue, STR_SIZE, QUEUE_SIZE);

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
        if (queue_try_remove(&uart_queue, &str)) {
            cJSON* root = cJSON_Parse(str);

            char* topic =
                cJSON_GetStringValue(cJSON_GetObjectItem(root, "topic"));
            if (strcmp(topic, "rpm") == 0) {
                char* payload =
                    cJSON_GetStringValue(cJSON_GetObjectItem(root, "payload"));
                printf("%s\n", payload);
            }

            cJSON_Delete(root);
        }
    }
}
