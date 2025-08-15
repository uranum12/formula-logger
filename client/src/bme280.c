#include "bme280.h"

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/time.h>

#define SPI_ID (spi0)
#define PIN_SPI_CS_BME280 (5)

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_SPI_CS_BME280, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_SPI_CS_BME280, 1);
    asm volatile("nop \n nop \n nop");
}

static void write_register(uint8_t addr, uint8_t data) {
    uint8_t buf[2];
    buf[0] = addr & 0x7F;
    buf[1] = data;
    cs_select();
    spi_write_blocking(SPI_ID, buf, 2);
    cs_deselect();
    sleep_us(10);
}

static void read_registers(uint8_t addr, uint8_t* buf, uint16_t len) {
    addr |= 0x80;
    cs_select();
    spi_write_blocking(SPI_ID, &addr, 1);
    sleep_us(10);
    spi_read_blocking(SPI_ID, 0, buf, len);
    cs_deselect();
    sleep_us(10);
}

void bme280_reset() {
    write_register(0xE0, 0xB6);
}

uint8_t bme280_get_chip_id() {
    uint8_t chip_id = 0;
    read_registers(0xD0, &chip_id, 1);
    return chip_id;
}

bool bme280_is_chip_id_valid(uint8_t chip_id) {
    return chip_id == 0x60;
}

uint8_t bme280_get_status() {
    uint8_t status = 0;
    read_registers(0xF3, &status, 1);
    return status;
}

bool bme280_is_status_measuring(uint8_t status) {
    return status & 0x08;
}

bool bme280_is_status_im_update(uint8_t status) {
    return status & 0x01;
}

void bme280_get_calib_data(bme280_calib_data_t* calib_data) {
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

uint8_t bme280_get_mode() {
    uint8_t buf = 0;
    read_registers(0xF4, &buf, 1);
    return buf & 0x03;
}

void bme280_set_mode(uint8_t mode) {
    uint8_t buf = 0;
    read_registers(0xF4, &buf, 1);
    buf = (buf & 0xFC) | mode;
    write_register(0xF4, buf);
}

void bme280_get_settings(bme280_settings_t* settings) {
    uint8_t buf[3];
    read_registers(0xF2, buf, 1);
    read_registers(0xF4, buf + 1, 2);
    settings->osr_h = buf[0] & 0x07;
    settings->osr_p = (buf[1] & 0x1C) >> 2;
    settings->osr_t = (buf[1] & 0xE0) >> 5;
    settings->filter = (buf[2] & 0x1C) >> 2;
    settings->standby_time = (buf[2] & 0xE0) >> 5;
}

void bme280_set_settings(bme280_settings_t* settings) {
    uint8_t mode = bme280_get_mode();
    write_register(0xF2, settings->osr_h);
    write_register(0xF4, settings->osr_t << 5 | settings->osr_p << 2 | mode);
    write_register(0xF5, settings->standby_time << 5 | settings->filter << 2);
}

void bme280_get_raw_data(bme280_raw_data_t* raw_data) {
    uint8_t buf[8];
    read_registers(0xF7, buf, 8);
    raw_data->pressure = buf[0] << 12 | buf[1] << 4 | buf[2] >> 4;
    raw_data->temperature = buf[3] << 12 | buf[4] << 4 | buf[5] >> 4;
    raw_data->humidity = buf[6] << 8 | buf[7];
}

double bme280_compensate_temperature(uint32_t raw_data,
                                     bme280_calib_data_t* calib_data,
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

double bme280_compensate_pressure(uint32_t raw_data,
                                  bme280_calib_data_t* calib_data,
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

double bme280_compensate_humidity(uint16_t raw_data,
                                  bme280_calib_data_t* calib_data,
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
