#ifndef BNO055_H
#define BNO055_H

#include <stdbool.h>
#include <stdint.h>

#include <hardware/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    bno055_mode_configmode = 0b0000,
    bno055_mode_acconly = 0b0001,
    bno055_mode_magonly = 0b0010,
    bno055_mode_gyroonly = 0b0011,
    bno055_mode_accmag = 0b0100,
    bno055_mode_accgyro = 0b0101,
    bno055_mode_maggyro = 0b0110,
    bno055_mode_amg = 0b0111,
    bno055_mode_imu = 0b1000,
    bno055_mode_compass = 0b1001,
    bno055_mode_m4g = 0b1010,
    bno055_mode_ndof_fmc_off = 0b1011,
    bno055_mode_ndof = 0b1100,
} bno055_mode_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} bno055_accel_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} bno055_gyro_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} bno055_mag_t;

typedef struct {
    int16_t heading;
    int16_t roll;
    int16_t pitch;
} bno055_euler_t;

typedef struct {
    int16_t w;
    int16_t x;
    int16_t y;
    int16_t z;
} bno055_quaternion_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} bno055_linear_accel_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} bno055_gravity_t;

typedef struct {
    int8_t sys;
    int8_t gyro;
    int8_t accel;
    int8_t mag;
} bno055_calib_status_t;

typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    int16_t mag_x;
    int16_t mag_y;
    int16_t mag_z;

    int16_t accel_radius;
    int16_t mag_radius;
} bno055_calib_data_t;

typedef struct {
    i2c_inst_t* i2c_id;
    uint8_t addr;
} bno055_dev_t;

uint8_t bno055_get_chip_id(bno055_dev_t* dev);
bool bno055_is_chip_id_valid(uint8_t chip_id);

void bno055_set_mode(bno055_dev_t* dev, uint8_t mode);

void bno055_read_accel(bno055_dev_t* dev, bno055_accel_t* acc);
void bno055_read_gyro(bno055_dev_t* dev, bno055_gyro_t* gyro);
void bno055_read_mag(bno055_dev_t* dev, bno055_mag_t* mag);
void bno055_read_euler(bno055_dev_t* dev, bno055_euler_t* euler);
void bno055_read_quaternion(bno055_dev_t* dev, bno055_quaternion_t* quaternion);
void bno055_read_linear_accel(bno055_dev_t* dev,
                              bno055_linear_accel_t* linear_accel);
void bno055_read_gravity(bno055_dev_t* dev, bno055_gravity_t* gravity);
void bno055_read_calib_status(bno055_dev_t* dev, bno055_calib_status_t* status);
void bno055_read_calib_data(bno055_dev_t* dev, bno055_calib_data_t* data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: BNO055_H */
