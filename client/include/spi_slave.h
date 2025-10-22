#ifndef SPI_SLAVE_H
#define SPI_SLAVE_H

#include <stdint.h>

#include <hardware/pio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_SLAVE_BUF_SIZE (512)
#define SPI_SLAVE_QUEUE_COUNT (32)

void spi_slave_init();
bool spi_slave_push_bytes(uint8_t* data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: SPI_SLAVE_H */
