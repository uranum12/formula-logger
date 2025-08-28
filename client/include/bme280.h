#ifndef BME280_H
#define BME280_H

#include <stdbool.h>
#include <stdint.h>

#include <hardware/spi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    bme280_osr_skip = 0x00,
    bme280_osr_x1 = 0x01,
    bme280_osr_x2 = 0x02,
    bme280_osr_x4 = 0x03,
    bme280_osr_x8 = 0x04,
    bme280_osr_x16 = 0x05,
} bme280_osr_t;

typedef enum {
    bme280_filter_off = 0x00,
    bme280_filter_x2 = 0x01,
    bme280_filter_x4 = 0x02,
    bme280_filter_x8 = 0x03,
    bme280_filter_x16 = 0x04,
} bme280_filter_t;

typedef enum {
    bme280_standby_time_0p5ms = 0x00,
    bme280_standby_time_62p5ms = 0x01,
    bme280_standby_time_125ms = 0x02,
    bme280_standby_time_250ms = 0x03,
    bme280_standby_time_500ms = 0x04,
    bme280_standby_time_1000ms = 0x05,
    bme280_standby_time_10ms = 0x06,
    bme280_standby_time_20ms = 0x07,
} bme280_standby_time_t;

typedef enum {
    bme280_mode_sleep = 0x00,
    bme280_mode_forced = 0x01,
    bme280_mode_normal = 0x03,
} bme280_mode_t;

typedef struct {
    uint8_t osr_t;         // temperature oversampling
    uint8_t osr_p;         // pressure oversampling
    uint8_t osr_h;         // humidity oversampling
    uint8_t filter;        // filter coefficient
    uint8_t standby_time;  // standby time
} bme280_settings_t;

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
} bme280_calib_data_t;

typedef struct {
    uint32_t temperature;
    uint32_t pressure;
    uint16_t humidity;
} bme280_raw_data_t;

typedef struct {
    spi_inst_t* spi_id;
    uint8_t pin_cs;
} bme280_dev_t;

void bme280_reset(bme280_dev_t* dev);

uint8_t bme280_get_chip_id(bme280_dev_t* dev);
bool bme280_is_chip_id_valid(uint8_t chip_id);

uint8_t bme280_get_status(bme280_dev_t* dev);
bool bme280_is_status_measuring(uint8_t status);
bool bme280_is_status_im_update(uint8_t status);

void bme280_get_calib_data(bme280_dev_t* dev, bme280_calib_data_t* calib_data);

uint8_t bme280_get_mode(bme280_dev_t* dev);
void bme280_set_mode(bme280_dev_t* dev, uint8_t mode);

void bme280_get_settings(bme280_dev_t* dev, bme280_settings_t* settings);
void bme280_set_settings(bme280_dev_t* dev, bme280_settings_t* settings);

void bme280_get_raw_data(bme280_dev_t* dev, bme280_raw_data_t* raw_data);

double bme280_compensate_temperature(uint32_t raw_data,
                                     bme280_calib_data_t* calib_data,
                                     int32_t* t_fine);
double bme280_compensate_pressure(uint32_t raw_data,
                                  bme280_calib_data_t* calib_data,
                                  int32_t t_fine);
double bme280_compensate_humidity(uint16_t raw_data,
                                  bme280_calib_data_t* calib_data,
                                  int32_t t_fine);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: BME280_H */
