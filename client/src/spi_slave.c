#include "spi_slave.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/util/queue.h>

#include "spi_slave.pio.h"

#define PIO_ID (pio1)
#define DREQ_RX_BASE DREQ_PIO1_RX0
#define DREQ_TX_BASE DREQ_PIO1_TX0

#define PIN_SCK (10)
#define PIN_TX (11)
#define PIN_RX (12)
#define PIN_CS (13)

static uint sm, offset;
static pio_sm_config pio_cfg;

static int dma_chan_tx, dma_chan_rx;

static uint8_t tx_buf[SPI_SLAVE_BUF_SIZE];
static uint8_t rx_buf[SPI_SLAVE_BUF_SIZE];

static queue_t queue_tx;

static void cs_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        pio_sm_set_enabled(PIO_ID, sm, false);

        dma_channel_abort(dma_chan_tx);
        dma_channel_abort(dma_chan_rx);

        pio_sm_init(PIO_ID, sm, offset, &pio_cfg);
        pio_sm_clear_fifos(PIO_ID, sm);

        dma_channel_set_read_addr(dma_chan_tx, tx_buf, false);
        dma_channel_set_write_addr(dma_chan_rx, rx_buf, false);
        dma_channel_set_read_addr(dma_chan_rx, &PIO_ID->rxf[sm], false);
        dma_channel_set_trans_count(dma_chan_tx, SPI_SLAVE_BUF_SIZE, false);
        dma_channel_set_trans_count(dma_chan_rx, SPI_SLAVE_BUF_SIZE, false);

        if (rx_buf[0] == 0x01) {
            bool res = queue_try_remove(&queue_tx, tx_buf);
            if (!res) {
                memset(tx_buf, 0x00, SPI_SLAVE_BUF_SIZE);
            }
        }

        dma_start_channel_mask((1u << dma_chan_rx) | (1u << dma_chan_tx));

        pio_sm_set_enabled(PIO_ID, sm, true);
    }
}

void spi_slave_init() {
    queue_init(&queue_tx, SPI_SLAVE_BUF_SIZE, SPI_SLAVE_QUEUE_COUNT);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_IN);
    gpio_pull_up(PIN_CS);
    gpio_set_irq_enabled_with_callback(PIN_CS, GPIO_IRQ_EDGE_RISE, true,
                                       &cs_callback);

    offset = pio_add_program(PIO_ID, &spi_slave_program);
    sm = pio_claim_unused_sm(PIO_ID, true);

    pio_gpio_init(PIO_ID, PIN_SCK);
    pio_gpio_init(PIO_ID, PIN_TX);
    pio_gpio_init(PIO_ID, PIN_RX);
    pio_sm_set_consecutive_pindirs(PIO_ID, sm, PIN_SCK, 1, GPIO_IN);
    pio_sm_set_consecutive_pindirs(PIO_ID, sm, PIN_TX, 1, GPIO_OUT);
    pio_sm_set_consecutive_pindirs(PIO_ID, sm, PIN_RX, 1, GPIO_IN);

    pio_cfg = spi_slave_program_get_default_config(offset);
    sm_config_set_out_pins(&pio_cfg, PIN_TX, 1);
    sm_config_set_in_pins(&pio_cfg, PIN_RX);
    sm_config_set_out_shift(&pio_cfg, false, false, 8);
    sm_config_set_in_shift(&pio_cfg, false, false, 8);
    sm_config_set_clkdiv(&pio_cfg, 1.0);

    pio_sm_init(PIO_ID, sm, offset, &pio_cfg);
    pio_sm_set_enabled(PIO_ID, sm, true);

    dma_chan_tx = dma_claim_unused_channel(true);
    dma_channel_config c_tx = dma_channel_get_default_config(dma_chan_tx);
    channel_config_set_transfer_data_size(&c_tx, DMA_SIZE_8);
    channel_config_set_dreq(&c_tx, DREQ_TX_BASE + sm);  // 消すと1byteずれる
    channel_config_set_read_increment(&c_tx, true);
    channel_config_set_write_increment(&c_tx, false);
    dma_channel_configure(dma_chan_tx, &c_tx,
                          &PIO_ID->txf[sm],    // 書き込み先
                          tx_buf,              // 読み込み元
                          SPI_SLAVE_BUF_SIZE,  // 転送サイズ
                          false                // 自動スタートしない
    );

    dma_chan_rx = dma_claim_unused_channel(true);
    dma_channel_config c_rx = dma_channel_get_default_config(dma_chan_rx);
    channel_config_set_transfer_data_size(&c_rx, DMA_SIZE_8);
    channel_config_set_dreq(&c_rx, DREQ_RX_BASE + sm);  // 消すと1byteずれる
    channel_config_set_read_increment(&c_rx, false);
    channel_config_set_write_increment(&c_rx, true);
    dma_channel_configure(dma_chan_rx, &c_rx,
                          rx_buf,            // 書き込み先
                          &PIO_ID->rxf[sm],  // 読み込み元
                          SPI_SLAVE_BUF_SIZE, false);

    dma_start_channel_mask((1u << dma_chan_rx) | (1u << dma_chan_tx));
}

bool spi_slave_push_bytes(uint8_t* data) {
    return queue_try_add(&queue_tx, data);
}
